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

static CHAR16 *second_stage;
void *load_options;
UINT32 load_options_size;

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

UINT32 vendor_authorized_size = 0;
UINT8 *vendor_authorized = NULL;

UINT32 vendor_deauthorized_size = 0;
UINT8 *vendor_deauthorized = NULL;

#if defined(ENABLE_SHIM_CERT)
UINT32 build_cert_size;
UINT8 *build_cert;
#endif /* defined(ENABLE_SHIM_CERT) */

/*
 * indicator of how an image has been verified
 */
verification_method_t verification_method;
int loader_is_participating;

#define EFI_IMAGE_SECURITY_DATABASE_GUID { 0xd719b2cb, 0x3d3a, 0x4596, { 0xa3, 0xbc, 0xda, 0xd0, 0x0e, 0x67, 0x65, 0x6f }}

UINT8 user_insecure_mode;
UINT8 ignore_db;

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

	if (check_db_hash(L"MokList", SHIM_LOCK_GUID, sha256hash,
			  SHA256_DIGEST_SIZE, EFI_CERT_SHA256_GUID)
				== DATA_FOUND) {
		verification_method = VERIFIED_BY_HASH;
		update_verification_method(VERIFIED_BY_HASH);
		return EFI_SUCCESS;
	} else {
		LogError(L"check_db_hash(MokList, sha256hash) != DATA_FOUND\n");
	}
	if (cert && check_db_cert(L"MokList", SHIM_LOCK_GUID, cert, sha256hash)
			== DATA_FOUND) {
		verification_method = VERIFIED_BY_CERT;
		update_verification_method(VERIFIED_BY_CERT);
		return EFI_SUCCESS;
	} else if (cert) {
		LogError(L"check_db_cert(MokList, sha256hash) != DATA_FOUND\n");
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
		if (verbose && !in_protocol && first)
			console_notify(L"Secure boot not enabled");
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
		if (verbose && !in_protocol && first)
			console_notify(L"Platform is in setup mode");
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
verify_buffer (char *data, int datasize,
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

static int
should_use_fallback(EFI_HANDLE image_handle)
{
	EFI_LOADED_IMAGE *li;
	unsigned int pathlen = 0;
	CHAR16 *bootpath = NULL;
	EFI_FILE_IO_INTERFACE *fio = NULL;
	EFI_FILE *vh = NULL;
	EFI_FILE *fh = NULL;
	EFI_STATUS efi_status;
	int ret = 0;

	efi_status = gBS->HandleProtocol(image_handle, &EFI_LOADED_IMAGE_GUID,
					 (void **)&li);
	if (EFI_ERROR(efi_status)) {
		perror(L"Could not get image for bootx64.efi: %r\n",
		       efi_status);
		return 0;
	}

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

	efi_status = gBS->HandleProtocol(li->DeviceHandle, &FileSystemProtocol,
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
	if (bootpath)
		FreePool(bootpath);

	return ret;
}

/*
 * Generate the path of an executable given shim's path and the name
 * of the executable
 */
static EFI_STATUS generate_path_from_image_path(EFI_LOADED_IMAGE *li,
						CHAR16 *ImagePath,
						CHAR16 **PathName)
{
	EFI_DEVICE_PATH *devpath;
	unsigned int i;
	int j, last = -1;
	unsigned int pathlen = 0;
	EFI_STATUS efi_status = EFI_SUCCESS;
	CHAR16 *bootpath;

	/*
	 * Suuuuper lazy technique here, but check and see if this is a full
	 * path to something on the ESP.  Backwards compatibility demands
	 * that we don't just use \\, because we (not particularly brightly)
	 * used to require that the relative file path started with that.
	 *
	 * If it is a full path, don't try to merge it with the directory
	 * from our Loaded Image handle.
	 */
	if (StrSize(ImagePath) > 5 && StrnCmp(ImagePath, L"\\EFI\\", 5) == 0) {
		*PathName = StrDuplicate(ImagePath);
		if (!*PathName) {
			perror(L"Failed to allocate path buffer\n");
			return EFI_OUT_OF_RESOURCES;
		}
		return EFI_SUCCESS;
	}

	devpath = li->FilePath;

	bootpath = DevicePathToStr(devpath);

	pathlen = StrLen(bootpath);

	/*
	 * DevicePathToStr() concatenates two nodes with '/'.
	 * Convert '/' to '\\'.
	 */
	for (i = 0; i < pathlen; i++) {
		if (bootpath[i] == '/')
			bootpath[i] = '\\';
	}

	for (i=pathlen; i>0; i--) {
		if (bootpath[i] == '\\' && bootpath[i-1] == '\\')
			bootpath[i] = '/';
		else if (last == -1 && bootpath[i] == '\\')
			last = i;
	}

	if (last == -1 && bootpath[0] == '\\')
		last = 0;
	bootpath[last+1] = '\0';

	if (last > 0) {
		for (i = 0, j = 0; bootpath[i] != '\0'; i++) {
			if (bootpath[i] != '/') {
				bootpath[j] = bootpath[i];
				j++;
			}
		}
		bootpath[j] = '\0';
	}

	for (i = 0, last = 0; i < StrLen(ImagePath); i++)
		if (ImagePath[i] == '\\')
			last = i + 1;

	ImagePath = ImagePath + last;
	*PathName = AllocatePool(StrSize(bootpath) + StrSize(ImagePath));

	if (!*PathName) {
		perror(L"Failed to allocate path buffer\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto error;
	}

	*PathName[0] = '\0';
	if (StrnCaseCmp(bootpath, ImagePath, StrLen(bootpath)))
		StrCat(*PathName, bootpath);
	StrCat(*PathName, ImagePath);

error:
	FreePool(bootpath);

	return efi_status;
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
	efi_status = gBS->HandleProtocol(device, &EFI_SIMPLE_FILE_SYSTEM_GUID,
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
EFI_STATUS start_image(EFI_HANDLE image_handle, CHAR16 *ImagePath)
{
	EFI_STATUS efi_status;
	EFI_IMAGE_ENTRY_POINT entry_point;
	EFI_PHYSICAL_ADDRESS alloc_address;
	UINTN alloc_pages;
	CHAR16 *PathName = NULL;
	void *sourcebuffer = NULL;
	UINT64 sourcesize = 0;
	void *data = NULL;
	int datasize = 0;

	/*
	 * We need to refer to the loaded image protocol on the running
	 * binary in order to find our path
	 */
	efi_status = gBS->HandleProtocol(image_handle, &EFI_LOADED_IMAGE_GUID,
					 (void **)&shim_li);
	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to init protocol\n");
		return efi_status;
	}

	/*
	 * Build a new path from the existing one plus the executable name
	 */
	efi_status = generate_path_from_image_path(shim_li, ImagePath, &PathName);
	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to generate path %s: %r\n", ImagePath,
		       efi_status);
		goto done;
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
		data = sourcebuffer;
		datasize = sourcesize;
	} else if (find_httpboot(shim_li->DeviceHandle)) {
		efi_status = httpboot_fetch_buffer (image_handle,
						    &sourcebuffer,
						    &sourcesize);
		if (EFI_ERROR(efi_status)) {
			perror(L"Unable to fetch HTTP image: %r\n",
			       efi_status);
			return efi_status;
		}
		data = sourcebuffer;
		datasize = sourcesize;
	} else {
		/*
		 * Read the new executable off disk
		 */
		efi_status = load_image(shim_li, &data, &datasize, PathName);
		if (EFI_ERROR(efi_status)) {
			perror(L"Failed to load image %s: %r\n",
			       PathName, efi_status);
			PrintErrors();
			ClearErrors();
			goto done;
		}
	}

	if (datasize < 0) {
		efi_status = EFI_INVALID_PARAMETER;
		goto done;
	}

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

	if (EFI_ERROR(efi_status)) {
		console_print(L"start_image() returned %r\n", efi_status);
		msleep(2000000);
	}

	return efi_status;
}

static inline EFI_STATUS
get_load_option_optional_data(UINT8 *data, UINTN data_size,
			      UINT8 **od, UINTN *ods)
{
	/*
	 * If it's not at least Attributes + FilePathListLength +
	 * Description=L"" + 0x7fff0400 (EndEntrireDevicePath), it can't
	 * be valid.
	 */
	if (data_size < (sizeof(UINT32) + sizeof(UINT16) + 2 + 4))
		return EFI_INVALID_PARAMETER;

	UINT8 *cur = data + sizeof(UINT32);
	UINT16 fplistlen = *(UINT16 *)cur;
	/*
	 * If there's not enough space for the file path list and the
	 * smallest possible description (L""), it's not valid.
	 */
	if (fplistlen > data_size - (sizeof(UINT32) + 2 + 4))
		return EFI_INVALID_PARAMETER;

	cur += sizeof(UINT16);
	UINTN limit = data_size - (cur - data) - fplistlen;
	UINTN i;
	for (i = 0; i < limit ; i++) {
		/* If the description isn't valid UCS2-LE, it's not valid. */
		if (i % 2 != 0) {
			if (cur[i] != 0)
				return EFI_INVALID_PARAMETER;
		} else if (cur[i] == 0) {
			/* we've found the end */
			i++;
			if (i >= limit || cur[i] != 0)
				return EFI_INVALID_PARAMETER;
			break;
		}
	}
	i++;
	if (i > limit)
		return EFI_INVALID_PARAMETER;

	/*
	 * If i is limit, we know the rest of this is the FilePathList and
	 * there's no optional data.  So just bail now.
	 */
	if (i == limit) {
		*od = NULL;
		*ods = 0;
		return EFI_SUCCESS;
	}

	cur += i;
	limit -= i;
	limit += fplistlen;
	i = 0;
	while (limit - i >= 4) {
		struct {
			UINT8 type;
			UINT8 subtype;
			UINT16 len;
		} dp = {
			.type = cur[i],
			.subtype = cur[i+1],
			/*
			 * it's a little endian UINT16, but we're not
			 * guaranteed alignment is sane, so we can't just
			 * typecast it directly.
			 */
			.len = (cur[i+3] << 8) | cur[i+2],
		};

		/*
		 * We haven't found an EndEntire, so this has to be a valid
		 * EFI_DEVICE_PATH in order for the data to be valid.  That
		 * means it has to fit, and it can't be smaller than 4 bytes.
		 */
		if (dp.len < 4 || dp.len > limit)
			return EFI_INVALID_PARAMETER;

		/*
		 * see if this is an EndEntire node...
		 */
		if (dp.type == 0x7f && dp.subtype == 0xff) {
			/*
			 * if we've found the EndEntire node, it must be 4
			 * bytes
			 */
			if (dp.len != 4)
				return EFI_INVALID_PARAMETER;

			i += dp.len;
			break;
		}

		/*
		 * It's just some random DP node; skip it.
		 */
		i += dp.len;
	}
	if (i != fplistlen)
		return EFI_INVALID_PARAMETER;

	/*
	 * if there's any space left, it's "optional data"
	 */
	*od = cur + i;
	*ods = limit - i;
	return EFI_SUCCESS;
}

static int is_our_path(EFI_LOADED_IMAGE *li, CHAR16 *path)
{
	CHAR16 *dppath = NULL;
	CHAR16 *PathName = NULL;
	EFI_STATUS efi_status;
	int ret = 1;

	dppath = DevicePathToStr(li->FilePath);
	if (!dppath)
		return 0;

	efi_status = generate_path_from_image_path(li, path, &PathName);
	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to generate path %s: %r\n", path,
		       efi_status);
		goto done;
	}

	dprint(L"dppath: %s\n", dppath);
	dprint(L"path:   %s\n", path);
	if (StrnCaseCmp(dppath, PathName, StrLen(dppath)))
		ret = 0;

done:
	FreePool(dppath);
	FreePool(PathName);
	return ret;
}

/*
 * Check the load options to specify the second stage loader
 */
EFI_STATUS set_second_stage (EFI_HANDLE image_handle)
{
	EFI_STATUS efi_status;
	EFI_LOADED_IMAGE *li = NULL;
	CHAR16 *start = NULL;
	UINTN remaining_size = 0;
	CHAR16 *loader_str = NULL;
	UINTN loader_len = 0;
	unsigned int i;
	UINTN second_stage_len;

	second_stage_len = (StrLen(DEFAULT_LOADER) + 1) * sizeof(CHAR16);
	second_stage = AllocatePool(second_stage_len);
	if (!second_stage) {
		perror(L"Could not allocate %lu bytes\n", second_stage_len);
		return EFI_OUT_OF_RESOURCES;
	}
	StrCpy(second_stage, DEFAULT_LOADER);
	load_options = NULL;
	load_options_size = 0;

	efi_status = gBS->HandleProtocol(image_handle, &LoadedImageProtocol,
					 (void **) &li);
	if (EFI_ERROR(efi_status)) {
		perror (L"Failed to get load options: %r\n", efi_status);
		return efi_status;
	}

	/* Sanity check since we make several assumptions about the length */
	if (li->LoadOptionsSize % 2 != 0)
		return EFI_INVALID_PARAMETER;

	/* So, load options are a giant pain in the ass.  If we're invoked
	 * from the EFI shell, we get something like this:

00000000  5c 00 45 00 36 00 49 00  5c 00 66 00 65 00 64 00  |\.E.F.I.\.f.e.d.|
00000010  6f 00 72 00 61 00 5c 00  73 00 68 00 69 00 6d 00  |o.r.a.\.s.h.i.m.|
00000020  78 00 36 00 34 00 2e 00  64 00 66 00 69 00 20 00  |x.6.4...e.f.i. .|
00000030  5c 00 45 00 46 00 49 00  5c 00 66 00 65 00 64 00  |\.E.F.I.\.f.e.d.|
00000040  6f 00 72 00 61 00 5c 00  66 00 77 00 75 00 70 00  |o.r.a.\.f.w.u.p.|
00000050  64 00 61 00 74 00 65 00  2e 00 65 00 66 00 20 00  |d.a.t.e.e.f.i. .|
00000060  00 00 66 00 73 00 30 00  3a 00 5c 00 00 00        |..f.s.0.:.\...|

	*
	* which is just some paths rammed together separated by a UCS-2 NUL.
	* But if we're invoked from BDS, we get something more like:
	*

00000000  01 00 00 00 62 00 4c 00  69 00 6e 00 75 00 78 00  |....b.L.i.n.u.x.|
00000010  20 00 46 00 69 00 72 00  6d 00 77 00 61 00 72 00  | .F.i.r.m.w.a.r.|
00000020  65 00 20 00 55 00 70 00  64 00 61 00 74 00 65 00  |e. .U.p.d.a.t.e.|
00000030  72 00 00 00 40 01 2a 00  01 00 00 00 00 08 00 00  |r.....*.........|
00000040  00 00 00 00 00 40 06 00  00 00 00 00 1a 9e 55 bf  |.....@........U.|
00000050  04 57 f2 4f b4 4a ed 26  4a 40 6a 94 02 02 04 04  |.W.O.:.&J@j.....|
00000060  34 00 5c 00 45 00 46 00  49 00 5c 00 66 00 65 00  |4.\.E.F.I.f.e.d.|
00000070  64 00 6f 00 72 00 61 00  5c 00 73 00 68 00 69 00  |o.r.a.\.s.h.i.m.|
00000080  6d 00 78 00 36 00 34 00  2e 00 65 00 66 00 69 00  |x.6.4...e.f.i...|
00000090  00 00 7f ff 40 00 20 00  5c 00 66 00 77 00 75 00  |...... .\.f.w.u.|
000000a0  70 00 78 00 36 00 34 00  2e 00 65 00 66 00 69 00  |p.x.6.4...e.f.i.|
000000b0  00 00                                             |..|

	*
	* which is clearly an EFI_LOAD_OPTION filled in halfway reasonably.
	* In short, the UEFI shell is still a useless piece of junk.
	*
	* But then on some versions of BDS, we get:

00000000  5c 00 66 00 77 00 75 00  70 00 78 00 36 00 34 00  |\.f.w.u.p.x.6.4.|
00000010  2e 00 65 00 66 00 69 00  00 00                    |..e.f.i...|
0000001a

	* which as you can see is one perfectly normal UCS2-EL string
	* containing the load option from the Boot#### variable.
	*
	* We also sometimes find a guid or partial guid at the end, because
	* BDS will add that, but we ignore that here.
	*/

	/*
	 * Maybe there just aren't any options...
	 */
	if (li->LoadOptionsSize == 0)
		return EFI_SUCCESS;

	/*
	 * In either case, we've got to have at least a UCS2 NUL...
	 */
	if (li->LoadOptionsSize < 2)
		return EFI_BAD_BUFFER_SIZE;

	/*
	 * Some awesome versions of BDS will add entries for Linux.  On top
	 * of that, some versions of BDS will "tag" any Boot#### entries they
	 * create by putting a GUID at the very end of the optional data in
	 * the EFI_LOAD_OPTIONS, thus screwing things up for everybody who
	 * tries to actually *use* the optional data for anything.  Why they
	 * did this instead of adding a flag to the spec to /say/ it's
	 * created by BDS, I do not know.  For shame.
	 *
	 * Anyway, just nerf that out from the start.  It's always just
	 * garbage at the end.
	 */
	if (li->LoadOptionsSize > 16) {
		if (CompareGuid((EFI_GUID *)(li->LoadOptions
					     + (li->LoadOptionsSize - 16)),
				&BDS_GUID) == 0)
			li->LoadOptionsSize -= 16;
	}

	/*
	 * Apparently sometimes we get L"\0\0"?  Which isn't useful at all.
	 */
	if (is_all_nuls(li->LoadOptions, li->LoadOptionsSize))
		return EFI_SUCCESS;

	/*
	 * Check and see if this is just a list of strings.  If it's an
	 * EFI_LOAD_OPTION, it'll be 0, since we know EndEntire device path
	 * won't pass muster as UCS2-LE.
	 *
	 * If there are 3 strings, we're launched from the shell most likely,
	 * But we actually only care about the second one.
	 */
	UINTN strings = count_ucs2_strings(li->LoadOptions,
					   li->LoadOptionsSize);

	/*
	 * In some cases we get strings == 1 because BDS is using L' ' as the
	 * delimeter:
	 * 0000:74 00 65 00 73 00 74 00 2E 00 65 00 66 00 69 00 t.e.s.t...e.f.i.
	 * 0016:20 00 6F 00 6E 00 65 00 20 00 74 00 77 00 6F 00 ..o.n.e...t.w.o.
	 * 0032:20 00 74 00 68 00 72 00 65 00 65 00 00 00       ..t.h.r.e.e...
	 *
	 * If so replace it with NULs since the code already handles that
	 * case.
	 */
	if (strings == 1) {
		UINT16 *cur = start = li->LoadOptions;

		/* replace L' ' with L'\0' if we find any */
		for (i = 0; i < li->LoadOptionsSize / 2; i++) {
			if (cur[i] == L' ')
				cur[i] = L'\0';
		}

		/* redo the string count */
		strings = count_ucs2_strings(li->LoadOptions,
					     li->LoadOptionsSize);
	}

	/*
	 * If it's not string data, try it as an EFI_LOAD_OPTION.
	 */
	if (strings == 0) {
		/*
		 * We at least didn't find /enough/ strings.  See if it works
		 * as an EFI_LOAD_OPTION.
		 */
		efi_status = get_load_option_optional_data(li->LoadOptions,
							   li->LoadOptionsSize,
							   (UINT8 **)&start,
							   &loader_len);
		if (EFI_ERROR(efi_status))
			return EFI_SUCCESS;

		remaining_size = 0;
	} else if (strings >= 2) {
		/*
		 * UEFI shell copies the whole line of the command into
		 * LoadOptions.  We ignore the string before the first L'\0',
		 * i.e. the name of this program.
		 */
		UINT16 *cur = li->LoadOptions;
		for (i = 1; i < li->LoadOptionsSize / 2; i++) {
			if (cur[i - 1] == L'\0') {
				start = &cur[i];
				remaining_size = li->LoadOptionsSize - (i * 2);
				break;
			}
		}

		remaining_size -= i * 2 + 2;
	} else if (strings == 1 && is_our_path(li, start)) {
		/*
		 * And then I found a version of BDS that gives us our own path
		 * in LoadOptions:

77162C58                           5c 00 45 00 46 00 49 00          |\.E.F.I.|
77162C60  5c 00 42 00 4f 00 4f 00  54 00 5c 00 42 00 4f 00  |\.B.O.O.T.\.B.O.|
77162C70  4f 00 54 00 58 00 36 00  34 00 2e 00 45 00 46 00  |O.T.X.6.4...E.F.|
77162C80  49 00 00 00                                       |I...|

		* which is just cruel... So yeah, just don't use it.
		*/
		return EFI_SUCCESS;
	}

	/*
	 * Set up the name of the alternative loader and the LoadOptions for
	 * the loader
	 */
	if (loader_len > 0) {
		/* we might not always have a NULL at the end */
		loader_str = AllocatePool(loader_len + 2);
		if (!loader_str) {
			perror(L"Failed to allocate loader string\n");
			return EFI_OUT_OF_RESOURCES;
		}

		for (i = 0; i < loader_len / 2; i++)
			loader_str[i] = start[i];
		loader_str[loader_len/2] = L'\0';

		second_stage = loader_str;
		load_options = remaining_size ? start + (loader_len/2) : NULL;
		load_options_size = remaining_size;
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
	efi_status = gBS->InstallProtocolInterface(&shim_lock_handle,
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
	gBS->UninstallProtocolInterface(shim_lock_handle, &SHIM_LOCK_GUID,
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

	/*
	 * Free the space allocated for the alternative 2nd stage loader
	 */
	if (load_options_size > 0 && second_stage)
		FreePool(second_stage);

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

	const CHAR16 * const debug_var_name =
#ifdef ENABLE_SHIM_DEVEL
		L"SHIM_DEVEL_DEBUG";
#else
		L"SHIM_DEBUG";
#endif

	if (x)
		return;

	efi_status = get_variable(debug_var_name, &data, &dataSize,
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
		      debug_var_name, &SHIM_LOCK_GUID);
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
		pause();
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
		gRT->ResetSystem(EfiResetCold, EFI_SECURITY_VIOLATION, 0, NULL);
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
		L"SBAT UEFI variable setting failed",
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
		perror(L"SBAT variable initialization failed\n");
		msg = SET_SBAT;
		goto die;
	} else if (EFI_ERROR(efi_status)) {
		dprint(L"SBAT variable initialization failed: %r\n",
		       efi_status);
	}

	if (secure_mode()) {
		char *sbat_start = (char *)&_sbat;
		char *sbat_end = (char *)&_esbat;

		INIT_LIST_HEAD(&sbat_var);
		efi_status = parse_sbat_var(&sbat_var);
		if (EFI_ERROR(efi_status)) {
			perror(L"Parsing SBAT variable failed: %r\n",
				efi_status);
			msg = IMPORT_SBAT;
			goto die;
		}

		efi_status = handle_sbat(sbat_start, sbat_end - sbat_start);
		if (EFI_ERROR(efi_status)) {
			perror(L"Verifiying shim SBAT data failed: %r\n",
			       efi_status);
			msg = SBAT_SELF_CHECK;
			goto die;
		}
	}

	init_openssl();

	/*
	 * Before we do anything else, validate our non-volatile,
	 * boot-services-only state variables are what we think they are.
	 */
	efi_status = import_mok_state(image_handle);
	if (!secure_mode() && efi_status == EFI_INVALID_PARAMETER) {
		/*
		 * Make copy failures fatal only if secure_mode is enabled, or
		 * the error was anything else than EFI_INVALID_PARAMETER.
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
		gRT->ResetSystem(EfiResetShutdown, EFI_SECURITY_VIOLATION,
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
