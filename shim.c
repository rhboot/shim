// SPDX-License-Identifier: BSD-2-Clause-Patent

/*
 * shim - trivial UEFI first-stage bootloader
 *
 * Copyright Red Hat, Inc
 * Author: Matthew Garrett
 *
 * Significant portions of this code are derived from Tianocore
 * (http://tianocore.sf.net) and are Copyright 2009-2012 Intel
 * Corporation.
 */

#include "shim.h"
#if defined(ENABLE_SHIM_CERT)
#include "shim_cert.h"
#endif /* defined(ENABLE_SHIM_CERT) */

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

#include <stdint.h>

#define OID_EKU_MODSIGN "1.3.6.1.4.1.2312.16.1.2"

static EFI_SYSTEM_TABLE *systab;
static EFI_HANDLE global_image_handle;
static EFI_LOADED_IMAGE *shim_li;
static EFI_LOADED_IMAGE shim_li_bak;

list_t sbat_var;

/*
 * The vendor certificate used for validating the second stage loader
 */
extern struct {
	UINT32 vendor_authorized_size;
	UINT32 vendor_deauthorized_size;
	UINT32 vendor_authorized_offset;
	UINT32 vendor_deauthorized_offset;
} cert_table;

#define EFI_IMAGE_SECURITY_DATABASE_GUID { 0xd719b2cb, 0x3d3a, 0x4596, { 0xa3, 0xbc, 0xda, 0xd0, 0x0e, 0x67, 0x65, 0x6f }}

typedef enum {
	DATA_FOUND,
	DATA_NOT_FOUND,
	VAR_NOT_FOUND
} CHECK_STATUS;

typedef struct {
	UINT32 MokSize;
	UINT8 *Mok;
} MokListNode;

static void
drain_openssl_errors(void)
{
	unsigned long err = -1;
	while (err != 0)
		err = ERR_get_error();
}

static BOOLEAN verify_x509(UINT8 *Cert, UINTN CertSize)
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

static BOOLEAN verify_eku(UINT8 *Cert, UINTN CertSize)
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

static CHECK_STATUS check_db_cert_in_ram(EFI_SIGNATURE_LIST *CertList,
					 UINTN dbsize,
					 WIN_CERTIFICATE_EFI_PKCS *data,
					 UINT8 *hash, CHAR16 *dbname,
					 EFI_GUID guid)
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

static CHECK_STATUS check_db_cert(CHAR16 *dbname, EFI_GUID guid,
				  WIN_CERTIFICATE_EFI_PKCS *data, UINT8 *hash)
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
static CHECK_STATUS check_db_hash_in_ram(EFI_SIGNATURE_LIST *CertList,
					 UINTN dbsize, UINT8 *data,
					 int SignatureSize, EFI_GUID CertType,
					 CHAR16 *dbname, EFI_GUID guid)
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
static CHECK_STATUS check_db_hash(CHAR16 *dbname, EFI_GUID guid, UINT8 *data,
				  int SignatureSize, EFI_GUID CertType)
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
static EFI_STATUS check_denylist (WIN_CERTIFICATE_EFI_PKCS *cert,
				  UINT8 *sha256hash, UINT8 *sha1hash)
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

static void update_verification_method(verification_method_t method)
{
	if (verification_method == VERIFIED_BY_NOTHING)
		verification_method = method;
}

/*
 * Check whether the binary signature or hash are present in db or MokList
 */
static EFI_STATUS check_allowlist (WIN_CERTIFICATE_EFI_PKCS *cert,
				   UINT8 *sha256hash, UINT8 *sha1hash)
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

/*
 * Check whether we're in Secure Boot and user mode
 */
BOOLEAN secure_mode (void)
{
	static int first = 1;
	if (user_insecure_mode)
		return FALSE;

	if (variable_is_secureboot() != 1) {
		if (verbose && !in_protocol && first) {
			CHAR16 *title = L"Secure boot not enabled";
			CHAR16 *message = L"Press any key to continue";
			console_countdown(title, message, 5);
		}
		first = 0;
		return FALSE;
	}

	/* If we /do/ have "SecureBoot", but /don't/ have "SetupMode",
	 * then the implementation is bad, but we assume that secure boot is
	 * enabled according to the status of "SecureBoot".  If we have both
	 * of them, then "SetupMode" may tell us additional data, and we need
	 * to consider it.
	 */
	if (variable_is_setupmode(0) == 1) {
		if (verbose && !in_protocol && first) {
			CHAR16 *title = L"Platform is in setup mode";
			CHAR16 *message = L"Press any key to continue";
			console_countdown(title, message, 5);
		}
		first = 0;
		return FALSE;
	}

	first = 0;
	return TRUE;
}

