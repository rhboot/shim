// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * verify.c - verification routines for UEFI Secure Boot
 * Copyright Peter Jones <pjones@redhat.com>
 * Copyright Matthew Garrett
 *
 * Significant portions of this code are derived from Tianocore
 * (http://tianocore.sf.net) and are Copyright 2009-2012 Intel
 * Corporation.
 */

#include "shim.h"

#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/ocsp.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h>
#include <openssl/dso.h>

#include <Library/BaseCryptLib.h>

#define OID_EKU_MODSIGN "1.3.6.1.4.1.2312.16.1.2"

typedef enum {
	DATA_FOUND,
	DATA_NOT_FOUND,
	VAR_NOT_FOUND
} CHECK_STATUS;

void
init_openssl(void)
{
	OPENSSL_init();
	ERR_load_ERR_strings();
	ERR_load_BN_strings();
	ERR_load_RSA_strings();
	ERR_load_DH_strings();
	ERR_load_EVP_strings();
	ERR_load_BUF_strings();
	ERR_load_OBJ_strings();
	ERR_load_PEM_strings();
	ERR_load_X509_strings();
	ERR_load_ASN1_strings();
	ERR_load_CONF_strings();
	ERR_load_CRYPTO_strings();
	ERR_load_COMP_strings();
	ERR_load_BIO_strings();
	ERR_load_PKCS7_strings();
	ERR_load_X509V3_strings();
	ERR_load_PKCS12_strings();
	ERR_load_RAND_strings();
	ERR_load_DSO_strings();
	ERR_load_OCSP_strings();
}

static void
drain_openssl_errors(void)
{
	unsigned long err = -1;
	while (err != 0)
		err = ERR_get_error();
}

static BOOLEAN
verify_x509(UINT8 *Cert, UINTN CertSize)
{
	UINTN length;

	if (!Cert || CertSize < 4)
		return FALSE;

	/*
	 * A DER encoding x509 certificate starts with SEQUENCE(0x30),
	 * the number of length bytes, and the number of value bytes.
	 * The size of a x509 certificate is usually between 127 bytes
	 * and 64KB. For convenience, assume the number of value bytes
	 * is 2, i.e. the second byte is 0x82.
	 */
	if (Cert[0] != 0x30 || Cert[1] != 0x82) {
		dprint(L"cert[0:1] is [%02x%02x], should be [%02x%02x]\n",
		       Cert[0], Cert[1], 0x30, 0x82);
		return FALSE;
	}

	length = Cert[2]<<8 | Cert[3];
	if (length != (CertSize - 4)) {
		dprint(L"Cert length is %ld, expecting %ld\n",
		       length, CertSize);
		return FALSE;
	}

	return TRUE;
}

static BOOLEAN
verify_eku(UINT8 *Cert, UINTN CertSize)
{
	X509 *x509;
	CONST UINT8 *Temp = Cert;
	EXTENDED_KEY_USAGE *eku;
	ASN1_OBJECT *module_signing;

        module_signing = OBJ_nid2obj(OBJ_create(OID_EKU_MODSIGN,
                                                "modsign-eku",
                                                "modsign-eku"));

	x509 = d2i_X509 (NULL, &Temp, (long) CertSize);
	if (x509 != NULL) {
		eku = X509_get_ext_d2i(x509, NID_ext_key_usage, NULL, NULL);

		if (eku) {
			int i = 0;
			for (i = 0; i < sk_ASN1_OBJECT_num(eku); i++) {
				ASN1_OBJECT *key_usage = sk_ASN1_OBJECT_value(eku, i);

				if (OBJ_cmp(module_signing, key_usage) == 0)
					return FALSE;
			}
			EXTENDED_KEY_USAGE_free(eku);
		}

		X509_free(x509);
	}

	OBJ_cleanup();

	return TRUE;
}

static CHECK_STATUS
check_db_cert_in_ram(EFI_SIGNATURE_LIST *CertList, UINTN dbsize,
                     WIN_CERTIFICATE_EFI_PKCS *data, UINT8 *hash,
                     CHAR16 *dbname, EFI_GUID guid)
{
	EFI_SIGNATURE_DATA *Cert;
	UINTN CertSize;
	BOOLEAN IsFound = FALSE;
	int i = 0;

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		if (CompareGuid (&CertList->SignatureType, &EFI_CERT_TYPE_X509_GUID) == 0) {
			Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
			CertSize = CertList->SignatureSize - sizeof(EFI_GUID);
			dprint(L"trying to verify cert %d (%s)\n", i++, dbname);
			if (verify_x509(Cert->SignatureData, CertSize)) {
				if (verify_eku(Cert->SignatureData, CertSize)) {
					drain_openssl_errors();
					IsFound = AuthenticodeVerify (data->CertData,
								      data->Hdr.dwLength - sizeof(data->Hdr),
								      Cert->SignatureData,
								      CertSize,
								      hash, SHA256_DIGEST_SIZE);
					if (IsFound) {
						dprint(L"AuthenticodeVerify() succeeded: %d\n", IsFound);
						tpm_measure_variable(dbname, guid, CertList->SignatureSize, Cert);
						drain_openssl_errors();
						return DATA_FOUND;
					} else {
						LogError(L"AuthenticodeVerify(): %d\n", IsFound);
					}
				}
			} else if (verbose) {
				console_print(L"Not a DER encoded x.509 Certificate");
				dprint(L"cert:\n");
				dhexdumpat(Cert->SignatureData, CertSize, 0);
			}
		}

		dbsize -= CertList->SignatureListSize;
		CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
	}

	return DATA_NOT_FOUND;
}

