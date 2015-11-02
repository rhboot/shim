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

#include <efi.h>
#include <efilib.h>
#include <Library/BaseCryptLib.h>
#include "PeImage.h"
#include "shim.h"
#include "netboot.h"
#include "shim_cert.h"
#include "replacements.h"
#include "ucs2.h"

#include "guid.h"
#include "variables.h"
#include "efiauthenticated.h"
#include "security_policy.h"
#include "console.h"
#include "version.h"

#define FALLBACK L"\\fallback.efi"
#define MOK_MANAGER L"\\MokManager.efi"

static EFI_SYSTEM_TABLE *systab;
static EFI_HANDLE image_handle;
static EFI_STATUS (EFIAPI *entry_point) (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table);

static CHAR16 *second_stage;
static void *load_options;
static UINT32 load_options_size;
static UINT8 in_protocol;

#define perror(fmt, ...) ({						\
		UINTN __perror_ret = 0;					\
		if (!in_protocol)					\
			__perror_ret = Print((fmt), ##__VA_ARGS__);	\
		__perror_ret;						\
	})

EFI_GUID SHIM_LOCK_GUID = { 0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23} };

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
static void *ImageAddress (void *image, unsigned int size, unsigned int address)
{
	if (address > size)
		return NULL;

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
	char *Fixup, *FixupBase, *FixupData = NULL;
	UINT16 *Fixup16;
	UINT32 *Fixup32;
	UINT64 *Fixup64;
	int size = context->ImageSize;
	void *ImageEnd = (char *)orig + size;
	int n = 0;

	if (image_is_64_bit(context->PEHdr))
		context->PEHdr->Pe32Plus.OptionalHeader.ImageBase = (UINT64)(unsigned long)data;
	else
		context->PEHdr->Pe32.OptionalHeader.ImageBase = (UINT32)(unsigned long)data;

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
				if (FixupData != NULL) {
					*(UINT16 *) FixupData = *Fixup16;
					FixupData             = FixupData + sizeof (UINT16);
				}
				break;

			case EFI_IMAGE_REL_BASED_LOW:
				Fixup16   = (UINT16 *) Fixup;
				*Fixup16  = (UINT16) (*Fixup16 + (UINT16) Adjust);
				if (FixupData != NULL) {
					*(UINT16 *) FixupData = *Fixup16;
					FixupData             = FixupData + sizeof (UINT16);
				}
				break;

			case EFI_IMAGE_REL_BASED_HIGHLOW:
				Fixup32   = (UINT32 *) Fixup;
				*Fixup32  = *Fixup32 + (UINT32) Adjust;
				if (FixupData != NULL) {
					FixupData             = ALIGN_POINTER (FixupData, sizeof (UINT32));
					*(UINT32 *)FixupData  = *Fixup32;
					FixupData             = FixupData + sizeof (UINT32);
				}
				break;

			case EFI_IMAGE_REL_BASED_DIR64:
				Fixup64 = (UINT64 *) Fixup;
				*Fixup64 = *Fixup64 + (UINT64) Adjust;
				if (FixupData != NULL) {
					FixupData = ALIGN_POINTER (FixupData, sizeof(UINT64));
					*(UINT64 *)(FixupData) = *Fixup64;
					FixupData = FixupData + sizeof(UINT64);
				}
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

static CHECK_STATUS check_db_cert_in_ram(EFI_SIGNATURE_LIST *CertList,
					 UINTN dbsize,
					 WIN_CERTIFICATE_EFI_PKCS *data,
					 UINT8 *hash)
{
	EFI_SIGNATURE_DATA *Cert;
	UINTN CertSize;
	BOOLEAN IsFound = FALSE;
	EFI_GUID CertType = X509_GUID;

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		if (CompareGuid (&CertList->SignatureType, &CertType) == 0) {
			Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
			CertSize = CertList->SignatureSize - sizeof(EFI_GUID);
			if (verify_x509(Cert->SignatureData, CertSize)) {
				IsFound = AuthenticodeVerify (data->CertData,
							      data->Hdr.dwLength - sizeof(data->Hdr),
							      Cert->SignatureData,
							      CertSize,
							      hash, SHA256_DIGEST_SIZE);
				if (IsFound)
					return DATA_FOUND;
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

	if (efi_status != EFI_SUCCESS)
		return VAR_NOT_FOUND;

	CertList = (EFI_SIGNATURE_LIST *)db;

	rc = check_db_cert_in_ram(CertList, dbsize, data, hash);

	FreePool(db);

	return rc;
}

/*
 * Check a hash against an EFI_SIGNATURE_LIST in a buffer
 */
static CHECK_STATUS check_db_hash_in_ram(EFI_SIGNATURE_LIST *CertList,
					 UINTN dbsize, UINT8 *data,
					 int SignatureSize, EFI_GUID CertType)
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

	if (efi_status != EFI_SUCCESS) {
		return VAR_NOT_FOUND;
	}

	CertList = (EFI_SIGNATURE_LIST *)db;

	CHECK_STATUS rc = check_db_hash_in_ram(CertList, dbsize, data,
						SignatureSize, CertType);
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
	EFI_GUID secure_var = EFI_IMAGE_SECURITY_DATABASE_GUID;
	EFI_GUID shim_var = SHIM_LOCK_GUID;
	EFI_SIGNATURE_LIST *dbx = (EFI_SIGNATURE_LIST *)vendor_dbx;

	if (check_db_hash_in_ram(dbx, vendor_dbx_size, sha256hash,
				 SHA256_DIGEST_SIZE, EFI_CERT_SHA256_GUID) ==
				DATA_FOUND)
		return EFI_ACCESS_DENIED;
	if (check_db_hash_in_ram(dbx, vendor_dbx_size, sha1hash,
				 SHA1_DIGEST_SIZE, EFI_CERT_SHA1_GUID) ==
				DATA_FOUND)
		return EFI_ACCESS_DENIED;
	if (cert && check_db_cert_in_ram(dbx, vendor_dbx_size, cert,
					 sha256hash) == DATA_FOUND)
		return EFI_ACCESS_DENIED;

	if (check_db_hash(L"dbx", secure_var, sha256hash, SHA256_DIGEST_SIZE,
			  EFI_CERT_SHA256_GUID) == DATA_FOUND)
		return EFI_ACCESS_DENIED;
	if (check_db_hash(L"dbx", secure_var, sha1hash, SHA1_DIGEST_SIZE,
			  EFI_CERT_SHA1_GUID) == DATA_FOUND)
		return EFI_ACCESS_DENIED;
	if (cert && check_db_cert(L"dbx", secure_var, cert, sha256hash) ==
				DATA_FOUND)
		return EFI_ACCESS_DENIED;
	if (check_db_hash(L"MokListX", shim_var, sha256hash, SHA256_DIGEST_SIZE,
			  EFI_CERT_SHA256_GUID) == DATA_FOUND) {
		return EFI_ACCESS_DENIED;
	}
	if (cert && check_db_cert(L"MokListX", shim_var, cert, sha256hash) ==
				DATA_FOUND) {
		return EFI_ACCESS_DENIED;
	}

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
	EFI_GUID secure_var = EFI_IMAGE_SECURITY_DATABASE_GUID;
	EFI_GUID shim_var = SHIM_LOCK_GUID;

	if (!ignore_db) {
		if (check_db_hash(L"db", secure_var, sha256hash, SHA256_DIGEST_SIZE,
					EFI_CERT_SHA256_GUID) == DATA_FOUND) {
			update_verification_method(VERIFIED_BY_HASH);
			return EFI_SUCCESS;
		}
		if (check_db_hash(L"db", secure_var, sha1hash, SHA1_DIGEST_SIZE,
					EFI_CERT_SHA1_GUID) == DATA_FOUND) {
			verification_method = VERIFIED_BY_HASH;
			update_verification_method(VERIFIED_BY_HASH);
			return EFI_SUCCESS;
		}
		if (cert && check_db_cert(L"db", secure_var, cert, sha256hash)
					== DATA_FOUND) {
			verification_method = VERIFIED_BY_CERT;
			update_verification_method(VERIFIED_BY_CERT);
			return EFI_SUCCESS;
		}
	}

	if (check_db_hash(L"MokList", shim_var, sha256hash, SHA256_DIGEST_SIZE,
			  EFI_CERT_SHA256_GUID) == DATA_FOUND) {
		verification_method = VERIFIED_BY_HASH;
		update_verification_method(VERIFIED_BY_HASH);
		return EFI_SUCCESS;
	}
	if (cert && check_db_cert(L"MokList", shim_var, cert, sha256hash) ==
				DATA_FOUND) {
		verification_method = VERIFIED_BY_CERT;
		update_verification_method(VERIFIED_BY_CERT);
		return EFI_SUCCESS;
	}

	update_verification_method(VERIFIED_BY_NOTHING);
	return EFI_ACCESS_DENIED;
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
		perror(L"shim.c:%d Invalid hash base 0x%016x\n", l,	\
			hashbase);					\
		goto done;						\
	}								\
	if ((unsigned long)hashbase + hashsize >			\
			(unsigned long)data + datasize_in) {		\
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
	EFI_STATUS status = EFI_SUCCESS;
	EFI_IMAGE_DOS_HEADER *DosHdr = (void *)data;
	unsigned int PEHdr_offset = 0;

	if (datasize_in < 0) {
		perror(L"Invalid data size\n");
		return EFI_INVALID_PARAMETER;
	}
	size = datasize = (unsigned int)datasize_in;

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
		status = EFI_OUT_OF_RESOURCES;
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
		status = EFI_OUT_OF_RESOURCES;
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
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash end of certificate table to end of image header */
	EFI_IMAGE_DATA_DIRECTORY *dd = context->SecDir + 1;
	hashbase = (char *)dd;
	hashsize = context->SizeOfHeaders - (unsigned long)((char *)dd - data);
	if (hashsize > datasize_in) {
		perror(L"Data Directory size %d is invalid\n", hashsize);
		status = EFI_INVALID_PARAMETER;
		goto done;
	}
	check_size(data, datasize_in, hashbase, hashsize);

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		perror(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
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
			status = EFI_INVALID_PARAMETER;
			goto done;
		}
		/* Validate section size is within image. */
		if (SectionPtr->SizeOfRawData >
		    datasize - SumOfBytesHashed - SumOfSectionBytes) {
			perror(L"Malformed section %d size\n", index);
			status = EFI_INVALID_PARAMETER;
			goto done;
		}
		SumOfSectionBytes += SectionPtr->SizeOfRawData;
	}

	SectionHeader = (EFI_IMAGE_SECTION_HEADER *) AllocateZeroPool (sizeof (EFI_IMAGE_SECTION_HEADER) * context->PEHdr->Pe32.FileHeader.NumberOfSections);
	if (SectionHeader == NULL) {
		perror(L"Unable to allocate section header\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Already validated above */
	Section = ImageAddress(data, datasize,
		PEHdr_offset +
		sizeof (UINT32) +
		sizeof (EFI_IMAGE_FILE_HEADER) +
		context->PEHdr->Pe32.FileHeader.SizeOfOptionalHeader);

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
			status = EFI_INVALID_PARAMETER;
			goto done;
		}

		/* Verify hashsize within image. */
		if (Section->SizeOfRawData >
		    datasize - Section->PointerToRawData) {
			perror(L"Malformed section raw size %d\n", index);
			status = EFI_INVALID_PARAMETER;
			goto done;
		}
		hashsize  = (unsigned int) Section->SizeOfRawData;
		check_size(data, datasize_in, hashbase, hashsize);

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			perror(L"Unable to generate hash\n");
			status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
		SumOfBytesHashed += Section->SizeOfRawData;
	}

	/* Hash all remaining data */
	if (datasize > SumOfBytesHashed) {
		hashbase = data + SumOfBytesHashed;
		hashsize = datasize - context->SecDir->Size - SumOfBytesHashed;
		check_size(data, datasize_in, hashbase, hashsize);

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			perror(L"Unable to generate hash\n");
			status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
	}

	if (!(Sha256Final(sha256ctx, sha256hash)) ||
	    !(Sha1Final(sha1ctx, sha1hash))) {
		perror(L"Unable to finalise hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

done:
	if (SectionHeader)
		FreePool(SectionHeader);
	if (sha1ctx)
		FreePool(sha1ctx);
	if (sha256ctx)
		FreePool(sha256ctx);

	return status;
}

/*
 * Ensure that the MOK database hasn't been set or modified from an OS
 */
static EFI_STATUS verify_mok (void) {
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS status = EFI_SUCCESS;
	UINT8 *MokListData = NULL;
	UINTN MokListDataSize = 0;
	UINT32 attributes;

	status = get_variable_attr(L"MokList", &MokListData, &MokListDataSize,
				   shim_lock_guid, &attributes);

	if (!EFI_ERROR(status) && attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		perror(L"MokList is compromised!\nErase all keys in MokList!\n");
		if (LibDeleteVariable(L"MokList", &shim_lock_guid) != EFI_SUCCESS) {
			perror(L"Failed to erase MokList\n");
                        return EFI_ACCESS_DENIED;
		}
	}

	if (MokListData)
		FreePool(MokListData);

	return EFI_SUCCESS;
}

/*
 * Check that the signature is valid and matches the binary
 */
static EFI_STATUS verify_buffer (char *data, int datasize,
			 PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	UINT8 sha256hash[SHA256_DIGEST_SIZE];
	UINT8 sha1hash[SHA1_DIGEST_SIZE];
	EFI_STATUS status = EFI_ACCESS_DENIED;
	WIN_CERTIFICATE_EFI_PKCS *cert = NULL;
	unsigned int size = datasize;

	if (context->SecDir->Size != 0) {
		cert = ImageAddress (data, size,
				     context->SecDir->VirtualAddress);

		if (!cert) {
			perror(L"Certificate located outside the image\n");
			return EFI_INVALID_PARAMETER;
		}

		if (cert->Hdr.wCertificateType !=
		    WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
			perror(L"Unsupported certificate type %x\n",
				cert->Hdr.wCertificateType);
			return EFI_UNSUPPORTED;
		}
	}

	status = generate_hash(data, datasize, context, sha256hash, sha1hash);

	if (status != EFI_SUCCESS)
		return status;

	/*
	 * Check that the MOK database hasn't been modified
	 */
	status = verify_mok();
	if (status != EFI_SUCCESS)
		return status;

	/*
	 * Ensure that the binary isn't blacklisted
	 */
	status = check_blacklist(cert, sha256hash, sha1hash);

	if (status != EFI_SUCCESS) {
		perror(L"Binary is blacklisted\n");
		return status;
	}

	/*
	 * Check whether the binary is whitelisted in any of the firmware
	 * databases
	 */
	status = check_whitelist(cert, sha256hash, sha1hash);
	if (status == EFI_SUCCESS)
		return status;

	if (cert) {
		/*
		 * Check against the shim build key
		 */
		if (sizeof(shim_cert) &&
		    AuthenticodeVerify(cert->CertData,
			       context->SecDir->Size - sizeof(cert->Hdr),
			       shim_cert, sizeof(shim_cert), sha256hash,
			       SHA256_DIGEST_SIZE)) {
			status = EFI_SUCCESS;
			return status;
		}

		/*
		 * And finally, check against shim's built-in key
		 */
		if (vendor_cert_size && AuthenticodeVerify(cert->CertData,
							context->SecDir->Size - sizeof(cert->Hdr),
							vendor_cert, vendor_cert_size, sha256hash,
							SHA256_DIGEST_SIZE)) {
			status = EFI_SUCCESS;
			return status;
		}
	}

	status = EFI_ACCESS_DENIED;

	return status;
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
		OptHeaderSize = sizeof(EFI_IMAGE_OPTIONAL_HEADER64);
	} else {
		context->NumberOfRvaAndSizes = PEHdr->Pe32.OptionalHeader.NumberOfRvaAndSizes;
		context->SizeOfHeaders = PEHdr->Pe32.OptionalHeader.SizeOfHeaders;
		context->ImageSize = (UINT64)PEHdr->Pe32.OptionalHeader.SizeOfImage;
		context->SectionAlignment = PEHdr->Pe32.OptionalHeader.SectionAlignment;
		OptHeaderSize = sizeof(EFI_IMAGE_OPTIONAL_HEADER32);
	}

	if (context->SectionAlignment < 0x1000)
		context->SectionAlignment = 0x1000;
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

	if (context->SecDir->VirtualAddress >= datasize) {
		perror(L"Malformed security header\n");
		return EFI_INVALID_PARAMETER;
	}
	return EFI_SUCCESS;
}

/*
 * Once the image has been loaded it needs to be validated and relocated
 */
static EFI_STATUS handle_image (void *data, unsigned int datasize,
				EFI_LOADED_IMAGE *li)
{
	EFI_STATUS efi_status;
	char *buffer;
	int i;
	unsigned int size;
	EFI_IMAGE_SECTION_HEADER *Section;
	char *base, *end;
	PE_COFF_LOADER_IMAGE_CONTEXT context;

	/*
	 * The binary header contains relevant context and section pointers
	 */
	efi_status = read_header(data, datasize, &context);
	if (efi_status != EFI_SUCCESS) {
		perror(L"Failed to read header: %r\n", efi_status);
		return efi_status;
	}

	/*
	 * We only need to verify the binary if we're in secure mode
	 */
	if (secure_mode ()) {
		efi_status = verify_buffer(data, datasize, &context);

		if (EFI_ERROR(efi_status)) {
			console_error(L"Verification failed", efi_status);
			return efi_status;
		} else {
			if (verbose)
				console_notify(L"Verification succeeded");
		}
	}

	buffer = AllocatePool(context.ImageSize + context.SectionAlignment);
	buffer = ALIGN_POINTER(buffer, context.SectionAlignment);

	if (!buffer) {
		perror(L"Failed to allocate image buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	CopyMem(buffer, data, context.SizeOfHeaders);

	char *RelocBase, *RelocBaseEnd;
	RelocBase = ImageAddress(buffer, datasize,
				 context.RelocDir->VirtualAddress);
	/* RelocBaseEnd here is the address of the last byte of the table */
	RelocBaseEnd = ImageAddress(buffer, datasize,
				    context.RelocDir->VirtualAddress +
				    context.RelocDir->Size - 1);

	EFI_IMAGE_SECTION_HEADER *RelocSection = NULL;

	/*
	 * Copy the executable's sections to their desired offsets
	 */
	Section = context.FirstSection;
	for (i = 0; i < context.NumberOfSections; i++, Section++) {
		size = Section->Misc.VirtualSize;

		if (size > Section->SizeOfRawData)
			size = Section->SizeOfRawData;

		base = ImageAddress (buffer, context.ImageSize, Section->VirtualAddress);
		end = ImageAddress (buffer, context.ImageSize, Section->VirtualAddress + size - 1);

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

		if (Section->Characteristics & 0x02000000) {
			/* section has EFI_IMAGE_SCN_MEM_DISCARDABLE attr set */
			continue;
		}

		if (!base || !end) {
			perror(L"Section %d has invalid size\n", i);
			return EFI_UNSUPPORTED;
		}

		if (Section->VirtualAddress < context.SizeOfHeaders ||
				Section->PointerToRawData < context.SizeOfHeaders) {
			perror(L"Section %d is inside image headers\n", i);
			return EFI_UNSUPPORTED;
		}

		if (Section->SizeOfRawData > 0)
			CopyMem(base, data + Section->PointerToRawData, size);

		if (size < Section->Misc.VirtualSize)
			ZeroMem (base + size, Section->Misc.VirtualSize - size);
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

		if (efi_status != EFI_SUCCESS) {
			perror(L"Relocation failed: %r\n", efi_status);
			FreePool(buffer);
			return efi_status;
		}
	}

	entry_point = ImageAddress(buffer, context.ImageSize, context.EntryPoint);
	/*
	 * grub needs to know its location and size in memory, so fix up
	 * the loaded image protocol values
	 */
	li->ImageBase = buffer;
	li->ImageSize = context.ImageSize;

	/* Pass the load options to the second stage loader */
	li->LoadOptions = load_options;
	li->LoadOptionsSize = load_options_size;

	if (!entry_point) {
		perror(L"Invalid entry point\n");
		FreePool(buffer);
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

static int
should_use_fallback(EFI_HANDLE image_handle)
{
	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	EFI_LOADED_IMAGE *li;
	unsigned int pathlen = 0;
	CHAR16 *bootpath = NULL;
	EFI_FILE_IO_INTERFACE *fio = NULL;
	EFI_FILE *vh;
	EFI_FILE *fh;
	EFI_STATUS rc;
	int ret = 0;

	rc = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
				       &loaded_image_protocol, (void **)&li);
	if (EFI_ERROR(rc)) {
		perror(L"Could not get image for bootx64.efi: %r\n", rc);
		return 0;
	}

	bootpath = DevicePathToStr(li->FilePath);

	/* Check the beginning of the string and the end, to avoid
	 * caring about which arch this is. */
	/* I really don't know why, but sometimes bootpath gives us
	 * L"\\EFI\\BOOT\\/BOOTX64.EFI".  So just handle that here...
	 */
	if (StrnCaseCmp(bootpath, L"\\EFI\\BOOT\\BOOT", 14) &&
			StrnCaseCmp(bootpath, L"\\EFI\\BOOT\\/BOOT", 15))
		goto error;

	pathlen = StrLen(bootpath);
	if (pathlen < 5 || StrCaseCmp(bootpath + pathlen - 4, L".EFI"))
		goto error;

	rc = uefi_call_wrapper(BS->HandleProtocol, 3, li->DeviceHandle,
			       &FileSystemProtocol, (void **)&fio);
	if (EFI_ERROR(rc)) {
		perror(L"Could not get fio for li->DeviceHandle: %r\n", rc);
		goto error;
	}

	rc = uefi_call_wrapper(fio->OpenVolume, 2, fio, &vh);
	if (EFI_ERROR(rc)) {
		perror(L"Could not open fio volume: %r\n", rc);
		goto error;
	}

	rc = uefi_call_wrapper(vh->Open, 5, vh, &fh, L"\\EFI\\BOOT" FALLBACK,
			       EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(rc)) {
		/* Do not print the error here - this is an acceptable case
		 * for removable media, where we genuinely don't want
		 * fallback.efi to exist.
		 * Print(L"Could not open \"\\EFI\\BOOT%s\": %d\n", FALLBACK,
		 * 	 rc);
		 */
		uefi_call_wrapper(vh->Close, 1, vh);
		goto error;
	}
	uefi_call_wrapper(fh->Close, 1, fh);
	uefi_call_wrapper(vh->Close, 1, vh);

	ret = 1;
error:
	if (bootpath)
		FreePool(bootpath);

	return ret;
}

/*
 * Generate the path of an executable given shim's path and the name
 * of the executable
 */
static EFI_STATUS generate_path(EFI_LOADED_IMAGE *li, CHAR16 *ImagePath,
				CHAR16 **PathName)
{
	EFI_DEVICE_PATH *devpath;
	unsigned int i;
	int j, last = -1;
	unsigned int pathlen = 0;
	EFI_STATUS efi_status = EFI_SUCCESS;
	CHAR16 *bootpath;

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
	EFI_GUID simple_file_system_protocol = SIMPLE_FILE_SYSTEM_PROTOCOL;
	EFI_GUID file_info_id = EFI_FILE_INFO_ID;
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
	efi_status = uefi_call_wrapper(BS->HandleProtocol, 3, device,
				       &simple_file_system_protocol,
				       (void **)&drive);

	if (efi_status != EFI_SUCCESS) {
		perror(L"Failed to find fs: %r\n", efi_status);
		goto error;
	}

	efi_status = uefi_call_wrapper(drive->OpenVolume, 2, drive, &root);

	if (efi_status != EFI_SUCCESS) {
		perror(L"Failed to open fs: %r\n", efi_status);
		goto error;
	}

	/*
	 * And then open the file
	 */
	efi_status = uefi_call_wrapper(root->Open, 5, root, &grub, PathName,
				       EFI_FILE_MODE_READ, 0);

	if (efi_status != EFI_SUCCESS) {
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
	efi_status = uefi_call_wrapper(grub->GetInfo, 4, grub, &file_info_id,
				       &buffersize, fileinfo);

	if (efi_status == EFI_BUFFER_TOO_SMALL) {
		FreePool(fileinfo);
		fileinfo = AllocatePool(buffersize);
		if (!fileinfo) {
			perror(L"Unable to allocate file info buffer\n");
			efi_status = EFI_OUT_OF_RESOURCES;
			goto error;
		}
		efi_status = uefi_call_wrapper(grub->GetInfo, 4, grub,
					       &file_info_id, &buffersize,
					       fileinfo);
	}

	if (efi_status != EFI_SUCCESS) {
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
	efi_status = uefi_call_wrapper(grub->Read, 3, grub, &buffersize,
				       *data);

	if (efi_status == EFI_BUFFER_TOO_SMALL) {
		FreePool(*data);
		*data = AllocatePool(buffersize);
		efi_status = uefi_call_wrapper(grub->Read, 3, grub,
					       &buffersize, *data);
	}

	if (efi_status != EFI_SUCCESS) {
		perror(L"Unexpected return from initial read: %r, buffersize %x\n", efi_status, buffersize);
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
	EFI_STATUS status = EFI_SUCCESS;
	PE_COFF_LOADER_IMAGE_CONTEXT context;

	loader_is_participating = 1;
	in_protocol = 1;

	if (!secure_mode())
		goto done;

	status = read_header(buffer, size, &context);
	if (status != EFI_SUCCESS)
		goto done;

	status = verify_buffer(buffer, size, &context);
done:
	in_protocol = 0;
	return status;
}

static EFI_STATUS shim_hash (char *data, int datasize,
			     PE_COFF_LOADER_IMAGE_CONTEXT *context,
			     UINT8 *sha256hash, UINT8 *sha1hash)
{
	EFI_STATUS status;

	in_protocol = 1;
	status = generate_hash(data, datasize, context, sha256hash, sha1hash);
	in_protocol = 0;

	return status;
}

static EFI_STATUS shim_read_header(void *data, unsigned int datasize,
				   PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	EFI_STATUS status;

	in_protocol = 1;
	status = read_header(data, datasize, context);
	in_protocol = 0;

	return status;
}

/*
 * Load and run an EFI executable
 */
EFI_STATUS start_image(EFI_HANDLE image_handle, CHAR16 *ImagePath)
{
	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	EFI_STATUS efi_status;
	EFI_LOADED_IMAGE *li, li_bak;
	CHAR16 *PathName = NULL;
	void *sourcebuffer = NULL;
	UINT64 sourcesize = 0;
	void *data = NULL;
	int datasize;

	/*
	 * We need to refer to the loaded image protocol on the running
	 * binary in order to find our path
	 */
	efi_status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
				       &loaded_image_protocol, (void **)&li);

	if (efi_status != EFI_SUCCESS) {
		perror(L"Unable to init protocol\n");
		return efi_status;
	}

	/*
	 * Build a new path from the existing one plus the executable name
	 */
	efi_status = generate_path(li, ImagePath, &PathName);

	if (efi_status != EFI_SUCCESS) {
		perror(L"Unable to generate path %s: %r\n", ImagePath, efi_status);
		goto done;
	}

	if (findNetboot(li->DeviceHandle)) {
		efi_status = parseNetbootinfo(image_handle);
		if (efi_status != EFI_SUCCESS) {
			perror(L"Netboot parsing failed: %r\n", efi_status);
			return EFI_PROTOCOL_ERROR;
		}
		efi_status = FetchNetbootimage(image_handle, &sourcebuffer,
					       &sourcesize);
		if (efi_status != EFI_SUCCESS) {
			perror(L"Unable to fetch TFTP image: %r\n", efi_status);
			return efi_status;
		}
		data = sourcebuffer;
		datasize = sourcesize;
	} else {
		/*
		 * Read the new executable off disk
		 */
		efi_status = load_image(li, &data, &datasize, PathName);

		if (efi_status != EFI_SUCCESS) {
			perror(L"Failed to load image %s: %r\n", PathName, efi_status);
			goto done;
		}
	}

	/*
	 * We need to modify the loaded image protocol entry before running
	 * the new binary, so back it up
	 */
	CopyMem(&li_bak, li, sizeof(li_bak));

	/*
	 * Verify and, if appropriate, relocate and execute the executable
	 */
	efi_status = handle_image(data, datasize, li);

	if (efi_status != EFI_SUCCESS) {
		perror(L"Failed to load image: %r\n", efi_status);
		CopyMem(li, &li_bak, sizeof(li_bak));
		goto done;
	}

	loader_is_participating = 0;

	/*
	 * The binary is trusted and relocated. Run it
	 */
	efi_status = uefi_call_wrapper(entry_point, 2, image_handle, systab);

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

	if (efi_status == EFI_SECURITY_VIOLATION) {
		efi_status = start_image(image_handle, MOK_MANAGER);
		if (efi_status != EFI_SUCCESS) {
			Print(L"start_image() returned %r\n", efi_status);
			uefi_call_wrapper(BS->Stall, 1, 2000000);
			return efi_status;
		}

		efi_status = start_image(image_handle,
					 use_fb ? FALLBACK : second_stage);
	}

	if (efi_status != EFI_SUCCESS) {
		Print(L"start_image() returned %r\n", efi_status);
		uefi_call_wrapper(BS->Stall, 1, 2000000);
	}

	return efi_status;
}

/*
 * Copy the boot-services only MokList variable to the runtime-accessible
 * MokListRT variable. It's not marked NV, so the OS can't modify it.
 */
EFI_STATUS mirror_mok_list()
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINT8 *Data = NULL;
	UINTN DataSize = 0;
	void *FullData = NULL;
	UINTN FullDataSize = 0;
	EFI_SIGNATURE_LIST *CertList = NULL;
	EFI_SIGNATURE_DATA *CertData = NULL;
	uint8_t *p = NULL;

	efi_status = get_variable(L"MokList", &Data, &DataSize, shim_lock_guid);
	if (efi_status != EFI_SUCCESS)
		DataSize = 0;

	if (vendor_cert_size) {
		FullDataSize = DataSize
			     + sizeof (*CertList)
			     + sizeof (EFI_GUID)
			     + vendor_cert_size
			     ;
		FullData = AllocatePool(FullDataSize);
		if (!FullData) {
			perror(L"Failed to allocate space for MokListRT\n");
			return EFI_OUT_OF_RESOURCES;
		}
		p = FullData;

		if (efi_status == EFI_SUCCESS && DataSize > 0) {
			CopyMem(p, Data, DataSize);
			p += DataSize;
		}
		CertList = (EFI_SIGNATURE_LIST *)p;
		p += sizeof (*CertList);
		CertData = (EFI_SIGNATURE_DATA *)p;
		p += sizeof (EFI_GUID);

		CertList->SignatureType = EFI_CERT_X509_GUID;
		CertList->SignatureListSize = vendor_cert_size
					      + sizeof (*CertList)
					      + sizeof (*CertData)
					      -1;
		CertList->SignatureHeaderSize = 0;
		CertList->SignatureSize = vendor_cert_size + sizeof (EFI_GUID);

		CertData->SignatureOwner = SHIM_LOCK_GUID;
		CopyMem(p, vendor_cert, vendor_cert_size);
	} else {
		FullDataSize = DataSize;
		FullData = Data;
	}

	efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokListRT",
				       &shim_lock_guid,
				       EFI_VARIABLE_BOOTSERVICE_ACCESS
				       | EFI_VARIABLE_RUNTIME_ACCESS,
				       FullDataSize, FullData);
	if (efi_status != EFI_SUCCESS) {
		perror(L"Failed to set MokListRT: %r\n", efi_status);
	}

	return efi_status;
}

/*
 * Copy the boot-services only MokListX variable to the runtime-accessible
 * MokListXRT variable. It's not marked NV, so the OS can't modify it.
 */
EFI_STATUS mirror_mok_list_x()
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINT8 *Data = NULL;
	UINTN DataSize = 0;

	efi_status = get_variable(L"MokListX", &Data, &DataSize, shim_lock_guid);
	if (efi_status != EFI_SUCCESS)
		return efi_status;

	efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokListXRT",
				       &shim_lock_guid,
				       EFI_VARIABLE_BOOTSERVICE_ACCESS
				       | EFI_VARIABLE_RUNTIME_ACCESS,
				       DataSize, Data);
	if (efi_status != EFI_SUCCESS) {
		console_error(L"Failed to set MokListRT", efi_status);
	}

	return efi_status;
}

/*
 * Check if a variable exists
 */
static BOOLEAN check_var(CHAR16 *varname)
{
	EFI_STATUS efi_status;
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	UINTN size = sizeof(UINT32);
	UINT32 MokVar;
	UINT32 attributes;

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, varname,
				       &shim_lock_guid, &attributes,
				       &size, (void *)&MokVar);

	if (efi_status == EFI_SUCCESS || efi_status == EFI_BUFFER_TOO_SMALL)
		return TRUE;

	return FALSE;
}

/*
 * If the OS has set any of these variables we need to drop into MOK and
 * handle them appropriately
 */
EFI_STATUS check_mok_request(EFI_HANDLE image_handle)
{
	EFI_STATUS efi_status;

	if (check_var(L"MokNew") || check_var(L"MokSB") ||
	    check_var(L"MokPW") || check_var(L"MokAuth") ||
	    check_var(L"MokDel") || check_var(L"MokDB") ||
	    check_var(L"MokXNew") || check_var(L"MokXDel") ||
	    check_var(L"MokXAuth")) {
		efi_status = start_image(image_handle, MOK_MANAGER);

		if (efi_status != EFI_SUCCESS) {
			perror(L"Failed to start MokManager: %r\n", efi_status);
			return efi_status;
		}
	}

	return EFI_SUCCESS;
}

/*
 * Verify that MokSBState is valid, and if appropriate set insecure mode
 */
static EFI_STATUS check_mok_sb (void)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS status = EFI_SUCCESS;
	UINT8 MokSBState;
	UINTN MokSBStateSize = sizeof(MokSBState);
	UINT32 attributes;

	user_insecure_mode = 0;
	ignore_db = 0;

	status = uefi_call_wrapper(RT->GetVariable, 5, L"MokSBState", &shim_lock_guid,
				   &attributes, &MokSBStateSize, &MokSBState);
	if (status != EFI_SUCCESS)
		return EFI_ACCESS_DENIED;

	/*
	 * Delete and ignore the variable if it's been set from or could be
	 * modified by the OS
	 */
	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		perror(L"MokSBState is compromised! Clearing it\n");
		if (LibDeleteVariable(L"MokSBState", &shim_lock_guid) != EFI_SUCCESS) {
			perror(L"Failed to erase MokSBState\n");
		}
		status = EFI_ACCESS_DENIED;
	} else {
		if (MokSBState == 1) {
			user_insecure_mode = 1;
		}
	}

	return status;
}