static EFI_STATUS
verify_one_signature(WIN_CERTIFICATE_EFI_PKCS *sig,
		     UINT8 *sha256hash, UINT8 *sha1hash)
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
EFI_STATUS
verify_buffer_authenticode (char *data, int datasize,
			    PE_COFF_LOADER_IMAGE_CONTEXT *context,
			    UINT8 *sha256hash, UINT8 *sha1hash)
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

	if (context->SecDir->Size >= size) {
		perror(L"Certificate Database size is too large\n");
		return EFI_INVALID_PARAMETER;
	}

	ret_efi_status = EFI_NOT_FOUND;
	do {
		WIN_CERTIFICATE_EFI_PKCS *sig = NULL;
		size_t sz;

		sig = ImageAddress(data, size,
				   context->SecDir->VirtualAddress + offset);
		if (!sig)
			break;

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
EFI_STATUS
verify_buffer_sbat (char *data, int datasize,
		    PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	int i;
	EFI_IMAGE_SECTION_HEADER *Section;
	char *SBATBase = NULL;
	size_t SBATSize = 0;

	Section = context->FirstSection;
	for (i = 0; i < context->NumberOfSections; i++, Section++) {
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
			SBATBase = ImageAddress(data, datasize,
						Section->PointerToRawData);
			SBATSize = Section->SizeOfRawData;
			dprint(L"sbat section base:0x%lx size:0x%lx\n",
			       SBATBase, SBATSize);
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
	       UINT8 *sha256hash, UINT8 *sha1hash)
{
	EFI_STATUS efi_status;

	efi_status = verify_buffer_sbat(data, datasize, context);
	if (EFI_ERROR(efi_status))
		return efi_status;

	return verify_buffer_authenticode(data, datasize, context, sha256hash, sha1hash);
}

static int
is_removable_media_path(EFI_LOADED_IMAGE *li)
{
	unsigned int pathlen = 0;
	CHAR16 *bootpath = NULL;
	int ret = 0;

	bootpath = DevicePathToStr(li->FilePath);

	/* Check the beginning of the string and the end, to avoid
	 * caring about which arch this is. */
	/* I really don't know why, but sometimes bootpath gives us
	 * L"\\EFI\\BOOT\\/BOOTX64.EFI".  So just handle that here...
	 */
	if (StrnCaseCmp(bootpath, L"\\EFI\\BOOT\\BOOT", 14) &&
			StrnCaseCmp(bootpath, L"\\EFI\\BOOT\\/BOOT", 15) &&
			StrnCaseCmp(bootpath, L"EFI\\BOOT\\BOOT", 13) &&
			StrnCaseCmp(bootpath, L"EFI\\BOOT\\/BOOT", 14))
		goto error;

	pathlen = StrLen(bootpath);
	if (pathlen < 5 || StrCaseCmp(bootpath + pathlen - 4, L".EFI"))
		goto error;

	ret = 1;

error:
	if (bootpath)
		FreePool(bootpath);

	return ret;
}

static int
should_use_fallback(EFI_HANDLE image_handle)
{
	EFI_LOADED_IMAGE *li;
	EFI_FILE_IO_INTERFACE *fio = NULL;
	EFI_FILE *vh = NULL;
	EFI_FILE *fh = NULL;
	EFI_STATUS efi_status;
	int ret = 0;

	efi_status = BS->HandleProtocol(image_handle, &EFI_LOADED_IMAGE_GUID,
	                                (void **)&li);
	if (EFI_ERROR(efi_status)) {
		perror(L"Could not get image for boot" EFI_ARCH L".efi: %r\n",
		       efi_status);
		return 0;
	}

	if (!is_removable_media_path(li))
		goto error;

	efi_status = BS->HandleProtocol(li->DeviceHandle, &FileSystemProtocol,
					(void **) &fio);
	if (EFI_ERROR(efi_status)) {
		perror(L"Could not get fio for li->DeviceHandle: %r\n",
		       efi_status);
		goto error;
	}

	efi_status = fio->OpenVolume(fio, &vh);
	if (EFI_ERROR(efi_status)) {
		perror(L"Could not open fio volume: %r\n", efi_status);
		goto error;
	}

	efi_status = vh->Open(vh, &fh, L"\\EFI\\BOOT" FALLBACK,
			      EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(efi_status)) {
		/* Do not print the error here - this is an acceptable case
		 * for removable media, where we genuinely don't want
		 * fallback.efi to exist.
		 * Print(L"Could not open \"\\EFI\\BOOT%s\": %r\n", FALLBACK,
		 *       efi_status);
		 */
		goto error;
	}

	ret = 1;
error:
	if (fh)
		fh->Close(fh);
	if (vh)
		vh->Close(vh);

	return ret;
}
/*
 * Open the second stage bootloader and read it into a buffer
 */
static EFI_STATUS load_image (EFI_LOADED_IMAGE *li, void **data,
			      int *datasize, CHAR16 *PathName)
{
	EFI_STATUS efi_status;
	EFI_HANDLE device;
	EFI_FILE_INFO *fileinfo = NULL;
	EFI_FILE_IO_INTERFACE *drive;
	EFI_FILE *root, *grub;
	UINTN buffersize = sizeof(EFI_FILE_INFO);

	device = li->DeviceHandle;

	dprint(L"attempting to load %s\n", PathName);
	/*
	 * Open the device
	 */
	efi_status = BS->HandleProtocol(device, &EFI_SIMPLE_FILE_SYSTEM_GUID,
					(void **) &drive);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to find fs: %r\n", efi_status);
		goto error;
	}

	efi_status = drive->OpenVolume(drive, &root);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to open fs: %r\n", efi_status);
		goto error;
	}

	/*
	 * And then open the file
	 */
	efi_status = root->Open(root, &grub, PathName, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to open %s - %r\n", PathName, efi_status);
		goto error;
	}

	fileinfo = AllocatePool(buffersize);

	if (!fileinfo) {
		perror(L"Unable to allocate file info buffer\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto error;
	}

	/*
	 * Find out how big the file is in order to allocate the storage
	 * buffer
	 */
	efi_status = grub->GetInfo(grub, &EFI_FILE_INFO_GUID, &buffersize,
				   fileinfo);
	if (efi_status == EFI_BUFFER_TOO_SMALL) {
		FreePool(fileinfo);
		fileinfo = AllocatePool(buffersize);
		if (!fileinfo) {
			perror(L"Unable to allocate file info buffer\n");
			efi_status = EFI_OUT_OF_RESOURCES;
			goto error;
		}
		efi_status = grub->GetInfo(grub, &EFI_FILE_INFO_GUID,
					   &buffersize, fileinfo);
	}

	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to get file info: %r\n", efi_status);
		goto error;
	}

	buffersize = fileinfo->FileSize;
	*data = AllocatePool(buffersize);
	if (!*data) {
		perror(L"Unable to allocate file buffer\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto error;
	}

	/*
	 * Perform the actual read
	 */
	efi_status = grub->Read(grub, &buffersize, *data);
	if (efi_status == EFI_BUFFER_TOO_SMALL) {
		FreePool(*data);
		*data = AllocatePool(buffersize);
		efi_status = grub->Read(grub, &buffersize, *data);
	}
	if (EFI_ERROR(efi_status)) {
		perror(L"Unexpected return from initial read: %r, buffersize %x\n",
		       efi_status, buffersize);
		goto error;
	}

	*datasize = buffersize;

	FreePool(fileinfo);

	return EFI_SUCCESS;
error:
	if (*data) {
		FreePool(*data);
		*data = NULL;
	}

	if (fileinfo)
		FreePool(fileinfo);
	return efi_status;
}

/*
 * Protocol entry point. If secure boot is enabled, verify that the provided
 * buffer is signed with a trusted key.
 */
EFI_STATUS shim_verify (void *buffer, UINT32 size)
{
	EFI_STATUS efi_status = EFI_SUCCESS;
	PE_COFF_LOADER_IMAGE_CONTEXT context;
	UINT8 sha1hash[SHA1_DIGEST_SIZE];
	UINT8 sha256hash[SHA256_DIGEST_SIZE];

	if ((INT32)size < 0)
		return EFI_INVALID_PARAMETER;

	loader_is_participating = 1;
	in_protocol = 1;

	efi_status = read_header(buffer, size, &context);
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
				   &context, sha256hash, sha1hash);
done:
	in_protocol = 0;
	return efi_status;
}

