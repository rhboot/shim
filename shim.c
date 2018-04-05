/*
 * shim - trivial UEFI first-stage bootloader
 *
 * Copyright 2012 Red Hat, Inc <mjg@redhat.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include <stdint.h>

#define OID_EKU_MODSIGN "1.3.6.1.4.1.2312.16.1.2"

static EFI_SYSTEM_TABLE *systab;
static EFI_HANDLE global_image_handle;

static CHAR16 *second_stage;
static void *load_options;
static UINT32 load_options_size;

/*
 * The vendor certificate used for validating the second stage loader
 */
extern struct {
	UINT32 vendor_cert_size;
	UINT32 vendor_dbx_size;
	UINT32 vendor_cert_offset;
	UINT32 vendor_dbx_offset;
} cert_table;

UINT32 vendor_cert_size;
UINT32 vendor_dbx_size;
UINT8 *vendor_cert;
UINT8 *vendor_dbx;

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

/*
 * Perform basic bounds checking of the intra-image pointers
 */
static void *ImageAddress (void *image, uint64_t size, uint64_t address)
{
	/* ensure our local pointer isn't bigger than our size */
	if (address > size)
		return NULL;

	/* Insure our math won't overflow */
	if (UINT64_MAX - address < (uint64_t)(intptr_t)image)
		return NULL;

	/* return the absolute pointer */
	return image + address;
}

/* here's a chart:
 *		i686	x86_64	aarch64
 *  64-on-64:	nyet	yes	yes
 *  64-on-32:	nyet	yes	nyet
 *  32-on-32:	yes	yes	no
 */
static int
allow_64_bit(void)
{
#if defined(__x86_64__) || defined(__aarch64__)
	return 1;
#elif defined(__i386__) || defined(__i686__)
	/* Right now blindly assuming the kernel will correctly detect this
	 * and /halt the system/ if you're not really on a 64-bit cpu */
	if (in_protocol)
		return 1;
	return 0;
#else /* assuming everything else is 32-bit... */
	return 0;
#endif
}

static int
allow_32_bit(void)
{
#if defined(__x86_64__)
#if defined(ALLOW_32BIT_KERNEL_ON_X64)
	if (in_protocol)
		return 1;
	return 0;
#else
	return 0;
#endif
#elif defined(__i386__) || defined(__i686__)
	return 1;
#elif defined(__arch64__)
	return 0;
#else /* assuming everything else is 32-bit... */
	return 1;
#endif
}

static int
image_is_64_bit(EFI_IMAGE_OPTIONAL_HEADER_UNION *PEHdr)
{
	/* .Magic is the same offset in all cases */
	if (PEHdr->Pe32Plus.OptionalHeader.Magic
			== EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		return 1;
	return 0;
}

static const UINT16 machine_type =
#if defined(__x86_64__)
	IMAGE_FILE_MACHINE_X64;
#elif defined(__aarch64__)
	IMAGE_FILE_MACHINE_ARM64;
#elif defined(__arm__)
	IMAGE_FILE_MACHINE_ARMTHUMB_MIXED;
#elif defined(__i386__) || defined(__i486__) || defined(__i686__)
	IMAGE_FILE_MACHINE_I386;
#elif defined(__ia64__)
	IMAGE_FILE_MACHINE_IA64;
#else
#error this architecture is not supported by shim
#endif

static int
image_is_loadable(EFI_IMAGE_OPTIONAL_HEADER_UNION *PEHdr)
{
	/* If the machine type doesn't match the binary, bail, unless
	 * we're in an allowed 64-on-32 scenario */
	if (PEHdr->Pe32.FileHeader.Machine != machine_type) {
		if (!(machine_type == IMAGE_FILE_MACHINE_I386 &&
		      PEHdr->Pe32.FileHeader.Machine == IMAGE_FILE_MACHINE_X64 &&
		      allow_64_bit())) {
			return 0;
		}
	}

	/* If it's not a header type we recognize at all, bail */
	switch (PEHdr->Pe32Plus.OptionalHeader.Magic) {
	case EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC:
	case EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		break;
	default:
		return 0;
	}

	/* and now just check for general 64-vs-32 compatibility */
	if (image_is_64_bit(PEHdr)) {
		if (allow_64_bit())
			return 1;
	} else {
		if (allow_32_bit())
			return 1;
	}
	return 0;
}

/*
 * Perform the actual relocation
 */
static EFI_STATUS relocate_coff (PE_COFF_LOADER_IMAGE_CONTEXT *context,
				 EFI_IMAGE_SECTION_HEADER *Section,
				 void *orig, void *data)
{
	EFI_IMAGE_BASE_RELOCATION *RelocBase, *RelocBaseEnd;
	UINT64 Adjust;
	UINT16 *Reloc, *RelocEnd;
	char *Fixup, *FixupBase;
	UINT16 *Fixup16;
	UINT32 *Fixup32;
	UINT64 *Fixup64;
	int size = context->ImageSize;
	void *ImageEnd = (char *)orig + size;
	int n = 0;

	/* Alright, so here's how this works:
	 *
	 * context->RelocDir gives us two things:
	 * - the VA the table of base relocation blocks are (maybe) to be
	 *   mapped at (RelocDir->VirtualAddress)
	 * - the virtual size (RelocDir->Size)
	 *
	 * The .reloc section (Section here) gives us some other things:
	 * - the name! kind of. (Section->Name)
	 * - the virtual size (Section->VirtualSize), which should be the same
	 *   as RelocDir->Size
	 * - the virtual address (Section->VirtualAddress)
	 * - the file section size (Section->SizeOfRawData), which is
	 *   a multiple of OptHdr->FileAlignment.  Only useful for image
	 *   validation, not really useful for iteration bounds.
	 * - the file address (Section->PointerToRawData)
	 * - a bunch of stuff we don't use that's 0 in our binaries usually
	 * - Flags (Section->Characteristics)
	 *
	 * and then the thing that's actually at the file address is an array
	 * of EFI_IMAGE_BASE_RELOCATION structs with some values packed behind
	 * them.  The SizeOfBlock field of this structure includes the
	 * structure itself, and adding it to that structure's address will
	 * yield the next entry in the array.
	 */
	RelocBase = ImageAddress(orig, size, Section->PointerToRawData);
	/* RelocBaseEnd here is the address of the first entry /past/ the
	 * table.  */
	RelocBaseEnd = ImageAddress(orig, size, Section->PointerToRawData +
						Section->Misc.VirtualSize);

	if (!RelocBase && !RelocBaseEnd)
		return EFI_SUCCESS;

	if (!RelocBase || !RelocBaseEnd) {
		perror(L"Reloc table overflows binary\n");
		return EFI_UNSUPPORTED;
	}

	Adjust = (UINTN)data - context->ImageAddress;

	if (Adjust == 0)
		return EFI_SUCCESS;

	while (RelocBase < RelocBaseEnd) {
		Reloc = (UINT16 *) ((char *) RelocBase + sizeof (EFI_IMAGE_BASE_RELOCATION));

		if ((RelocBase->SizeOfBlock == 0) || (RelocBase->SizeOfBlock > context->RelocDir->Size)) {
			perror(L"Reloc %d block size %d is invalid\n", n, RelocBase->SizeOfBlock);
			return EFI_UNSUPPORTED;
		}

		RelocEnd = (UINT16 *) ((char *) RelocBase + RelocBase->SizeOfBlock);
		if ((void *)RelocEnd < orig || (void *)RelocEnd > ImageEnd) {
			perror(L"Reloc %d entry overflows binary\n", n);
			return EFI_UNSUPPORTED;
		}

		FixupBase = ImageAddress(data, size, RelocBase->VirtualAddress);
		if (!FixupBase) {
			perror(L"Reloc %d Invalid fixupbase\n", n);
			return EFI_UNSUPPORTED;
		}

		while (Reloc < RelocEnd) {
			Fixup = FixupBase + (*Reloc & 0xFFF);
			switch ((*Reloc) >> 12) {
			case EFI_IMAGE_REL_BASED_ABSOLUTE:
				break;

			case EFI_IMAGE_REL_BASED_HIGH:
				Fixup16   = (UINT16 *) Fixup;
				*Fixup16 = (UINT16) (*Fixup16 + ((UINT16) ((UINT32) Adjust >> 16)));
				break;

			case EFI_IMAGE_REL_BASED_LOW:
				Fixup16   = (UINT16 *) Fixup;
				*Fixup16  = (UINT16) (*Fixup16 + (UINT16) Adjust);
				break;

			case EFI_IMAGE_REL_BASED_HIGHLOW:
				Fixup32   = (UINT32 *) Fixup;
				*Fixup32  = *Fixup32 + (UINT32) Adjust;
				break;

			case EFI_IMAGE_REL_BASED_DIR64:
				Fixup64 = (UINT64 *) Fixup;
				*Fixup64 = *Fixup64 + (UINT64) Adjust;
				break;

			default:
				perror(L"Reloc %d Unknown relocation\n", n);
				return EFI_UNSUPPORTED;
			}
			Reloc += 1;
		}
		RelocBase = (EFI_IMAGE_BASE_RELOCATION *) RelocEnd;
		n++;
	}

	return EFI_SUCCESS;
}

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
	if (Cert[0] != 0x30 || Cert[1] != 0x82)
		return FALSE;

	length = Cert[2]<<8 | Cert[3];
	if (length != (CertSize - 4))
		return FALSE;

	return TRUE;
}