/*
 * Verify that MokDBState is valid, and if appropriate set ignore db mode
 */

static EFI_STATUS check_mok_db (void)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS status = EFI_SUCCESS;
	UINT8 MokDBState;
	UINTN MokDBStateSize = sizeof(MokDBStateSize);
	UINT32 attributes;

	status = uefi_call_wrapper(RT->GetVariable, 5, L"MokDBState", &shim_lock_guid,
				   &attributes, &MokDBStateSize, &MokDBState);
	if (status != EFI_SUCCESS)
		return EFI_ACCESS_DENIED;

	ignore_db = 0;

	/*
	 * Delete and ignore the variable if it's been set from or could be
	 * modified by the OS
	 */
	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		perror(L"MokDBState is compromised! Clearing it\n");
		if (LibDeleteVariable(L"MokDBState", &shim_lock_guid) != EFI_SUCCESS) {
			perror(L"Failed to erase MokDBState\n");
		}
		status = EFI_ACCESS_DENIED;
	} else {
		if (MokDBState == 1) {
			ignore_db = 1;
		}
	}

	return status;
}

static EFI_STATUS mok_ignore_db()
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status = EFI_SUCCESS;
	UINT8 Data = 1;
	UINTN DataSize = sizeof(UINT8);

	check_mok_db();

	if (ignore_db) {
		efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokIgnoreDB",
				&shim_lock_guid,
				EFI_VARIABLE_BOOTSERVICE_ACCESS
				| EFI_VARIABLE_RUNTIME_ACCESS,
				DataSize, (void *)&Data);
		if (efi_status != EFI_SUCCESS) {
			perror(L"Failed to set MokIgnoreDB: %r\n", efi_status);
		}
	}

	return efi_status;

}