static EFI_STATUS shim_hash (char *data, int datasize,
			     PE_COFF_LOADER_IMAGE_CONTEXT *context,
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

static EFI_STATUS shim_read_header(void *data, unsigned int datasize,
				   PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	EFI_STATUS efi_status;

	in_protocol = 1;
	efi_status = read_header(data, datasize, context);
	in_protocol = 0;

	return efi_status;
}

VOID
restore_loaded_image(VOID)
{
	if (shim_li->FilePath)
		FreePool(shim_li->FilePath);

	/*
	 * Restore our original loaded image values
	 */
	CopyMem(shim_li, &shim_li_bak, sizeof(shim_li_bak));
}

/*
 * Load and run an EFI executable
 */
EFI_STATUS read_image(EFI_HANDLE image_handle, CHAR16 *ImagePath,
		      CHAR16 **PathName, void **data, int *datasize)
{
	EFI_STATUS efi_status;
	void *sourcebuffer = NULL;
	UINT64 sourcesize = 0;

	/*
	 * We need to refer to the loaded image protocol on the running
	 * binary in order to find our path
	 */
	efi_status = BS->HandleProtocol(image_handle, &EFI_LOADED_IMAGE_GUID,
					(void **)&shim_li);
	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to init protocol\n");
		return efi_status;
	}

	/*
	 * Build a new path from the existing one plus the executable name
	 */
	efi_status = generate_path_from_image_path(shim_li, ImagePath, PathName);
	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to generate path %s: %r\n", ImagePath,
		       efi_status);
		return efi_status;
	}

	if (findNetboot(shim_li->DeviceHandle)) {
		efi_status = parseNetbootinfo(image_handle);
		if (EFI_ERROR(efi_status)) {
			perror(L"Netboot parsing failed: %r\n", efi_status);
			return EFI_PROTOCOL_ERROR;
		}
		efi_status = FetchNetbootimage(image_handle, &sourcebuffer,
					       &sourcesize);
		if (EFI_ERROR(efi_status)) {
			perror(L"Unable to fetch TFTP image: %r\n",
			       efi_status);
			return efi_status;
		}
		*data = sourcebuffer;
		*datasize = sourcesize;
	} else if (find_httpboot(shim_li->DeviceHandle)) {
		efi_status = httpboot_fetch_buffer (image_handle,
						    &sourcebuffer,
						    &sourcesize);
		if (EFI_ERROR(efi_status)) {
			perror(L"Unable to fetch HTTP image: %r\n",
			       efi_status);
			return efi_status;
		}
		*data = sourcebuffer;
		*datasize = sourcesize;
	} else {
		/*
		 * Read the new executable off disk
		 */
		efi_status = load_image(shim_li, data, datasize, *PathName);
		if (EFI_ERROR(efi_status)) {
			perror(L"Failed to load image %s: %r\n",
			       PathName, efi_status);
			PrintErrors();
			ClearErrors();
			return efi_status;
		}
	}

	if (*datasize < 0)
		efi_status = EFI_INVALID_PARAMETER;

	return efi_status;
}