static CHECK_STATUS
check_db_cert(CHAR16 *dbname, EFI_GUID guid, WIN_CERTIFICATE_EFI_PKCS *data,
              UINT8 *hash)
{
	CHECK_STATUS rc;
	EFI_STATUS efi_status;
	EFI_SIGNATURE_LIST *CertList;
	UINTN dbsize = 0;
	UINT8 *db;

	efi_status = get_variable(dbname, &db, &dbsize, guid);
	if (EFI_ERROR(efi_status))
		return VAR_NOT_FOUND;

	CertList = (EFI_SIGNATURE_LIST *)db;

	rc = check_db_cert_in_ram(CertList, dbsize, data, hash, dbname, guid);

	FreePool(db);

	return rc;
}

/*
 * Check a hash against an EFI_SIGNATURE_LIST in a buffer
 */
static CHECK_STATUS
check_db_hash_in_ram(EFI_SIGNATURE_LIST *CertList, UINTN dbsize, UINT8 *data,
                     int SignatureSize, EFI_GUID CertType, CHAR16 *dbname,
                     EFI_GUID guid)
{
	EFI_SIGNATURE_DATA *Cert;
	UINTN CertCount, Index;
	BOOLEAN IsFound = FALSE;

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		CertCount = (CertList->SignatureListSize -sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
		Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
		if (CompareGuid(&CertList->SignatureType, &CertType) == 0) {
			for (Index = 0; Index < CertCount; Index++) {
				if (CompareMem (Cert->SignatureData, data, SignatureSize) == 0) {
					//
					// Find the signature in database.
					//
					IsFound = TRUE;
					tpm_measure_variable(dbname, guid, CertList->SignatureSize, Cert);
					break;
				}

				Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
			}
			if (IsFound) {
				break;
			}
		}

		dbsize -= CertList->SignatureListSize;
		CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
	}

	if (IsFound)
		return DATA_FOUND;

	return DATA_NOT_FOUND;
}

/*
 * Check a hash against an EFI_SIGNATURE_LIST in a UEFI variable
 */
static CHECK_STATUS
check_db_hash(CHAR16 *dbname, EFI_GUID guid, UINT8 *data, int SignatureSize,
              EFI_GUID CertType)
{
	EFI_STATUS efi_status;
	EFI_SIGNATURE_LIST *CertList;
	UINTN dbsize = 0;
	UINT8 *db;

	efi_status = get_variable(dbname, &db, &dbsize, guid);
	if (EFI_ERROR(efi_status)) {
		return VAR_NOT_FOUND;
	}

	CertList = (EFI_SIGNATURE_LIST *)db;

	CHECK_STATUS rc = check_db_hash_in_ram(CertList, dbsize, data,
					       SignatureSize, CertType,
					       dbname, guid);
	FreePool(db);
	return rc;
}