/*
 * Check the load options to specify the second stage loader
 */
EFI_STATUS set_second_stage (EFI_HANDLE image_handle)
{
	EFI_STATUS status;
	EFI_LOADED_IMAGE *li;
	CHAR16 *start = NULL, *c;
	unsigned int i;
	int remaining_size = 0;
	CHAR16 *loader_str = NULL;
	unsigned int loader_len = 0;

	second_stage = DEFAULT_LOADER;
	load_options = NULL;
	load_options_size = 0;

	status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
				   &LoadedImageProtocol, (void **) &li);
	if (status != EFI_SUCCESS) {
		perror (L"Failed to get load options: %r\n", status);
		return status;
	}

	/* Expect a CHAR16 string with at least one CHAR16 */
	if (li->LoadOptionsSize < 4 || li->LoadOptionsSize % 2 != 0) {
		return EFI_BAD_BUFFER_SIZE;
	}
	c = (CHAR16 *)(li->LoadOptions + (li->LoadOptionsSize - 2));
	if (*c != L'\0') {
		return EFI_BAD_BUFFER_SIZE;
	}

	/*
	 * UEFI shell copies the whole line of the command into LoadOptions.
	 * We ignore the string before the first L' ', i.e. the name of this
	 * program.
	 */
	for (i = 0; i < li->LoadOptionsSize; i += 2) {
		c = (CHAR16 *)(li->LoadOptions + i);
		if (*c == L' ') {
			*c = L'\0';
			start = c + 1;
			remaining_size = li->LoadOptionsSize - i - 2;
			break;
		}
	}

	if (!start || remaining_size <= 0)
		return EFI_SUCCESS;

	for (i = 0; start[i] != '\0'; i++) {
		if (start[i] == L' ' || start[i] == L'\0')
			break;
		loader_len++;
	}

	/*
	 * Setup the name of the alternative loader and the LoadOptions for
	 * the loader
	 */
	if (loader_len > 0) {
		loader_str = AllocatePool((loader_len + 1) * sizeof(CHAR16));
		if (!loader_str) {
			perror(L"Failed to allocate loader string\n");
			return EFI_OUT_OF_RESOURCES;
		}
		for (i = 0; i < loader_len; i++)
			loader_str[i] = start[i];
		loader_str[loader_len] = L'\0';

		second_stage = loader_str;
		load_options = start;
		load_options_size = remaining_size;
	}

	return EFI_SUCCESS;
}