/*
 * Load and run an EFI executable
 */
EFI_STATUS start_image(EFI_HANDLE image_handle, CHAR16 *ImagePath)
{
	EFI_STATUS efi_status;
	EFI_IMAGE_ENTRY_POINT entry_point;
	EFI_PHYSICAL_ADDRESS alloc_address;
	UINTN alloc_pages;
	CHAR16 *PathName = NULL;
	void *data = NULL;
	int datasize = 0;

	efi_status = read_image(image_handle, ImagePath, &PathName, &data,
				&datasize);
	if (EFI_ERROR(efi_status))
		goto done;

	/*
	 * We need to modify the loaded image protocol entry before running
	 * the new binary, so back it up
	 */
	CopyMem(&shim_li_bak, shim_li, sizeof(shim_li_bak));

	/*
	 * Update the loaded image with the second stage loader file path
	 */
	shim_li->FilePath = FileDevicePath(NULL, PathName);
	if (!shim_li->FilePath) {
		perror(L"Unable to update loaded image file path\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto restore;
	}

	/*
	 * Verify and, if appropriate, relocate and execute the executable
	 */
	efi_status = handle_image(data, datasize, shim_li, &entry_point,
				  &alloc_address, &alloc_pages);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to load image: %r\n", efi_status);
		PrintErrors();
		ClearErrors();
		goto restore;
	}

	loader_is_participating = 0;

	/*
	 * The binary is trusted and relocated. Run it
	 */
	efi_status = entry_point(image_handle, systab);

restore:
	restore_loaded_image();
done:
	if (PathName)
		FreePool(PathName);

	if (data)
		FreePool(data);

	return efi_status;
}