static BOOLEAN verify_eku(UINT8 *Cert, UINTN CertSize)
{
	X509 *x509;
	CONST UINT8 *Temp = Cert;
	EXTENDED_KEY_USAGE *eku;
	ASN1_OBJECT *module_signing;

	module_signing = OBJ_nid2obj(OBJ_create(OID_EKU_MODSIGN, NULL, NULL));

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

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		if (CompareGuid (&CertList->SignatureType, &EFI_CERT_TYPE_X509_GUID) == 0) {
			Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
			CertSize = CertList->SignatureSize - sizeof(EFI_GUID);
			if (verify_x509(Cert->SignatureData, CertSize)) {
				if (verify_eku(Cert->SignatureData, CertSize)) {
					IsFound = AuthenticodeVerify (data->CertData,
								      data->Hdr.dwLength - sizeof(data->Hdr),
								      Cert->SignatureData,
								      CertSize,
								      hash, SHA256_DIGEST_SIZE);
					if (IsFound) {
						tpm_measure_variable(dbname, guid, CertSize, Cert->SignatureData);
						drain_openssl_errors();
						return DATA_FOUND;
					} else {
						LogError(L"AuthenticodeVerify(): %d\n", IsFound);
					}
				}
			} else if (verbose) {
				console_notify(L"Not a DER encoding x.509 Certificate");
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
					tpm_measure_variable(dbname, guid, SignatureSize, data);
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
 * built-in blacklist
 */
static EFI_STATUS check_blacklist (WIN_CERTIFICATE_EFI_PKCS *cert,
				   UINT8 *sha256hash, UINT8 *sha1hash)
{
	EFI_SIGNATURE_LIST *dbx = (EFI_SIGNATURE_LIST *)vendor_dbx;

	if (check_db_hash_in_ram(dbx, vendor_dbx_size, sha256hash,
			SHA256_DIGEST_SIZE, EFI_CERT_SHA256_GUID, L"dbx",
			EFI_SECURE_BOOT_DB_GUID) == DATA_FOUND) {
		LogError(L"binary sha256hash found in vendor dbx\n");
		return EFI_SECURITY_VIOLATION;
	}
	if (check_db_hash_in_ram(dbx, vendor_dbx_size, sha1hash,
				 SHA1_DIGEST_SIZE, EFI_CERT_SHA1_GUID, L"dbx",
				 EFI_SECURE_BOOT_DB_GUID) == DATA_FOUND) {
		LogError(L"binary sha1hash found in vendor dbx\n");
		return EFI_SECURITY_VIOLATION;
	}
	if (cert &&
	    check_db_cert_in_ram(dbx, vendor_dbx_size, cert, sha256hash, L"dbx",
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
static EFI_STATUS check_whitelist (WIN_CERTIFICATE_EFI_PKCS *cert,
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
		} else {
			LogError(L"check_db_cert(db, sha256hash) != DATA_FOUND\n");
		}
	}

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
	} else {
		LogError(L"check_db_cert(MokList, sha256hash) != DATA_FOUND\n");
	}

	update_verification_method(VERIFIED_BY_NOTHING);
	return EFI_SECURITY_VIOLATION;
}

/*
 * Check whether we're in Secure Boot and user mode
 */

static BOOLEAN secure_mode (void)
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

#define check_size_line(data, datasize_in, hashbase, hashsize, l) ({	\
	if ((unsigned long)hashbase >					\
			(unsigned long)data + datasize_in) {		\
		efi_status = EFI_INVALID_PARAMETER;			\
		perror(L"shim.c:%d Invalid hash base 0x%016x\n", l,	\
			hashbase);					\
		goto done;						\
	}								\
	if ((unsigned long)hashbase + hashsize >			\
			(unsigned long)data + datasize_in) {		\
		efi_status = EFI_INVALID_PARAMETER;			\
		perror(L"shim.c:%d Invalid hash size 0x%016x\n", l,	\
			hashsize);					\
		goto done;						\
	}								\
})
#define check_size(d,ds,h,hs) check_size_line(d,ds,h,hs,__LINE__)

/*
 * Calculate the SHA1 and SHA256 hashes of a binary
 */

static EFI_STATUS generate_hash (char *data, unsigned int datasize_in,
				 PE_COFF_LOADER_IMAGE_CONTEXT *context,
				 UINT8 *sha256hash, UINT8 *sha1hash)

{
	unsigned int sha256ctxsize, sha1ctxsize;
	unsigned int size = datasize_in;
	void *sha256ctx = NULL, *sha1ctx = NULL;
	char *hashbase;
	unsigned int hashsize;
	unsigned int SumOfBytesHashed, SumOfSectionBytes;
	unsigned int index, pos;
	unsigned int datasize;
	EFI_IMAGE_SECTION_HEADER  *Section;
	EFI_IMAGE_SECTION_HEADER  *SectionHeader = NULL;
	EFI_STATUS efi_status = EFI_SUCCESS;
	EFI_IMAGE_DOS_HEADER *DosHdr = (void *)data;
	unsigned int PEHdr_offset = 0;

	size = datasize = datasize_in;

	if (datasize <= sizeof (*DosHdr) ||
	    DosHdr->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
		perror(L"Invalid signature\n");
		return EFI_INVALID_PARAMETER;
	}
	PEHdr_offset = DosHdr->e_lfanew;

	sha256ctxsize = Sha256GetContextSize();
	sha256ctx = AllocatePool(sha256ctxsize);

	sha1ctxsize = Sha1GetContextSize();
	sha1ctx = AllocatePool(sha1ctxsize);

	if (!sha256ctx || !sha1ctx) {
		perror(L"Unable to allocate memory for hash context\n");
		return EFI_OUT_OF_RESOURCES;
	}

	if (!Sha256Init(sha256ctx) || !Sha1Init(sha1ctx)) {
		perror(L"Unable to initialise hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash start to checksum */
	hashbase = data;
	hashsize = (char *)&context->PEHdr->Pe32.OptionalHeader.CheckSum -
		hashbase;
	check_size(data, datasize_in, hashbase, hashsize);

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		perror(L"Unable to generate hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash post-checksum to start of certificate table */
	hashbase = (char *)&context->PEHdr->Pe32.OptionalHeader.CheckSum +
		sizeof (int);
	hashsize = (char *)context->SecDir - hashbase;
	check_size(data, datasize_in, hashbase, hashsize);

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		perror(L"Unable to generate hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash end of certificate table to end of image header */
	EFI_IMAGE_DATA_DIRECTORY *dd = context->SecDir + 1;
	hashbase = (char *)dd;
	hashsize = context->SizeOfHeaders - (unsigned long)((char *)dd - data);
	if (hashsize > datasize_in) {
		perror(L"Data Directory size %d is invalid\n", hashsize);
		efi_status = EFI_INVALID_PARAMETER;
		goto done;
	}
	check_size(data, datasize_in, hashbase, hashsize);

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		perror(L"Unable to generate hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Sort sections */
	SumOfBytesHashed = context->SizeOfHeaders;

	/* Validate section locations and sizes */
	for (index = 0, SumOfSectionBytes = 0; index < context->PEHdr->Pe32.FileHeader.NumberOfSections; index++) {
		EFI_IMAGE_SECTION_HEADER  *SectionPtr;

		/* Validate SectionPtr is within image */
		SectionPtr = ImageAddress(data, datasize,
			PEHdr_offset +
			sizeof (UINT32) +
			sizeof (EFI_IMAGE_FILE_HEADER) +
			context->PEHdr->Pe32.FileHeader.SizeOfOptionalHeader +
			(index * sizeof(*SectionPtr)));
		if (!SectionPtr) {
			perror(L"Malformed section %d\n", index);
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}
		/* Validate section size is within image. */
		if (SectionPtr->SizeOfRawData >
		    datasize - SumOfBytesHashed - SumOfSectionBytes) {
			perror(L"Malformed section %d size\n", index);
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}
		SumOfSectionBytes += SectionPtr->SizeOfRawData;
	}

	SectionHeader = (EFI_IMAGE_SECTION_HEADER *) AllocateZeroPool (sizeof (EFI_IMAGE_SECTION_HEADER) * context->PEHdr->Pe32.FileHeader.NumberOfSections);
	if (SectionHeader == NULL) {
		perror(L"Unable to allocate section header\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Already validated above */
	Section = ImageAddress(data, datasize,
		PEHdr_offset +
		sizeof (UINT32) +
		sizeof (EFI_IMAGE_FILE_HEADER) +
		context->PEHdr->Pe32.FileHeader.SizeOfOptionalHeader);
	/* But check it again just for better error messaging, and so
	 * clang-analyzer doesn't get confused. */
	if (Section == NULL) {
		uint64_t addr;

		addr = PEHdr_offset + sizeof(UINT32) + sizeof(EFI_IMAGE_FILE_HEADER)
			+ context->PEHdr->Pe32.FileHeader.SizeOfOptionalHeader;
		perror(L"Malformed file header.\n");
		perror(L"Image address for Section 0 is 0x%016llx\n", addr);
		perror(L"File size is 0x%016llx\n", datasize);
		efi_status = EFI_INVALID_PARAMETER;
		goto done;
	}

	/* Sort the section headers */
	for (index = 0; index < context->PEHdr->Pe32.FileHeader.NumberOfSections; index++) {
		pos = index;
		while ((pos > 0) && (Section->PointerToRawData < SectionHeader[pos - 1].PointerToRawData)) {
			CopyMem (&SectionHeader[pos], &SectionHeader[pos - 1], sizeof (EFI_IMAGE_SECTION_HEADER));
			pos--;
		}
		CopyMem (&SectionHeader[pos], Section, sizeof (EFI_IMAGE_SECTION_HEADER));
		Section += 1;
	}

	/* Hash the sections */
	for (index = 0; index < context->PEHdr->Pe32.FileHeader.NumberOfSections; index++) {
		Section = &SectionHeader[index];
		if (Section->SizeOfRawData == 0) {
			continue;
		}
		hashbase  = ImageAddress(data, size, Section->PointerToRawData);

		if (!hashbase) {
			perror(L"Malformed section header\n");
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}

		/* Verify hashsize within image. */
		if (Section->SizeOfRawData >
		    datasize - Section->PointerToRawData) {
			perror(L"Malformed section raw size %d\n", index);
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}
		hashsize  = (unsigned int) Section->SizeOfRawData;
		check_size(data, datasize_in, hashbase, hashsize);

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			perror(L"Unable to generate hash\n");
			efi_status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
		SumOfBytesHashed += Section->SizeOfRawData;
	}

	/* Hash all remaining data up to SecDir if SecDir->Size is not 0 */
	if (datasize > SumOfBytesHashed && context->SecDir->Size) {
		hashbase = data + SumOfBytesHashed;
		hashsize = datasize - context->SecDir->Size - SumOfBytesHashed;

		if ((datasize - SumOfBytesHashed < context->SecDir->Size) ||
		    (SumOfBytesHashed + hashsize != context->SecDir->VirtualAddress)) {
			perror(L"Malformed binary after Attribute Certificate Table\n");
			console_print(L"datasize: %u SumOfBytesHashed: %u SecDir->Size: %lu\n",
				      datasize, SumOfBytesHashed, context->SecDir->Size);
			console_print(L"hashsize: %u SecDir->VirtualAddress: 0x%08lx\n",
				      hashsize, context->SecDir->VirtualAddress);
			efi_status = EFI_INVALID_PARAMETER;
			goto done;
		}
		check_size(data, datasize_in, hashbase, hashsize);

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			perror(L"Unable to generate hash\n");
			efi_status = EFI_OUT_OF_RESOURCES;
			goto done;
		}

#if 1
	}
#else // we have to migrate to doing this later :/
		SumOfBytesHashed += hashsize;
	}

	/* Hash all remaining data */
	if (datasize > SumOfBytesHashed) {
		hashbase = data + SumOfBytesHashed;
		hashsize = datasize - SumOfBytesHashed;

		check_size(data, datasize_in, hashbase, hashsize);

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			perror(L"Unable to generate hash\n");
			efi_status = EFI_OUT_OF_RESOURCES;
			goto done;
		}

		SumOfBytesHashed += hashsize;
	}
#endif

	if (!(Sha256Final(sha256ctx, sha256hash)) ||
	    !(Sha1Final(sha1ctx, sha1hash))) {
		perror(L"Unable to finalise hash\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

done:
	if (SectionHeader)
		FreePool(SectionHeader);
	if (sha1ctx)
		FreePool(sha1ctx);
	if (sha256ctx)
		FreePool(sha256ctx);

	return efi_status;
}

/*
 * Check that the signature is valid and matches the binary
 */
static EFI_STATUS verify_buffer (char *data, int datasize,
				 PE_COFF_LOADER_IMAGE_CONTEXT *context,
				 UINT8 *sha256hash, UINT8 *sha1hash)
{
	EFI_STATUS efi_status = EFI_SECURITY_VIOLATION;
	WIN_CERTIFICATE_EFI_PKCS *cert = NULL;
	unsigned int size = datasize;

	if (datasize < 0)
		return EFI_INVALID_PARAMETER;

	if (context->SecDir->Size != 0) {
		if (context->SecDir->Size >= size) {
			perror(L"Certificate Database size is too large\n");
			return EFI_INVALID_PARAMETER;
		}

		cert = ImageAddress (data, size,
				     context->SecDir->VirtualAddress);

		if (!cert) {
			perror(L"Certificate located outside the image\n");
			return EFI_INVALID_PARAMETER;
		}

		if (cert->Hdr.dwLength > context->SecDir->Size) {
			perror(L"Certificate list size is inconsistent with PE headers");
			return EFI_INVALID_PARAMETER;
		}

		if (cert->Hdr.wCertificateType !=
		    WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
			perror(L"Unsupported certificate type %x\n",
				cert->Hdr.wCertificateType);
			return EFI_UNSUPPORTED;
		}
	}

	/*
	 * Clear OpenSSL's error log, because we get some DSO unimplemented
	 * errors during its intialization, and we don't want those to look
	 * like they're the reason for validation failures.
	 */
	drain_openssl_errors();

	efi_status = generate_hash(data, datasize, context, sha256hash, sha1hash);
	if (EFI_ERROR(efi_status)) {
		LogError(L"generate_hash: %r\n", efi_status);
		return efi_status;
	}

	/*
	 * Ensure that the binary isn't blacklisted
	 */
	efi_status = check_blacklist(cert, sha256hash, sha1hash);
	if (EFI_ERROR(efi_status)) {
		perror(L"Binary is blacklisted\n");
		LogError(L"Binary is blacklisted: %r\n", efi_status);
		return efi_status;
	}

	/*
	 * Check whether the binary is whitelisted in any of the firmware
	 * databases
	 */
	efi_status = check_whitelist(cert, sha256hash, sha1hash);
	if (EFI_ERROR(efi_status)) {
		LogError(L"check_whitelist(): %r\n", efi_status);
	} else {
		drain_openssl_errors();
		return efi_status;
	}

	if (cert) {
#if defined(ENABLE_SHIM_CERT)
		/*
		 * Check against the shim build key
		 */
		if (sizeof(shim_cert) &&
		    AuthenticodeVerify(cert->CertData,
			       cert->Hdr.dwLength - sizeof(cert->Hdr),
			       shim_cert, sizeof(shim_cert), sha256hash,
			       SHA256_DIGEST_SIZE)) {
			update_verification_method(VERIFIED_BY_CERT);
			tpm_measure_variable(L"Shim", SHIM_LOCK_GUID,
					     sizeof(shim_cert), shim_cert);
			efi_status = EFI_SUCCESS;
			drain_openssl_errors();
			return efi_status;
		} else {
			LogError(L"AuthenticodeVerify(shim_cert) failed\n");
		}
#endif /* defined(ENABLE_SHIM_CERT) */

		/*
		 * And finally, check against shim's built-in key
		 */
		if (vendor_cert_size &&
		    AuthenticodeVerify(cert->CertData,
				       cert->Hdr.dwLength - sizeof(cert->Hdr),
				       vendor_cert, vendor_cert_size,
				       sha256hash, SHA256_DIGEST_SIZE)) {
			update_verification_method(VERIFIED_BY_CERT);
			tpm_measure_variable(L"Shim", SHIM_LOCK_GUID,
					     vendor_cert_size, vendor_cert);
			efi_status = EFI_SUCCESS;
			drain_openssl_errors();
			return efi_status;
		} else {
			LogError(L"AuthenticodeVerify(vendor_cert) failed\n");
		}
	}

	LogError(L"Binary is not whitelisted\n");
	crypterr(EFI_SECURITY_VIOLATION);
	PrintErrors();
	efi_status = EFI_SECURITY_VIOLATION;
	return efi_status;
}

/*
 * Read the binary header and grab appropriate information from it
 */
static EFI_STATUS read_header(void *data, unsigned int datasize,
			      PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	EFI_IMAGE_DOS_HEADER *DosHdr = data;
	EFI_IMAGE_OPTIONAL_HEADER_UNION *PEHdr = data;
	unsigned long HeaderWithoutDataDir, SectionHeaderOffset, OptHeaderSize;
	unsigned long FileAlignment = 0;

	if (datasize < sizeof (PEHdr->Pe32)) {
		perror(L"Invalid image\n");
		return EFI_UNSUPPORTED;
	}

	if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE)
		PEHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((char *)data + DosHdr->e_lfanew);

	if (!image_is_loadable(PEHdr)) {
		perror(L"Platform does not support this image\n");
		return EFI_UNSUPPORTED;
	}

	if (image_is_64_bit(PEHdr)) {
		context->NumberOfRvaAndSizes = PEHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes;
		context->SizeOfHeaders = PEHdr->Pe32Plus.OptionalHeader.SizeOfHeaders;
		context->ImageSize = PEHdr->Pe32Plus.OptionalHeader.SizeOfImage;
		context->SectionAlignment = PEHdr->Pe32Plus.OptionalHeader.SectionAlignment;
		FileAlignment = PEHdr->Pe32Plus.OptionalHeader.FileAlignment;
		OptHeaderSize = sizeof(EFI_IMAGE_OPTIONAL_HEADER64);
	} else {
		context->NumberOfRvaAndSizes = PEHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes;
		context->SizeOfHeaders = PEHdr->Pe32.OptionalHeader.SizeOfHeaders;
		context->ImageSize = (UINT64)PEHdr->Pe32.OptionalHeader.SizeOfImage;
		context->SectionAlignment = PEHdr->Pe32.OptionalHeader.SectionAlignment;
		FileAlignment = PEHdr->Pe32.OptionalHeader.FileAlignment;
		OptHeaderSize = sizeof(EFI_IMAGE_OPTIONAL_HEADER32);
	}

	if (FileAlignment % 2 != 0) {
		perror(L"File Alignment is invalid (%d)\n", FileAlignment);
		return EFI_UNSUPPORTED;
	}
	if (FileAlignment == 0)
		FileAlignment = 0x200;
	if (context->SectionAlignment == 0)
		context->SectionAlignment = PAGE_SIZE;
	if (context->SectionAlignment < FileAlignment)
		context->SectionAlignment = FileAlignment;

	context->NumberOfSections = PEHdr->Pe32.FileHeader.NumberOfSections;

	if (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES < context->NumberOfRvaAndSizes) {
		perror(L"Image header too small\n");
		return EFI_UNSUPPORTED;
	}

	HeaderWithoutDataDir = OptHeaderSize
			- sizeof (EFI_IMAGE_DATA_DIRECTORY) * EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;
	if (((UINT32)PEHdr->Pe32.FileHeader.SizeOfOptionalHeader - HeaderWithoutDataDir) !=
			context->NumberOfRvaAndSizes * sizeof (EFI_IMAGE_DATA_DIRECTORY)) {
		perror(L"Image header overflows data directory\n");
		return EFI_UNSUPPORTED;
	}

	SectionHeaderOffset = DosHdr->e_lfanew
				+ sizeof (UINT32)
				+ sizeof (EFI_IMAGE_FILE_HEADER)
				+ PEHdr->Pe32.FileHeader.SizeOfOptionalHeader;
	if (((UINT32)context->ImageSize - SectionHeaderOffset) / EFI_IMAGE_SIZEOF_SECTION_HEADER
			<= context->NumberOfSections) {
		perror(L"Image sections overflow image size\n");
		return EFI_UNSUPPORTED;
	}

	if ((context->SizeOfHeaders - SectionHeaderOffset) / EFI_IMAGE_SIZEOF_SECTION_HEADER
			< (UINT32)context->NumberOfSections) {
		perror(L"Image sections overflow section headers\n");
		return EFI_UNSUPPORTED;
	}

	if ((((UINT8 *)PEHdr - (UINT8 *)data) + sizeof(EFI_IMAGE_OPTIONAL_HEADER_UNION)) > datasize) {
		perror(L"Invalid image\n");
		return EFI_UNSUPPORTED;
	}

	if (PEHdr->Te.Signature != EFI_IMAGE_NT_SIGNATURE) {
		perror(L"Unsupported image type\n");
		return EFI_UNSUPPORTED;
	}

	if (PEHdr->Pe32.FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED) {
		perror(L"Unsupported image - Relocations have been stripped\n");
		return EFI_UNSUPPORTED;
	}

	context->PEHdr = PEHdr;

	if (image_is_64_bit(PEHdr)) {
		context->ImageAddress = PEHdr->Pe32Plus.OptionalHeader.ImageBase;
		context->EntryPoint = PEHdr->Pe32Plus.OptionalHeader.AddressOfEntryPoint;
		context->RelocDir = &PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
		context->SecDir = &PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
	} else {
		context->ImageAddress = PEHdr->Pe32.OptionalHeader.ImageBase;
		context->EntryPoint = PEHdr->Pe32.OptionalHeader.AddressOfEntryPoint;
		context->RelocDir = &PEHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
		context->SecDir = &PEHdr->Pe32.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
	}

	context->FirstSection = (EFI_IMAGE_SECTION_HEADER *)((char *)PEHdr + PEHdr->Pe32.FileHeader.SizeOfOptionalHeader + sizeof(UINT32) + sizeof(EFI_IMAGE_FILE_HEADER));

	if (context->ImageSize < context->SizeOfHeaders) {
		perror(L"Invalid image\n");
		return EFI_UNSUPPORTED;
	}

	if ((unsigned long)((UINT8 *)context->SecDir - (UINT8 *)data) >
	    (datasize - sizeof(EFI_IMAGE_DATA_DIRECTORY))) {
		perror(L"Invalid image\n");
		return EFI_UNSUPPORTED;
	}

	if (context->SecDir->VirtualAddress > datasize ||
	    (context->SecDir->VirtualAddress == datasize &&
	     context->SecDir->Size > 0)) {
		perror(L"Malformed security header\n");
		return EFI_INVALID_PARAMETER;
	}
	return EFI_SUCCESS;
}

/*
 * Once the image has been loaded it needs to be validated and relocated
 */
static EFI_STATUS handle_image (void *data, unsigned int datasize,
				EFI_LOADED_IMAGE *li,
				EFI_IMAGE_ENTRY_POINT *entry_point,
				EFI_PHYSICAL_ADDRESS *alloc_address,
				UINTN *alloc_pages)
{
	EFI_STATUS efi_status;
	char *buffer;
	int i;
	EFI_IMAGE_SECTION_HEADER *Section;
	char *base, *end;
	PE_COFF_LOADER_IMAGE_CONTEXT context;
	unsigned int alignment, alloc_size;
	int found_entry_point = 0;
	UINT8 sha1hash[SHA1_DIGEST_SIZE];
	UINT8 sha256hash[SHA256_DIGEST_SIZE];

	/*
	 * The binary header contains relevant context and section pointers
	 */
	efi_status = read_header(data, datasize, &context);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to read header: %r\n", efi_status);
		return efi_status;
	}

	/*
	 * We only need to verify the binary if we're in secure mode
	 */
	efi_status = generate_hash(data, datasize, &context, sha256hash,
				   sha1hash);
	if (EFI_ERROR(efi_status))
		return efi_status;

	/* Measure the binary into the TPM */
#ifdef REQUIRE_TPM
	efi_status =
#endif
	tpm_log_pe((EFI_PHYSICAL_ADDRESS)(UINTN)data, datasize, sha1hash, 4);
#ifdef REQUIRE_TPM
	if (efi_status != EFI_SUCCESS) {
		return efi_status;
	}
#endif

	if (secure_mode ()) {
		efi_status = verify_buffer(data, datasize, &context,
					   sha256hash, sha1hash);

		if (EFI_ERROR(efi_status)) {
			console_error(L"Verification failed", efi_status);
			return efi_status;
		} else {
			if (verbose)
				console_print(L"Verification succeeded\n");
		}
	}

	/* The spec says, uselessly, of SectionAlignment:
	 * =====
	 * The alignment (in bytes) of sections when they are loaded into
	 * memory. It must be greater than or equal to FileAlignment. The
	 * default is the page size for the architecture.
	 * =====
	 * Which doesn't tell you whose responsibility it is to enforce the
	 * "default", or when.  It implies that the value in the field must
	 * be > FileAlignment (also poorly defined), but it appears visual
	 * studio will happily write 512 for FileAlignment (its default) and
	 * 0 for SectionAlignment, intending to imply PAGE_SIZE.
	 *
	 * We only support one page size, so if it's zero, nerf it to 4096.
	 */
	alignment = context.SectionAlignment;
	if (!alignment)
		alignment = 4096;

	alloc_size = ALIGN_VALUE(context.ImageSize + context.SectionAlignment,
				 PAGE_SIZE);
	*alloc_pages = alloc_size / PAGE_SIZE;

	efi_status = gBS->AllocatePages(AllocateAnyPages, EfiLoaderCode,
					*alloc_pages, alloc_address);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to allocate image buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	buffer = (void *)ALIGN_VALUE((unsigned long)*alloc_address, alignment);

	CopyMem(buffer, data, context.SizeOfHeaders);

	*entry_point = ImageAddress(buffer, context.ImageSize, context.EntryPoint);
	if (!*entry_point) {
		perror(L"Entry point is invalid\n");
		gBS->FreePages(*alloc_address, *alloc_pages);
		return EFI_UNSUPPORTED;
	}


	char *RelocBase, *RelocBaseEnd;
	/*
	 * These are relative virtual addresses, so we have to check them
	 * against the image size, not the data size.
	 */
	RelocBase = ImageAddress(buffer, context.ImageSize,
				 context.RelocDir->VirtualAddress);
	/*
	 * RelocBaseEnd here is the address of the last byte of the table
	 */
	RelocBaseEnd = ImageAddress(buffer, context.ImageSize,
				    context.RelocDir->VirtualAddress +
				    context.RelocDir->Size - 1);

	EFI_IMAGE_SECTION_HEADER *RelocSection = NULL;

	/*
	 * Copy the executable's sections to their desired offsets
	 */
	Section = context.FirstSection;
	for (i = 0; i < context.NumberOfSections; i++, Section++) {
		base = ImageAddress (buffer, context.ImageSize,
				     Section->VirtualAddress);
		end = ImageAddress (buffer, context.ImageSize,
				    Section->VirtualAddress
				     + Section->Misc.VirtualSize - 1);

		if (end < base) {
			perror(L"Section %d has negative size\n", i);
			gBS->FreePages(*alloc_address, *alloc_pages);
			return EFI_UNSUPPORTED;
		}

		if (Section->VirtualAddress <= context.EntryPoint &&
		    (Section->VirtualAddress + Section->SizeOfRawData - 1)
		    > context.EntryPoint)
			found_entry_point++;

		/* We do want to process .reloc, but it's often marked
		 * discardable, so we don't want to memcpy it. */
		if (CompareMem(Section->Name, ".reloc\0\0", 8) == 0) {
			if (RelocSection) {
				perror(L"Image has multiple relocation sections\n");
				return EFI_UNSUPPORTED;
			}
			/* If it has nonzero sizes, and our bounds check
			 * made sense, and the VA and size match RelocDir's
			 * versions, then we believe in this section table. */
			if (Section->SizeOfRawData &&
					Section->Misc.VirtualSize &&
					base && end &&
					RelocBase == base &&
					RelocBaseEnd == end) {
				RelocSection = Section;
			}
		}

		if (Section->Characteristics & EFI_IMAGE_SCN_MEM_DISCARDABLE) {
			continue;
		}

		if (!base) {
			perror(L"Section %d has invalid base address\n", i);
			return EFI_UNSUPPORTED;
		}
		if (!end) {
			perror(L"Section %d has zero size\n", i);
			return EFI_UNSUPPORTED;
		}

		if (!(Section->Characteristics & EFI_IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
		    (Section->VirtualAddress < context.SizeOfHeaders ||
		     Section->PointerToRawData < context.SizeOfHeaders)) {
			perror(L"Section %d is inside image headers\n", i);
			return EFI_UNSUPPORTED;
		}

		if (Section->Characteristics & EFI_IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
			ZeroMem(base, Section->Misc.VirtualSize);
		} else {
			if (Section->PointerToRawData < context.SizeOfHeaders) {
				perror(L"Section %d is inside image headers\n", i);
				return EFI_UNSUPPORTED;
			}

			if (Section->SizeOfRawData > 0)
				CopyMem(base, data + Section->PointerToRawData,
					Section->SizeOfRawData);

			if (Section->SizeOfRawData < Section->Misc.VirtualSize)
				ZeroMem(base + Section->SizeOfRawData,
					Section->Misc.VirtualSize - Section->SizeOfRawData);
		}
	}

	if (context.NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
		perror(L"Image has no relocation entry\n");
		FreePool(buffer);
		return EFI_UNSUPPORTED;
	}

	if (context.RelocDir->Size && RelocSection) {
		/*
		 * Run the relocation fixups
		 */
		efi_status = relocate_coff(&context, RelocSection, data,
					   buffer);

		if (EFI_ERROR(efi_status)) {
			perror(L"Relocation failed: %r\n", efi_status);
			FreePool(buffer);
			return efi_status;
		}
	}

	/*
	 * grub needs to know its location and size in memory, so fix up
	 * the loaded image protocol values
	 */
	li->ImageBase = buffer;
	li->ImageSize = context.ImageSize;

	/* Pass the load options to the second stage loader */
	if ( load_options ) {
		li->LoadOptions = load_options;
		li->LoadOptionsSize = load_options_size;
	}

	if (!found_entry_point) {
		perror(L"Entry point is not within sections\n");
		return EFI_UNSUPPORTED;
	}
	if (found_entry_point > 1) {
		perror(L"%d sections contain entry point\n");
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
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
	 * that we don't just use \\, becuase we (not particularly brightly)
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

	while (*ImagePath == '\\')
		ImagePath++;

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
	tpm_log_pe((EFI_PHYSICAL_ADDRESS)(UINTN)buffer, size, sha1hash, 4);
#ifdef REQUIRE_TPM
	if (EFI_ERROR(efi_status))
		goto done;
#endif

	if (!secure_mode()) {
		efi_status = EFI_SUCCESS;
		goto done;
	}

	efi_status = verify_buffer(buffer, size, &context,
				   sha256hash, sha1hash);
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

/*
 * Load and run an EFI executable
 */
EFI_STATUS start_image(EFI_HANDLE image_handle, CHAR16 *ImagePath)
{
	EFI_STATUS efi_status;
	EFI_LOADED_IMAGE *li, li_bak;
	EFI_IMAGE_ENTRY_POINT entry_point;
	EFI_PHYSICAL_ADDRESS alloc_address;
	UINTN alloc_pages;
	CHAR16 *PathName = NULL;
	void *sourcebuffer = NULL;
	UINT64 sourcesize = 0;
	void *data = NULL;
	int datasize;

	/*
	 * We need to refer to the loaded image protocol on the running
	 * binary in order to find our path
	 */
	efi_status = gBS->HandleProtocol(image_handle, &EFI_LOADED_IMAGE_GUID,
					 (void **)&li);
	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to init protocol\n");
		return efi_status;
	}

	/*
	 * Build a new path from the existing one plus the executable name
	 */
	efi_status = generate_path_from_image_path(li, ImagePath, &PathName);
	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to generate path %s: %r\n", ImagePath,
		       efi_status);
		goto done;
	}

	if (findNetboot(li->DeviceHandle)) {
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
#if  defined(ENABLE_HTTPBOOT)
	} else if (find_httpboot(li->DeviceHandle)) {
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
#endif
	} else {
		/*
		 * Read the new executable off disk
		 */
		efi_status = load_image(li, &data, &datasize, PathName);
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
	CopyMem(&li_bak, li, sizeof(li_bak));

	/*
	 * Verify and, if appropriate, relocate and execute the executable
	 */
	efi_status = handle_image(data, datasize, li, &entry_point,
				  &alloc_address, &alloc_pages);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to load image: %r\n", efi_status);
		PrintErrors();
		ClearErrors();
		CopyMem(li, &li_bak, sizeof(li_bak));
		goto done;
	}

	loader_is_participating = 0;

	/*
	 * The binary is trusted and relocated. Run it
	 */
	efi_status = entry_point(image_handle, systab);

	/*
	 * Restore our original loaded image values
	 */
	CopyMem(li, &li_bak, sizeof(li_bak));
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

static int is_our_path(EFI_LOADED_IMAGE *li, CHAR16 *path, UINTN len)
{
	CHAR16 *dppath = NULL;
	int ret = 1;

	dppath = DevicePathToStr(li->FilePath);
	if (!dppath)
		return 0;

	console_print(L"dppath: %s\n", dppath);
	console_print(L"path:   %s\n", path);
	if (StrnCaseCmp(dppath, path, len))
		ret = 0;

	FreePool(dppath);
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
	int remaining_size = 0;
	CHAR16 *loader_str = NULL;
	UINTN loader_len = 0;
	unsigned int i;

	second_stage = DEFAULT_LOADER;
	load_options = NULL;
	load_options_size = 0;

	efi_status = gBS->HandleProtocol(image_handle, &LoadedImageProtocol,
					 (void **) &li);
	if (EFI_ERROR(efi_status)) {
		perror (L"Failed to get load options: %r\n", efi_status);
		return efi_status;
	}

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
		 * LoadOptions.  We ignore the string before the first L' ',
		 * i.e. the name of this program.
		 * Counting by two bytes is safe, because we know the size is
		 * compatible with a UCS2-LE string.
		 */
		UINT8 *cur = li->LoadOptions;
		for (i = 0; i < li->LoadOptionsSize - 2; i += 2) {
			CHAR16 c = (cur[i+1] << 8) | cur[i];
			if (c == L' ') {
				start = (CHAR16 *)&cur[i+2];
				remaining_size = li->LoadOptionsSize - i - 2;
				break;
			}
		}

		if (!start || remaining_size <= 0 || start[0] == L'\0')
			return EFI_SUCCESS;

		for (i = 0; start[i] != '\0'; i++) {
			if (start[i] == L' ')
				start[i] = L'\0';
			if (start[i] == L'\0') {
				loader_len = 2 * i + 2;
				break;
			}
		}
		if (loader_len)
			remaining_size -= loader_len;
	} else {
		/* only find one string */
		start = li->LoadOptions;
		loader_len = li->LoadOptionsSize;
	}

	/*
	 * Just to be sure all that math is right...
	 */
	if (loader_len % 2 != 0)
		return EFI_INVALID_PARAMETER;

	strings = count_ucs2_strings((UINT8 *)start, loader_len);
	if (strings < 1)
		return EFI_SUCCESS;

	/*
	 * And then I found a version of BDS that gives us our own path in
	 * LoadOptions:

77162C58                           5c 00 45 00 46 00 49 00          |\.E.F.I.|
77162C60  5c 00 42 00 4f 00 4f 00  54 00 5c 00 42 00 4f 00  |\.B.O.O.T.\.B.O.|
77162C70  4f 00 54 00 58 00 36 00  34 00 2e 00 45 00 46 00  |O.T.X.6.4...E.F.|
77162C80  49 00 00 00                                       |I...|

	 * which is just cruel... So yeah, just don't use it.
	 */
	if (strings == 1 && is_our_path(li, start, loader_len))
		return EFI_SUCCESS;

	/*
	 * Set up the name of the alternative loader and the LoadOptions for
	 * the loader
	 */
	if (loader_len > 0) {
		loader_str = AllocatePool(loader_len);
		if (!loader_str) {
			perror(L"Failed to allocate loader string\n");
			return EFI_OUT_OF_RESOURCES;
		}

		for (i = 0; i < loader_len / 2; i++)
			loader_str[i] = start[i];
		loader_str[loader_len/2-1] = L'\0';

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
	setup_verbosity();
	dprint(L"%a", shim_version);

	/* Set the second stage loader */
	set_second_stage (global_image_handle);

	if (secure_mode()) {
		if (vendor_cert_size || vendor_dbx_size) {
			/*
			 * If shim includes its own certificates then ensure
			 * that anything it boots has performed some
			 * validation of the next image.
			 */
			hook_system_services(systab);
			loader_is_participating = 0;
		}

		hook_exit(systab);
	}

	return install_shim_protocols();
}

void
shim_fini(void)
{
	/*
	 * Remove our protocols
	 */
	uninstall_shim_protocols();

	if (secure_mode()) {

		/*
		 * Remove our hooks from system services.
		 */
		unhook_system_services();
		unhook_exit();
	}

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
	volatile register UINTN x = 0;
	extern char _text, _data;

	if (x)
		return;

	efi_status = get_variable(L"SHIM_DEBUG", &data, &dataSize,
				  SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
		return;
	}

	FreePool(data);

	console_print(L"add-symbol-file "DEBUGDIR
		      L"shim" EFI_ARCH L".efi.debug 0x%08x -s .data 0x%08x\n",
		      &_text, &_data);

	console_print(L"Pausing for debugger attachment.\n");
	console_print(L"To disable this, remove the EFI variable SHIM_DEBUG-%g .\n",
		      &SHIM_LOCK_GUID);
	x = 1;
	while (x++) {
		/* Make this so it can't /totally/ DoS us. */
#if defined(__x86_64__) || defined(__i386__) || defined(__i686__)
		if (x > 4294967294ULL)
			break;
		__asm__ __volatile__("pause");
#elif defined(__aarch64__)
		if (x > 1000)
			break;
		__asm__ __volatile__("wfi");
#else
		if (x > 12000)
			break;
		msleep(5000);
#endif
	}
	x = 1;
}

EFI_STATUS
efi_main (EFI_HANDLE passed_image_handle, EFI_SYSTEM_TABLE *passed_systab)
{
	EFI_STATUS efi_status;
	EFI_HANDLE image_handle;

	verification_method = VERIFIED_BY_NOTHING;

	vendor_cert_size = cert_table.vendor_cert_size;
	vendor_dbx_size = cert_table.vendor_dbx_size;
	vendor_cert = (UINT8 *)&cert_table + cert_table.vendor_cert_offset;
	vendor_dbx = (UINT8 *)&cert_table + cert_table.vendor_dbx_offset;
	CHAR16 *msgs[] = {
		L"import_mok_state() failed\n",
		L"shim_int() failed\n",
		NULL
	};
	int msg = 0;


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

	init_openssl();

	/*
	 * if SHIM_DEBUG is set, wait for a debugger to attach.
	 */
	debug_hook();

	/*
	 * Before we do anything else, validate our non-volatile,
	 * boot-services-only state variables are what we think they are.
	 */
	efi_status = import_mok_state(image_handle);
	if (EFI_ERROR(efi_status)) {
die:
		console_print(L"Something has gone seriously wrong: %s: %r\n",
			      msgs[msg], efi_status);
		msleep(5000000);
		gRT->ResetSystem(EfiResetShutdown, EFI_SECURITY_VIOLATION,
				 0, NULL);
	}

	efi_status = shim_init();
	if (EFI_ERROR(efi_status)) {
		msg = 1;
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
	return efi_status;
}