static SHIM_LOCK shim_lock_interface;
static EFI_HANDLE shim_lock_handle;

EFI_STATUS
install_shim_protocols(void)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;

	if (!secure_mode())
		return EFI_SUCCESS;

	/*
	 * Install the protocol
	 */
	efi_status = uefi_call_wrapper(BS->InstallProtocolInterface, 4,
			  &shim_lock_handle, &shim_lock_guid,
			  EFI_NATIVE_INTERFACE, &shim_lock_interface);
	if (EFI_ERROR(efi_status)) {
		console_error(L"Could not install security protocol",
			      efi_status);
		return efi_status;
	}

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
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;

	if (!secure_mode())
		return;

#if defined(OVERRIDE_SECURITY_POLICY)
	/*
	 * Clean up the security protocol hook
	 */
	security_policy_uninstall();
#endif

	/*
	 * If we're back here then clean everything up before exiting
	 */
	uefi_call_wrapper(BS->UninstallProtocolInterface, 3, shim_lock_handle,
			  &shim_lock_guid, &shim_lock_interface);
}

EFI_STATUS
shim_init(void)
{
	EFI_STATUS status = EFI_SUCCESS;
	setup_console(1);
	setup_verbosity();
	dprinta(shim_version);

	/* Set the second stage loader */
	set_second_stage (image_handle);

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

		status = install_shim_protocols();
	}
	return status;
}