/*
 * Check whether the binary signature or hash are present in dbx or the
 * built-in denylist
 */
static EFI_STATUS
check_denylist(WIN_CERTIFICATE_EFI_PKCS *cert, UINT8 *sha256hash,
               UINT8 *sha1hash)
{
	EFI_SIGNATURE_LIST *dbx = (EFI_SIGNATURE_LIST *)vendor_deauthorized;

	if (check_db_hash_in_ram(dbx, vendor_deauthorized_size, sha256hash,
			SHA256_DIGEST_SIZE, EFI_CERT_SHA256_GUID, L"dbx",
			EFI_SECURE_BOOT_DB_GUID) == DATA_FOUND) {
		LogError(L"binary sha256hash found in vendor dbx\n");
		return EFI_SECURITY_VIOLATION;
	}
	if (check_db_hash_in_ram(dbx, vendor_deauthorized_size, sha1hash,
				 SHA1_DIGEST_SIZE, EFI_CERT_SHA1_GUID, L"dbx",
				 EFI_SECURE_BOOT_DB_GUID) == DATA_FOUND) {
		LogError(L"binary sha1hash found in vendor dbx\n");
		return EFI_SECURITY_VIOLATION;
	}
	if (cert &&
	    check_db_cert_in_ram(dbx, vendor_deauthorized_size, cert, sha256hash, L"dbx",
				 EFI_SECURE_BOOT_DB_GUID) == DATA_FOUND) {
		LogError(L"cert sha256hash found in vendor dbx\n");
		return EFI_SECURITY_VIOLATION;
	}
	if (check_db_hash(L"dbx", EFI_SECURE_BOOT_DB_GUID, sha256hash,
			  SHA256_DIGEST_SIZE, EFI_CERT_SHA256_GUID) == DATA_FOUND) {
		LogError(L"binary sha256hash found in system dbx\n");
		return EFI_SECURITY_VIOLATION;
	}
	if (check_db_hash(L"dbx", EFI_SECURE_BOOT_DB_GUID, sha1hash,
			  SHA1_DIGEST_SIZE, EFI_CERT_SHA1_GUID) == DATA_FOUND) {
		LogError(L"binary sha1hash found in system dbx\n");
		return EFI_SECURITY_VIOLATION;
	}
	if (cert &&
	    check_db_cert(L"dbx", EFI_SECURE_BOOT_DB_GUID,
			  cert, sha256hash) == DATA_FOUND) {
		LogError(L"cert sha256hash found in system dbx\n");
		return EFI_SECURITY_VIOLATION;
	}
	if (check_db_hash(L"MokListX", SHIM_LOCK_GUID, sha256hash,
			  SHA256_DIGEST_SIZE, EFI_CERT_SHA256_GUID) == DATA_FOUND) {
		LogError(L"binary sha256hash found in Mok dbx\n");
		return EFI_SECURITY_VIOLATION;
	}
	if (cert &&
	    check_db_cert(L"MokListX", SHIM_LOCK_GUID,
			  cert, sha256hash) == DATA_FOUND) {
		LogError(L"cert sha256hash found in Mok dbx\n");
		return EFI_SECURITY_VIOLATION;
	}

	drain_openssl_errors();
	return EFI_SUCCESS;
}

static void
update_verification_method(verification_method_t method)
{
	if (verification_method == VERIFIED_BY_NOTHING)
		verification_method = method;
}

/*
 * Check whether the binary signature or hash are present in db or MokList
 */