/*
 * Load and run grub. If that fails because grub isn't trusted, load and
 * run MokManager.
 */
EFI_STATUS init_grub(EFI_HANDLE image_handle)
{
	EFI_STATUS efi_status;
	int use_fb = should_use_fallback(image_handle);

	efi_status = start_image(image_handle, use_fb ? FALLBACK :second_stage);
	if (efi_status == EFI_SECURITY_VIOLATION ||
	    efi_status == EFI_ACCESS_DENIED) {
		efi_status = start_image(image_handle, MOK_MANAGER);
		if (EFI_ERROR(efi_status)) {
			console_print(L"start_image() returned %r\n", efi_status);
			msleep(2000000);
			return efi_status;
		}

		efi_status = start_image(image_handle,
					 use_fb ? FALLBACK : second_stage);
	}

	// If the filename is invalid, or the file does not exist,
	// just fallback to the default loader.
	if (!use_fb && (efi_status == EFI_INVALID_PARAMETER ||
	                efi_status == EFI_NOT_FOUND)) {
		console_print(
			L"start_image() returned %r, falling back to default loader\n",
			efi_status);
		msleep(2000000);
		load_options = NULL;
		load_options_size = 0;
		efi_status = start_image(image_handle, DEFAULT_LOADER);
	}

	if (EFI_ERROR(efi_status)) {
		console_print(L"start_image() returned %r\n", efi_status);
		msleep(2000000);
	}

	return efi_status;
}

/*
 * Check the load options to specify the second stage loader
 */
EFI_STATUS set_second_stage (EFI_HANDLE image_handle)
{
	EFI_STATUS efi_status;
	EFI_LOADED_IMAGE *li = NULL;

	second_stage = DEFAULT_LOADER;
	load_options = NULL;
	load_options_size = 0;

	efi_status = BS->HandleProtocol(image_handle, &LoadedImageProtocol,
					(void **) &li);
	if (EFI_ERROR(efi_status)) {
		perror (L"Failed to get load options: %r\n", efi_status);
		return efi_status;
	}

#if defined(DISABLE_REMOVABLE_LOAD_OPTIONS)
	/*
	 * boot services build very strange load options, and we might misparse them,
	 * causing boot failures on removable media.
	 */
	if (is_removable_media_path(li)) {
		dprint("Invoked from removable media path, ignoring boot options");
		return EFI_SUCCESS;
	}
#endif

	efi_status = parse_load_options(li);
	if (EFI_ERROR(efi_status)) {
		perror (L"Failed to get load options: %r\n", efi_status);
		return efi_status;
	}

	return EFI_SUCCESS;
}

static void *
ossl_malloc(size_t num)
{
	return AllocatePool(num);
}

static void
ossl_free(void *addr)
{
	FreePool(addr);
}