void
shim_fini(void)
{
	if (secure_mode()) {
		/*
		 * Remove our protocols
		 */
		uninstall_shim_protocols();

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

	setup_console(0);
}

extern EFI_STATUS
efi_main(EFI_HANDLE passed_image_handle, EFI_SYSTEM_TABLE *passed_systab);

static void
__attribute__((__optimize__("0")))
debug_hook(void)
{
	EFI_GUID guid = SHIM_LOCK_GUID;
	UINT8 *data = NULL;
	UINTN dataSize = 0;
	EFI_STATUS efi_status;
	volatile register UINTN x = 0;
	extern char _text, _data;

	if (x)
		return;

	efi_status = get_variable(L"SHIM_DEBUG", &data, &dataSize, guid);
	if (EFI_ERROR(efi_status)) {
		return;
	}

	Print(L"add-symbol-file "DEBUGDIR
	      L"shim.debug 0x%08x -s .data 0x%08x\n", &_text,
	      &_data);

	Print(L"Pausing for debugger attachment.\n");
	Print(L"To disable this, remove the EFI variable SHIM_DEBUG-%g .\n",
	      &guid);
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
		uefi_call_wrapper(BS->Stall, 1, 5000);
#endif
	}
	x = 1;
}

EFI_STATUS
efi_main (EFI_HANDLE passed_image_handle, EFI_SYSTEM_TABLE *passed_systab)
{
	EFI_STATUS efi_status;

	verification_method = VERIFIED_BY_NOTHING;

	vendor_cert_size = cert_table.vendor_cert_size;
	vendor_dbx_size = cert_table.vendor_dbx_size;
	vendor_cert = (UINT8 *)&cert_table + cert_table.vendor_cert_offset;
	vendor_dbx = (UINT8 *)&cert_table + cert_table.vendor_dbx_offset;

	/*
	 * Set up the shim lock protocol so that grub and MokManager can
	 * call back in and use shim functions
	 */
	shim_lock_interface.Verify = shim_verify;
	shim_lock_interface.Hash = shim_hash;
	shim_lock_interface.Context = shim_read_header;

	systab = passed_systab;
	image_handle = passed_image_handle;

	/*
	 * Ensure that gnu-efi functions are available
	 */
	InitializeLib(image_handle, systab);

	/*
	 * if SHIM_DEBUG is set, wait for a debugger to attach.
	 */
	debug_hook();

	/*
	 * Check whether the user has configured the system to run in
	 * insecure mode
	 */
	check_mok_sb();

	efi_status = shim_init();
	if (EFI_ERROR(efi_status)) {
		Print(L"Something has gone seriously wrong: %r\n", efi_status);
		Print(L"shim cannot continue, sorry.\n");
		systab->BootServices->Stall(5000000);
		systab->RuntimeServices->ResetSystem(EfiResetShutdown,
						     EFI_SECURITY_VIOLATION,
						     0, NULL);
	}

	/*
	 * Tell the user that we're in insecure mode if necessary
	 */
	if (user_insecure_mode) {
		Print(L"Booting in insecure mode\n");
		uefi_call_wrapper(BS->Stall, 1, 2000000);
	}

	/*
	 * Enter MokManager if necessary
	 */
	efi_status = check_mok_request(image_handle);

	/*
	 * Copy the MOK list to a runtime variable so the kernel can
	 * make use of it
	 */
	efi_status = mirror_mok_list();

	efi_status = mirror_mok_list_x();

	/*
	 * Create the runtime MokIgnoreDB variable so the kernel can
	 * make use of it
	 */
	efi_status = mok_ignore_db();

	/*
	 * Hand over control to the second stage bootloader
	 */
	efi_status = init_grub(image_handle);

	shim_fini();
	return efi_status;
}