static EFI_STATUS
check_allowlist(WIN_CERTIFICATE_EFI_PKCS *cert, UINT8 *sha256hash,
                UINT8 *sha1hash)
{
	if (!ignore_db) {
		if (check_db_hash(L"db", EFI_SECURE_BOOT_DB_GUID, sha256hash, SHA256_DIGEST_SIZE,
					EFI_CERT_SHA256_GUID) == DATA_FOUND) {
			update_verification_method(VERIFIED_BY_HASH);
			return EFI_SUCCESS;
		} else {
			LogError(L"check_db_hash(db, sha256hash) != DATA_FOUND\n");
		}
		if (check_db_hash(L"db", EFI_SECURE_BOOT_DB_GUID, sha1hash, SHA1_DIGEST_SIZE,
					EFI_CERT_SHA1_GUID) == DATA_FOUND) {
			verification_method = VERIFIED_BY_HASH;
			update_verification_method(VERIFIED_BY_HASH);
			return EFI_SUCCESS;
		} else {
			LogError(L"check_db_hash(db, sha1hash) != DATA_FOUND\n");
		}
		if (cert && check_db_cert(L"db", EFI_SECURE_BOOT_DB_GUID, cert, sha256hash)
					== DATA_FOUND) {
			verification_method = VERIFIED_BY_CERT;
			update_verification_method(VERIFIED_BY_CERT);
			return EFI_SUCCESS;
		} else if (cert) {
			LogError(L"check_db_cert(db, sha256hash) != DATA_FOUND\n");
		}
	}

#if defined(VENDOR_DB_FILE)
	EFI_SIGNATURE_LIST *db = (EFI_SIGNATURE_LIST *)vendor_db;

	if (check_db_hash_in_ram(db, vendor_db_size,
				 sha256hash, SHA256_DIGEST_SIZE,
				 EFI_CERT_SHA256_GUID, L"vendor_db",
				 EFI_SECURE_BOOT_DB_GUID) == DATA_FOUND) {
		verification_method = VERIFIED_BY_HASH;
		update_verification_method(VERIFIED_BY_HASH);
		return EFI_SUCCESS;
	} else {
		LogError(L"check_db_hash(vendor_db, sha256hash) != DATA_FOUND\n");
	}
	if (cert &&
	    check_db_cert_in_ram(db, vendor_db_size,
				 cert, sha256hash, L"vendor_db",
				 EFI_SECURE_BOOT_DB_GUID) == DATA_FOUND) {
		verification_method = VERIFIED_BY_CERT;
		update_verification_method(VERIFIED_BY_CERT);
		return EFI_SUCCESS;
	} else if (cert) {
		LogError(L"check_db_cert(vendor_db, sha256hash) != DATA_FOUND\n");
	}
#endif

	if (check_db_hash(L"MokListRT", SHIM_LOCK_GUID, sha256hash,
			  SHA256_DIGEST_SIZE, EFI_CERT_SHA256_GUID)
				== DATA_FOUND) {
		verification_method = VERIFIED_BY_HASH;
		update_verification_method(VERIFIED_BY_HASH);
		return EFI_SUCCESS;
	} else {
		LogError(L"check_db_hash(MokListRT, sha256hash) != DATA_FOUND\n");
	}
	if (cert && check_db_cert(L"MokListRT", SHIM_LOCK_GUID, cert, sha256hash)
			== DATA_FOUND) {
		verification_method = VERIFIED_BY_CERT;
		update_verification_method(VERIFIED_BY_CERT);
		return EFI_SUCCESS;
	} else if (cert) {
		LogError(L"check_db_cert(MokListRT, sha256hash) != DATA_FOUND\n");
	}

	update_verification_method(VERIFIED_BY_NOTHING);
	return EFI_NOT_FOUND;
}