static void
init_openssl(void)
{
	CRYPTO_set_mem_functions(ossl_malloc, NULL, ossl_free);
	OPENSSL_init();
	CRYPTO_set_mem_functions(ossl_malloc, NULL, ossl_free);
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

static SHIM_LOCK shim_lock_interface;
static EFI_HANDLE shim_lock_handle;

EFI_STATUS
install_shim_protocols(void)
{
	SHIM_LOCK *shim_lock;
	EFI_STATUS efi_status;

	/*
	 * Did another instance of shim earlier already install the
	 * protocol? If so, get rid of it.
	 *
	 * We have to uninstall shim's protocol here, because if we're
	 * On the fallback.efi path, then our call pathway is:
	 *
	 * shim->fallback->shim->grub
	 * ^               ^      ^
	 * |               |      \- gets protocol #0
	 * |               \- installs its protocol (#1)
	 * \- installs its protocol (#0)
	 * and if we haven't removed this, then grub will get the *first*
	 * shim's protocol, but it'll get the second shim's systab
	 * replacements.  So even though it will participate and verify
	 * the kernel, the systab never finds out.
	 */
	efi_status = LibLocateProtocol(&SHIM_LOCK_GUID, (VOID **)&shim_lock);
	if (!EFI_ERROR(efi_status))
		uninstall_shim_protocols();

	/*
	 * Install the protocol
	 */
	efi_status = BS->InstallProtocolInterface(&shim_lock_handle,
						  &SHIM_LOCK_GUID,
						  EFI_NATIVE_INTERFACE,
						  &shim_lock_interface);
	if (EFI_ERROR(efi_status)) {
		console_error(L"Could not install security protocol",
			      efi_status);
		return efi_status;
	}

	if (!secure_mode())
		return EFI_SUCCESS;

#if defined(OVERRIDE_SECURITY_POLICY)
	/*
	 * Install the security protocol hook
	 */
	security_policy_install(shim_verify);
#endif

	return EFI_SUCCESS;
}

void
uninstall_shim_protocols(void)
{
	/*
	 * If we're back here then clean everything up before exiting
	 */
	BS->UninstallProtocolInterface(shim_lock_handle, &SHIM_LOCK_GUID,
				       &shim_lock_interface);

	if (!secure_mode())
		return;

#if defined(OVERRIDE_SECURITY_POLICY)
	/*
	 * Clean up the security protocol hook
	 */
	security_policy_uninstall();
#endif
}

EFI_STATUS
load_cert_file(EFI_HANDLE image_handle, CHAR16 *filename, CHAR16 *PathName)
{
	EFI_STATUS efi_status;
	PE_COFF_LOADER_IMAGE_CONTEXT context;
	EFI_IMAGE_SECTION_HEADER *Section;
	EFI_SIGNATURE_LIST *certlist;
	void *pointer;
	UINT32 original;
	int datasize = 0;
	void *data = NULL;
	int i;

	efi_status = read_image(image_handle, filename, &PathName,
				&data, &datasize);
	if (EFI_ERROR(efi_status))
		return efi_status;

	efi_status = verify_image(data, datasize, shim_li, &context);
	if (EFI_ERROR(efi_status))
		return efi_status;

	Section = context.FirstSection;
	for (i = 0; i < context.NumberOfSections; i++, Section++) {
		if (CompareMem(Section->Name, ".db\0\0\0\0\0", 8) == 0) {
			original = user_cert_size;
			if (Section->SizeOfRawData < sizeof(EFI_SIGNATURE_LIST)) {
				continue;
			}
			pointer = ImageAddress(data, datasize,
					       Section->PointerToRawData);
			if (!pointer) {
				continue;
			}
			certlist = pointer;
			user_cert_size += certlist->SignatureListSize;;
			user_cert = ReallocatePool(user_cert, original,
						   user_cert_size);
			CopyMem(user_cert + original, pointer,
			        certlist->SignatureListSize);
		}
	}
	FreePool(data);
	return EFI_SUCCESS;
}

/* Read additional certificates from files (after verifying signatures) */
EFI_STATUS
load_certs(EFI_HANDLE image_handle)
{
	EFI_STATUS efi_status;
	EFI_LOADED_IMAGE *li = NULL;
	CHAR16 *PathName = NULL;
	EFI_FILE *root, *dir;
	EFI_FILE_INFO *info;
	EFI_HANDLE device;
	EFI_FILE_IO_INTERFACE *drive;
	UINTN buffersize = 0;
	void *buffer = NULL;

	efi_status = gBS->HandleProtocol(image_handle, &EFI_LOADED_IMAGE_GUID,
					 (void **)&li);
	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to init protocol\n");
		return efi_status;
	}

	efi_status = generate_path_from_image_path(li, L"", &PathName);
	if (EFI_ERROR(efi_status))
		goto done;

	device = li->DeviceHandle;
	efi_status = gBS->HandleProtocol(device, &EFI_SIMPLE_FILE_SYSTEM_GUID,
					 (void **)&drive);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to find fs: %r\n", efi_status);
		goto done;
	}

	efi_status = drive->OpenVolume(drive, &root);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to open fs: %r\n", efi_status);
		goto done;
	}

	efi_status = root->Open(root, &dir, PathName, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to open %s - %r\n", PathName, efi_status);
		goto done;
	}

	while (1) {
		int old = buffersize;
		efi_status = dir->Read(dir, &buffersize, buffer);
		if (efi_status == EFI_BUFFER_TOO_SMALL) {
			buffer = ReallocatePool(buffer, old, buffersize);
			continue;
		} else if (EFI_ERROR(efi_status)) {
			perror(L"Failed to read directory %s - %r\n", PathName,
			       efi_status);
			goto done;
		}

		info = (EFI_FILE_INFO *)buffer;
		if (buffersize == 0 || !info)
			goto done;

		if (StrnCaseCmp(info->FileName, L"shim_certificate", 16) == 0) {
			load_cert_file(image_handle, info->FileName, PathName);
		}
	}
done:
	FreePool(buffer);
	FreePool(PathName);
	return efi_status;
}