static EFI_STATUS
verify_one_signature(WIN_CERTIFICATE_EFI_PKCS *sig, UINT8 *sha256hash,
                     UINT8 *sha1hash)
{
	EFI_STATUS efi_status;

	/*
	 * Ensure that the binary isn't forbidden
	 */
	drain_openssl_errors();
	efi_status = check_denylist(sig, sha256hash, sha1hash);
	if (EFI_ERROR(efi_status)) {
		perror(L"Binary is forbidden: %r\n", efi_status);
		PrintErrors();
		ClearErrors();
		crypterr(efi_status);
		return efi_status;
	}

	/*
	 * Check whether the binary is authorized in any of the firmware
	 * databases
	 */
	drain_openssl_errors();
	efi_status = check_allowlist(sig, sha256hash, sha1hash);
	if (EFI_ERROR(efi_status)) {
		if (efi_status != EFI_NOT_FOUND) {
			dprint(L"check_allowlist(): %r\n", efi_status);
			PrintErrors();
			ClearErrors();
			crypterr(efi_status);
		}
	} else {
		drain_openssl_errors();
		return efi_status;
	}

	efi_status = EFI_NOT_FOUND;
#if defined(ENABLE_SHIM_CERT)
	/*
	 * Check against the shim build key
	 */
	drain_openssl_errors();
	if (build_cert && build_cert_size) {
		dprint("verifying against shim cert\n");
	}
	if (build_cert && build_cert_size &&
	    AuthenticodeVerify(sig->CertData,
		       sig->Hdr.dwLength - sizeof(sig->Hdr),
		       build_cert, build_cert_size, sha256hash,
		       SHA256_DIGEST_SIZE)) {
		dprint(L"AuthenticodeVerify(shim_cert) succeeded\n");
		update_verification_method(VERIFIED_BY_CERT);
		tpm_measure_variable(L"Shim", SHIM_LOCK_GUID,
				     build_cert_size, build_cert);
		efi_status = EFI_SUCCESS;
		drain_openssl_errors();
		return efi_status;
	} else {
		dprint(L"AuthenticodeVerify(shim_cert) failed\n");
		PrintErrors();
		ClearErrors();
		crypterr(EFI_NOT_FOUND);
	}
#endif /* defined(ENABLE_SHIM_CERT) */

#if defined(VENDOR_CERT_FILE)
	/*
	 * And finally, check against shim's built-in key
	 */
	drain_openssl_errors();
	if (vendor_cert_size) {
		dprint("verifying against vendor_cert\n");
	}
	if (vendor_cert_size &&
	    AuthenticodeVerify(sig->CertData,
			       sig->Hdr.dwLength - sizeof(sig->Hdr),
			       vendor_cert, vendor_cert_size,
			       sha256hash, SHA256_DIGEST_SIZE)) {
		dprint(L"AuthenticodeVerify(vendor_cert) succeeded\n");
		update_verification_method(VERIFIED_BY_CERT);
		tpm_measure_variable(L"Shim", SHIM_LOCK_GUID,
				     vendor_cert_size, vendor_cert);
		efi_status = EFI_SUCCESS;
		drain_openssl_errors();
		return efi_status;
	} else {
		dprint(L"AuthenticodeVerify(vendor_cert) failed\n");
		PrintErrors();
		ClearErrors();
		crypterr(EFI_NOT_FOUND);
	}
#endif /* defined(VENDOR_CERT_FILE) */

	return efi_status;
}

/*
 * Check that the signature is valid and matches the binary
 */