EFI_STATUS
shim_init(void)
{
	EFI_STATUS efi_status;

	dprint(L"%a", shim_version);

	/* Set the second stage loader */
	efi_status = set_second_stage(global_image_handle);
	if (EFI_ERROR(efi_status)) {
		perror(L"set_second_stage() failed: %r\n", efi_status);
		return efi_status;
	}

	if (secure_mode()) {
		if (vendor_authorized_size || vendor_deauthorized_size) {
			/*
			 * If shim includes its own certificates then ensure
			 * that anything it boots has performed some
			 * validation of the next image.
			 */
			hook_system_services(systab);
			loader_is_participating = 0;
		}

	}

	hook_exit(systab);

	efi_status = install_shim_protocols();
	if (EFI_ERROR(efi_status))
		perror(L"install_shim_protocols() failed: %r\n", efi_status);

	return efi_status;
}

void
shim_fini(void)
{
	if (secure_mode())
		cleanup_sbat_var(&sbat_var);

	/*
	 * Remove our protocols
	 */
	uninstall_shim_protocols();

	if (secure_mode()) {

		/*
		 * Remove our hooks from system services.
		 */
		unhook_system_services();
	}

	unhook_exit();

	console_fini();
}

extern EFI_STATUS
efi_main(EFI_HANDLE passed_image_handle, EFI_SYSTEM_TABLE *passed_systab);

static void
__attribute__((__optimize__("0")))
debug_hook(void)
{
	UINT8 *data = NULL;
	UINTN dataSize = 0;
	EFI_STATUS efi_status;
	register volatile UINTN x = 0;
	extern char _text, _data;

	if (x)
		return;

	efi_status = get_variable(DEBUG_VAR_NAME, &data, &dataSize,
				  SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
		return;
	}

	FreePool(data);

	console_print(L"add-symbol-file "DEBUGDIR
		      L"shim" EFI_ARCH L".efi.debug 0x%08x -s .data 0x%08x\n",
		      &_text, &_data);

	console_print(L"Pausing for debugger attachment.\n");
	console_print(L"To disable this, remove the EFI variable %s-%g .\n",
		      DEBUG_VAR_NAME, &SHIM_LOCK_GUID);
	x = 1;
	while (x++) {
		/* Make this so it can't /totally/ DoS us. */
#if defined(__x86_64__) || defined(__i386__) || defined(__i686__)
		if (x > 4294967294ULL)
			break;
#elif defined(__aarch64__)
		if (x > 1000)
			break;
#else
		if (x > 12000)
			break;
#endif
		wait_for_debug();
	}
	x = 1;
}

typedef enum {
	COLD_RESET,
	EXIT_FAILURE,
	EXIT_SUCCESS,	// keep this one last
} devel_egress_action;

void
devel_egress(devel_egress_action action UNUSED)
{
#ifdef ENABLE_SHIM_DEVEL
	char *reasons[] = {
		[COLD_RESET] = "reset",
		[EXIT_FAILURE] = "exit",
	};
	if (action == EXIT_SUCCESS)
		return;

	console_print(L"Waiting to %a...", reasons[action]);
	for (size_t sleepcount = 0; sleepcount < 10; sleepcount++) {
		console_print(L"%d...", 10 - sleepcount);
		msleep(1000000);
	}
	console_print(L"\ndoing %a\n", action);

	if (action == COLD_RESET)
		RT->ResetSystem(EfiResetCold, EFI_SECURITY_VIOLATION, 0, NULL);
#endif
}

EFI_STATUS
efi_main (EFI_HANDLE passed_image_handle, EFI_SYSTEM_TABLE *passed_systab)
{
	EFI_STATUS efi_status;
	EFI_HANDLE image_handle;

	verification_method = VERIFIED_BY_NOTHING;

	vendor_authorized_size = cert_table.vendor_authorized_size;
	vendor_authorized = (UINT8 *)&cert_table + cert_table.vendor_authorized_offset;

	vendor_deauthorized_size = cert_table.vendor_deauthorized_size;
	vendor_deauthorized = (UINT8 *)&cert_table + cert_table.vendor_deauthorized_offset;

#if defined(ENABLE_SHIM_CERT)
	build_cert_size = sizeof(shim_cert);
	build_cert = shim_cert;
#endif /* defined(ENABLE_SHIM_CERT) */

	CHAR16 *msgs[] = {
		L"import_mok_state() failed",
		L"shim_init() failed",
		L"import of SBAT data failed",
		L"SBAT self-check failed",
		SBAT_VAR_NAME L" UEFI variable setting failed",
		NULL
	};
	enum {
		IMPORT_MOK_STATE,
		SHIM_INIT,
		IMPORT_SBAT,
		SBAT_SELF_CHECK,
		SET_SBAT,
	} msg = IMPORT_MOK_STATE;

	/*
	 * Set up the shim lock protocol so that grub and MokManager can
	 * call back in and use shim functions
	 */
	shim_lock_interface.Verify = shim_verify;
	shim_lock_interface.Hash = shim_hash;
	shim_lock_interface.Context = shim_read_header;

	systab = passed_systab;
	image_handle = global_image_handle = passed_image_handle;

	/*
	 * Ensure that gnu-efi functions are available
	 */
	InitializeLib(image_handle, systab);
	setup_verbosity();

	dprint(L"vendor_authorized:0x%08lx vendor_authorized_size:%lu\n",
	       vendor_authorized, vendor_authorized_size);
	dprint(L"vendor_deauthorized:0x%08lx vendor_deauthorized_size:%lu\n",
	       vendor_deauthorized, vendor_deauthorized_size);

	/*
	 * if SHIM_DEBUG is set, wait for a debugger to attach.
	 */
	debug_hook();

	efi_status = set_sbat_uefi_variable();
	if (EFI_ERROR(efi_status) && secure_mode()) {
		perror(L"%s variable initialization failed\n", SBAT_VAR_NAME);
		msg = SET_SBAT;
		goto die;
	} else if (EFI_ERROR(efi_status)) {
		dprint(L"%s variable initialization failed: %r\n",
		       SBAT_VAR_NAME, efi_status);
	}

	if (secure_mode()) {
		char *sbat_start = (char *)&_sbat;
		char *sbat_end = (char *)&_esbat;

		INIT_LIST_HEAD(&sbat_var);
		efi_status = parse_sbat_var(&sbat_var);
		if (EFI_ERROR(efi_status)) {
			perror(L"Parsing %s variable failed: %r\n",
				SBAT_VAR_NAME, efi_status);
			msg = IMPORT_SBAT;
			goto die;
		}

		efi_status = verify_sbat_section(sbat_start, sbat_end - sbat_start - 1);
		if (EFI_ERROR(efi_status)) {
			perror(L"Verifiying shim SBAT data failed: %r\n",
			       efi_status);
			msg = SBAT_SELF_CHECK;
			goto die;
		}
		dprint(L"SBAT self-check succeeded\n");
	}

	init_openssl();

	if (secure_mode()) {
		efi_status = load_certs(global_image_handle);
		if (EFI_ERROR(efi_status)) {
			LogError(L"Failed to load addon certificates\n");
		}
	}

	/*
	 * Before we do anything else, validate our non-volatile,
	 * boot-services-only state variables are what we think they are.
	 */
	efi_status = import_mok_state(image_handle);
	if (!secure_mode() &&
	    (efi_status == EFI_INVALID_PARAMETER ||
	     efi_status == EFI_OUT_OF_RESOURCES)) {
		/*
		 * Make copy failures fatal only if secure_mode is enabled, or
		 * the error was anything else than EFI_INVALID_PARAMETER or
		 * EFI_OUT_OF_RESOURCES.
		 * There are non-secureboot firmware implementations that don't
		 * reserve enough EFI variable memory to fit the variable.
		 */
		console_print(L"Importing MOK states has failed: %s: %r\n",
			      msgs[msg], efi_status);
		console_print(L"Continuing boot since secure mode is disabled");
	} else if (EFI_ERROR(efi_status)) {
die:
		console_print(L"Something has gone seriously wrong: %s: %r\n",
			      msgs[msg], efi_status);
#if defined(ENABLE_SHIM_DEVEL)
		devel_egress(COLD_RESET);
#else
		msleep(5000000);
		RT->ResetSystem(EfiResetShutdown, EFI_SECURITY_VIOLATION,
				0, NULL);
#endif
	}

	efi_status = shim_init();
	if (EFI_ERROR(efi_status)) {
		msg = SHIM_INIT;
		goto die;
	}

	/*
	 * Tell the user that we're in insecure mode if necessary
	 */
	if (user_insecure_mode) {
		console_print(L"Booting in insecure mode\n");
		msleep(2000000);
	}

	/*
	 * Hand over control to the second stage bootloader
	 */
	efi_status = init_grub(image_handle);

	shim_fini();
	devel_egress(EFI_ERROR(efi_status) ? EXIT_FAILURE : EXIT_SUCCESS);
	return efi_status;
}