static EFI_STATUS
verify_buffer_authenticode (char *data, int datasize,
			    PE_COFF_LOADER_IMAGE_CONTEXT *context,
			    UINT8 *sha256hash, UINT8 *sha1hash,
			    bool parent_verified)
{
	EFI_STATUS ret_efi_status;
	size_t size = datasize;
	size_t offset = 0;
	unsigned int i = 0;

	if (datasize < 0)
		return EFI_INVALID_PARAMETER;

	/*
	 * Clear OpenSSL's error log, because we get some DSO unimplemented
	 * errors during its intialization, and we don't want those to look
	 * like they're the reason for validation failures.
	 */
	drain_openssl_errors();

	ret_efi_status = generate_hash(data, datasize, context, sha256hash, sha1hash);
	if (EFI_ERROR(ret_efi_status)) {
		dprint(L"generate_hash: %r\n", ret_efi_status);
		PrintErrors();
		ClearErrors();
		crypterr(ret_efi_status);
		return ret_efi_status;
	}

	/*
	 * Ensure that the binary isn't forbidden by hash
	 */
	drain_openssl_errors();
	ret_efi_status = check_denylist(NULL, sha256hash, sha1hash);
	if (EFI_ERROR(ret_efi_status)) {
//		perror(L"Binary is forbidden\n");
//		dprint(L"Binary is forbidden: %r\n", ret_efi_status);
		PrintErrors();
		ClearErrors();
		crypterr(ret_efi_status);
		return ret_efi_status;
	}

	if (parent_verified)
		return EFI_SUCCESS;

	/*
	 * Check whether the binary is authorized by hash in any of the
	 * firmware databases
	 */
	drain_openssl_errors();
	ret_efi_status = check_allowlist(NULL, sha256hash, sha1hash);
	if (EFI_ERROR(ret_efi_status)) {
		LogError(L"check_allowlist(): %r\n", ret_efi_status);
		dprint(L"check_allowlist: %r\n", ret_efi_status);
		if (ret_efi_status != EFI_NOT_FOUND) {
			dprint(L"check_allowlist(): %r\n", ret_efi_status);
			PrintErrors();
			ClearErrors();
			crypterr(ret_efi_status);
			return ret_efi_status;
		}
	} else {
		drain_openssl_errors();
		return ret_efi_status;
	}

	if (context->SecDir->Size == 0) {
		dprint(L"No signatures found\n");
		return EFI_SECURITY_VIOLATION;
	}

	if (checked_add(context->SecDir->Size, context->SecDir->VirtualAddress, &offset) ||
	    offset > size) {
		perror(L"Certificate Database size is too large\n");
		return EFI_INVALID_PARAMETER;
	}

	offset = 0;
	ret_efi_status = EFI_NOT_FOUND;
	do {
		WIN_CERTIFICATE_EFI_PKCS *sig = NULL;
		size_t sz;

		sig = ImageAddress(data, size,
				   context->SecDir->VirtualAddress + offset);
		if (!sig)
			break;

		if ((uint64_t)(uintptr_t)&sig[1]
		    > (uint64_t)(uintptr_t)data + datasize) {
			perror(L"Certificate size is too large for secruity database");
			return EFI_INVALID_PARAMETER;
		}

		sz = offset + offsetof(WIN_CERTIFICATE_EFI_PKCS, Hdr.dwLength)
		     + sizeof(sig->Hdr.dwLength);
		if (sz > context->SecDir->Size) {
			perror(L"Certificate size is too large for secruity database");
			return EFI_INVALID_PARAMETER;
		}

		sz = sig->Hdr.dwLength;
		if (sz > context->SecDir->Size - offset) {
			perror(L"Certificate size is too large for secruity database");
			return EFI_INVALID_PARAMETER;
		}

		if (sz < sizeof(sig->Hdr)) {
			perror(L"Certificate size is too small for certificate data");
			return EFI_INVALID_PARAMETER;
		}

		if (sig->Hdr.wCertificateType == WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
			EFI_STATUS efi_status;

			dprint(L"Attempting to verify signature %d:\n", i++);

			efi_status = verify_one_signature(sig, sha256hash, sha1hash);

			/*
			 * If we didn't get EFI_SECURITY_VIOLATION from
			 * checking the hashes above, then any dbx entries are
			 * for a certificate, not this individual binary.
			 *
			 * So don't clobber successes with security violation
			 * here; that just means it isn't a success.
			 */
			if (ret_efi_status != EFI_SUCCESS)
				ret_efi_status = efi_status;
		} else {
			perror(L"Unsupported certificate type %x\n",
				sig->Hdr.wCertificateType);
		}
		offset = ALIGN_VALUE(offset + sz, 8);
	} while (offset < context->SecDir->Size);

	if (ret_efi_status != EFI_SUCCESS) {
		dprint(L"Binary is not authorized\n");
		PrintErrors();
		ClearErrors();
		crypterr(EFI_SECURITY_VIOLATION);
		ret_efi_status = EFI_SECURITY_VIOLATION;
	}
	drain_openssl_errors();
	return ret_efi_status;
}

/*
 * Check that the binary is permitted to load by SBAT.
 */
static EFI_STATUS
verify_buffer_sbat (char *data, int datasize,
		    PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	int i;
	EFI_IMAGE_SECTION_HEADER *Section;
	char *SBATBase = NULL;
	size_t SBATSize = 0;

	Section = context->FirstSection;
	for (i = 0; i < context->NumberOfSections; i++, Section++) {
		if ((uint64_t)(uintptr_t)&Section[1]
		    > (uintptr_t)(uintptr_t)data + datasize) {
			perror(L"Section exceeds bounds of image\n");
			return EFI_UNSUPPORTED;
		}

		if (CompareMem(Section->Name, ".sbat\0\0\0", 8) != 0)
			continue;

		if (SBATBase || SBATSize) {
			perror(L"Image has multiple SBAT sections\n");
			return EFI_UNSUPPORTED;
		}

		if (Section->NumberOfRelocations != 0 ||
		    Section->PointerToRelocations != 0) {
			perror(L"SBAT section has relocations\n");
			return EFI_UNSUPPORTED;
		}

		/* The virtual size corresponds to the size of the SBAT
		 * metadata and isn't necessarily a multiple of the file
		 * alignment. The on-disk size is a multiple of the file
		 * alignment and is zero padded. Make sure that the
		 * on-disk size is at least as large as virtual size,
		 * and ignore the section if it isn't. */
		if (Section->SizeOfRawData &&
		    Section->SizeOfRawData >= Section->Misc.VirtualSize) {
			uint64_t boundary;
			SBATBase = ImageAddress(data, datasize,
						Section->PointerToRawData);
			SBATSize = Section->SizeOfRawData;
			dprint(L"sbat section base:0x%lx size:0x%lx\n",
			       SBATBase, SBATSize);
			if (checked_add((uint64_t)(uintptr_t)SBATBase, SBATSize, &boundary) ||
			    (boundary > (uint64_t)(uintptr_t)data + datasize)) {
				perror(L"Section exceeds bounds of image\n");
				return EFI_UNSUPPORTED;
			}
		}
	}

	return verify_sbat_section(SBATBase, SBATSize);
}

/*
 * Check that the signature is valid and matches the binary and that
 * the binary is permitted to load by SBAT.
 */
EFI_STATUS
verify_buffer (char *data, int datasize,
	       PE_COFF_LOADER_IMAGE_CONTEXT *context,
	       UINT8 *sha256hash, UINT8 *sha1hash,
	       bool parent_verified)
{
	EFI_STATUS efi_status;

	efi_status = verify_buffer_authenticode(data, datasize, context,
						sha256hash, sha1hash,
						parent_verified);
	if (EFI_ERROR(efi_status))
		return efi_status;

	return verify_buffer_sbat(data, datasize, context);
}

/*
 * Protocol entry point. If secure boot is enabled, verify that the provided
 * buffer is signed with a trusted key.
 */
EFI_STATUS
shim_verify(void *buffer, UINT32 size)
{
	EFI_STATUS efi_status = EFI_SUCCESS;
	PE_COFF_LOADER_IMAGE_CONTEXT context;
	UINT8 sha1hash[SHA1_DIGEST_SIZE];
	UINT8 sha256hash[SHA256_DIGEST_SIZE];

	if ((INT32)size < 0)
		return EFI_INVALID_PARAMETER;

	in_protocol = 1;

	efi_status = read_header(buffer, size, &context, true);
	if (EFI_ERROR(efi_status))
		goto done;

	efi_status = generate_hash(buffer, size, &context,
				   sha256hash, sha1hash);
	if (EFI_ERROR(efi_status))
		goto done;

	/* Measure the binary into the TPM */
#ifdef REQUIRE_TPM
	efi_status =
#endif
	tpm_log_pe((EFI_PHYSICAL_ADDRESS)(UINTN)buffer, size, 0, NULL,
		   sha1hash, 4);
#ifdef REQUIRE_TPM
	if (EFI_ERROR(efi_status))
		goto done;
#endif

	if (!secure_mode()) {
		efi_status = EFI_SUCCESS;
		goto done;
	}

	efi_status = verify_buffer(buffer, size,
				   &context, sha256hash, sha1hash,
				   false);
done:
	in_protocol = 0;
	return efi_status;
}

EFI_STATUS
shim_hash(char *data, int datasize, PE_COFF_LOADER_IMAGE_CONTEXT *context,
          UINT8 *sha256hash, UINT8 *sha1hash)
{
	EFI_STATUS efi_status;

	if (datasize < 0)
		return EFI_INVALID_PARAMETER;

	in_protocol = 1;
	efi_status = generate_hash(data, datasize, context,
				   sha256hash, sha1hash);
	in_protocol = 0;

	return efi_status;
}

EFI_STATUS
shim_read_header(void *data, unsigned int datasize,
                 PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	EFI_STATUS efi_status;

	in_protocol = 1;
	efi_status = read_header(data, datasize, context, true);
	in_protocol = 0;

	return efi_status;
}

// vim:fenc=utf-8:tw=75:noet
