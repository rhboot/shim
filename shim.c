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
static EFI_STATUS (EFIAPI *entry_point) (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table);

static CHAR16 *second_stage;
static void *load_options;
static UINT32 load_options_size;

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

UINT8 insecure_mode;
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
static void *ImageAddress (void *image, int size, unsigned int address)
{
	if (address > size)
		return NULL;

	return image + address;
}

/*
 * Perform the actual relocation
 */
static EFI_STATUS relocate_coff (PE_COFF_LOADER_IMAGE_CONTEXT *context,
				 void *data)
{
	EFI_IMAGE_BASE_RELOCATION *RelocBase, *RelocBaseEnd;
	UINT64 Adjust;
	UINT16 *Reloc, *RelocEnd;
	char *Fixup, *FixupBase, *FixupData = NULL;
	UINT16 *Fixup16;
	UINT32 *Fixup32;
	UINT64 *Fixup64;
	int size = context->ImageSize;
	void *ImageEnd = (char *)data + size;

	context->PEHdr->Pe32Plus.OptionalHeader.ImageBase = (UINT64)data;

	if (context->NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
		Print(L"Image has no relocation entry\n");
		return EFI_UNSUPPORTED;
	}

	RelocBase = ImageAddress(data, size, context->RelocDir->VirtualAddress);
	RelocBaseEnd = ImageAddress(data, size, context->RelocDir->VirtualAddress + context->RelocDir->Size - 1);

	if (!RelocBase || !RelocBaseEnd) {
		Print(L"Reloc table overflows binary\n");
		return EFI_UNSUPPORTED;
	}

	Adjust = (UINT64)data - context->ImageAddress;

	if (Adjust == 0)
		return EFI_SUCCESS;

	while (RelocBase < RelocBaseEnd) {
		Reloc = (UINT16 *) ((char *) RelocBase + sizeof (EFI_IMAGE_BASE_RELOCATION));

		if ((RelocBase->SizeOfBlock == 0) || (RelocBase->SizeOfBlock > context->RelocDir->Size)) {
			Print(L"Reloc block size is invalid\n");
			return EFI_UNSUPPORTED;
		}

		RelocEnd = (UINT16 *) ((char *) RelocBase + RelocBase->SizeOfBlock);
		if ((void *)RelocEnd < data || (void *)RelocEnd > ImageEnd) {
			Print(L"Reloc entry overflows binary\n");
			return EFI_UNSUPPORTED;
		}

		FixupBase = ImageAddress(data, size, RelocBase->VirtualAddress);
		if (!FixupBase) {
			Print(L"Invalid fixupbase\n");
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
				Print(L"Unknown relocation\n");
				return EFI_UNSUPPORTED;
			}
			Reloc += 1;
		}
		RelocBase = (EFI_IMAGE_BASE_RELOCATION *) RelocEnd;
	}

	return EFI_SUCCESS;
}

static CHECK_STATUS check_db_cert_in_ram(EFI_SIGNATURE_LIST *CertList,
					 UINTN dbsize,
					 WIN_CERTIFICATE_EFI_PKCS *data,
					 UINT8 *hash)
{
	EFI_SIGNATURE_DATA *Cert;
	UINTN CertCount, Index;
	BOOLEAN IsFound = FALSE;
	EFI_GUID CertType = X509_GUID;

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		if (CompareGuid (&CertList->SignatureType, &CertType) == 0) {
			CertCount = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
			Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
			for (Index = 0; Index < CertCount; Index++) {
				IsFound = AuthenticodeVerify (data->CertData,
							      data->Hdr.dwLength - sizeof(data->Hdr),
							      Cert->SignatureData,
							      CertList->SignatureSize,
							      hash, SHA256_DIGEST_SIZE);
				if (IsFound)
					break;

				Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
			}

		}

		if (IsFound)
			break;

		dbsize -= CertList->SignatureListSize;
		CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
	}

	if (IsFound)
		return DATA_FOUND;

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
	EFI_STATUS status;
	EFI_GUID global_var = EFI_GLOBAL_VARIABLE;
	UINTN len;
	UINT8 *Data;
	UINT8 sb, setupmode;

	if (insecure_mode)
		return FALSE;

	status = get_variable(L"SecureBoot", &Data, &len, global_var);
	if (status != EFI_SUCCESS) {
		if (verbose)
			console_notify(L"Secure boot not enabled\n");
		return FALSE;
	}
	sb = *Data;
	FreePool(Data);

	if (sb != 1) {
		if (verbose)
			console_notify(L"Secure boot not enabled\n");
		return FALSE;
	}

	status = get_variable(L"SetupMode", &Data, &len, global_var);
	if (status != EFI_SUCCESS)
		return TRUE;

	setupmode = *Data;
	FreePool(Data);

	if (setupmode == 1) {
		if (verbose)
			console_notify(L"Platform is in setup mode\n");
		return FALSE;
	}

	return TRUE;
}

/*
 * Calculate the SHA1 and SHA256 hashes of a binary
 */

static EFI_STATUS generate_hash (char *data, int datasize,
				 PE_COFF_LOADER_IMAGE_CONTEXT *context,
				 UINT8 *sha256hash, UINT8 *sha1hash)

{
	unsigned int sha256ctxsize, sha1ctxsize;
	unsigned int size = datasize;
	void *sha256ctx = NULL, *sha1ctx = NULL;
	char *hashbase;
	unsigned int hashsize;
	unsigned int SumOfBytesHashed, SumOfSectionBytes;
	unsigned int index, pos;
	EFI_IMAGE_SECTION_HEADER  *Section;
	EFI_IMAGE_SECTION_HEADER  *SectionHeader = NULL;
	EFI_IMAGE_SECTION_HEADER  *SectionCache;
	EFI_STATUS status = EFI_SUCCESS;

	sha256ctxsize = Sha256GetContextSize();
	sha256ctx = AllocatePool(sha256ctxsize);

	sha1ctxsize = Sha1GetContextSize();
	sha1ctx = AllocatePool(sha1ctxsize);

	if (!sha256ctx || !sha1ctx) {
		Print(L"Unable to allocate memory for hash context\n");
		return EFI_OUT_OF_RESOURCES;
	}

	if (!Sha256Init(sha256ctx) || !Sha1Init(sha1ctx)) {
		Print(L"Unable to initialise hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash start to checksum */
	hashbase = data;
	hashsize = (char *)&context->PEHdr->Pe32.OptionalHeader.CheckSum -
		hashbase;

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		Print(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash post-checksum to start of certificate table */
	hashbase = (char *)&context->PEHdr->Pe32.OptionalHeader.CheckSum +
		sizeof (int);
	hashsize = (char *)context->SecDir - hashbase;

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		Print(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash end of certificate table to end of image header */
	hashbase = (char *) &context->PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1];
	hashsize = context->PEHdr->Pe32Plus.OptionalHeader.SizeOfHeaders -
		(int) ((char *) (&context->PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1]) - data);

	if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
	    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
		Print(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Sort sections */
	SumOfBytesHashed = context->PEHdr->Pe32Plus.OptionalHeader.SizeOfHeaders;

	Section = (EFI_IMAGE_SECTION_HEADER *) (
		(char *)context->PEHdr + sizeof (UINT32) +
		sizeof (EFI_IMAGE_FILE_HEADER) +
		context->PEHdr->Pe32.FileHeader.SizeOfOptionalHeader
		);

	SectionCache = Section;

	for (index = 0, SumOfSectionBytes = 0; index < context->PEHdr->Pe32.FileHeader.NumberOfSections; index++, SectionCache++) {
		SumOfSectionBytes += SectionCache->SizeOfRawData;
	}

	if (SumOfSectionBytes >= datasize) {
		Print(L"Malformed binary: %x %x\n", SumOfSectionBytes, size);
		status = EFI_INVALID_PARAMETER;
		goto done;
	}

	SectionHeader = (EFI_IMAGE_SECTION_HEADER *) AllocateZeroPool (sizeof (EFI_IMAGE_SECTION_HEADER) * context->PEHdr->Pe32.FileHeader.NumberOfSections);
	if (SectionHeader == NULL) {
		Print(L"Unable to allocate section header\n");
		status = EFI_OUT_OF_RESOURCES;
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
		hashsize  = (unsigned int) Section->SizeOfRawData;

		if (!hashbase) {
			Print(L"Malformed section header\n");
			status = EFI_INVALID_PARAMETER;
			goto done;
		}

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			Print(L"Unable to generate hash\n");
			status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
		SumOfBytesHashed += Section->SizeOfRawData;
	}

	/* Hash all remaining data */
	if (size > SumOfBytesHashed) {
		hashbase = data + SumOfBytesHashed;
		hashsize = (unsigned int)(
			size -
			context->PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size -
			SumOfBytesHashed);

		if (!(Sha256Update(sha256ctx, hashbase, hashsize)) ||
		    !(Sha1Update(sha1ctx, hashbase, hashsize))) {
			Print(L"Unable to generate hash\n");
			status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
	}

	if (!(Sha256Final(sha256ctx, sha256hash)) ||
	    !(Sha1Final(sha1ctx, sha1hash))) {
		Print(L"Unable to finalise hash\n");
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

	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		Print(L"MokList is compromised!\nErase all keys in MokList!\n");
		if (LibDeleteVariable(L"MokList", &shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to erase MokList\n");
		}
		status = EFI_ACCESS_DENIED;
		return status;
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
			Print(L"Certificate located outside the image\n");
			return EFI_INVALID_PARAMETER;
		}

		if (cert->Hdr.wCertificateType !=
		    WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
			Print(L"Unsupported certificate type %x\n",
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
	verify_mok();

	/*
	 * Ensure that the binary isn't blacklisted
	 */
	status = check_blacklist(cert, sha256hash, sha1hash);

	if (status != EFI_SUCCESS) {
		Print(L"Binary is blacklisted\n");
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
		if (AuthenticodeVerify(cert->CertData,
			       context->SecDir->Size - sizeof(cert->Hdr),
			       shim_cert, sizeof(shim_cert), sha256hash,
			       SHA256_DIGEST_SIZE)) {
			status = EFI_SUCCESS;
			return status;
		}


		/*
		 * And finally, check against shim's built-in key
		 */
		if (AuthenticodeVerify(cert->CertData,
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
	unsigned long HeaderWithoutDataDir, SectionHeaderOffset;

	if (datasize < sizeof(EFI_IMAGE_DOS_HEADER)) {
		Print(L"Invalid image\n");
		return EFI_UNSUPPORTED;
	}

	if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE)
		PEHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((char *)data + DosHdr->e_lfanew);

	if (EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES
			< PEHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes) {
		Print(L"Image header too small\n");
		return EFI_UNSUPPORTED;
	}

	HeaderWithoutDataDir = sizeof (EFI_IMAGE_OPTIONAL_HEADER64)
			- sizeof (EFI_IMAGE_DATA_DIRECTORY) * EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;
	if (((UINT32)PEHdr->Pe32Plus.FileHeader.SizeOfOptionalHeader - HeaderWithoutDataDir) !=
			PEHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes
				* sizeof (EFI_IMAGE_DATA_DIRECTORY)) {
		Print(L"Image header overflows data directory\n");
		return EFI_UNSUPPORTED;
	}

	SectionHeaderOffset = DosHdr->e_lfanew
				+ sizeof (UINT32)
				+ sizeof (EFI_IMAGE_FILE_HEADER)
				+ PEHdr->Pe32Plus.FileHeader.SizeOfOptionalHeader;
	if ((PEHdr->Pe32Plus.OptionalHeader.SizeOfImage - SectionHeaderOffset) / EFI_IMAGE_SIZEOF_SECTION_HEADER
			<= PEHdr->Pe32Plus.FileHeader.NumberOfSections) {
		Print(L"Image sections overflow image size\n");
		return EFI_UNSUPPORTED;
	}

	if ((PEHdr->Pe32Plus.OptionalHeader.SizeOfHeaders - SectionHeaderOffset) / EFI_IMAGE_SIZEOF_SECTION_HEADER
			< (UINT32)PEHdr->Pe32Plus.FileHeader.NumberOfSections) {
		Print(L"Image sections overflow section headers\n");
		return EFI_UNSUPPORTED;
	}

	if ((((UINT8 *)PEHdr - (UINT8 *)data) + sizeof(EFI_IMAGE_OPTIONAL_HEADER_UNION)) > datasize) {
		Print(L"Invalid image\n");
		return EFI_UNSUPPORTED;
	}

	if (PEHdr->Te.Signature != EFI_IMAGE_NT_SIGNATURE) {
		Print(L"Unsupported image type\n");
		return EFI_UNSUPPORTED;
	}

	if (PEHdr->Pe32.FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED) {
		Print(L"Unsupported image - Relocations have been stripped\n");
		return EFI_UNSUPPORTED;
	}

	if (PEHdr->Pe32.OptionalHeader.Magic != EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		Print(L"Only 64-bit images supported\n");
		return EFI_UNSUPPORTED;
	}

	context->PEHdr = PEHdr;
	context->ImageAddress = PEHdr->Pe32Plus.OptionalHeader.ImageBase;
	context->ImageSize = (UINT64)PEHdr->Pe32Plus.OptionalHeader.SizeOfImage;
	context->SizeOfHeaders = PEHdr->Pe32Plus.OptionalHeader.SizeOfHeaders;
	context->EntryPoint = PEHdr->Pe32Plus.OptionalHeader.AddressOfEntryPoint;
	context->RelocDir = &PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
	context->NumberOfRvaAndSizes = PEHdr->Pe32Plus.OptionalHeader.NumberOfRvaAndSizes;
	context->NumberOfSections = PEHdr->Pe32.FileHeader.NumberOfSections;
	context->FirstSection = (EFI_IMAGE_SECTION_HEADER *)((char *)PEHdr + PEHdr->Pe32.FileHeader.SizeOfOptionalHeader + sizeof(UINT32) + sizeof(EFI_IMAGE_FILE_HEADER));
	context->SecDir = (EFI_IMAGE_DATA_DIRECTORY *) &PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];

	if (context->ImageSize < context->SizeOfHeaders) {
		Print(L"Invalid image\n");
		return EFI_UNSUPPORTED;
	}

	if (((UINT8 *)context->SecDir - (UINT8 *)data) > (datasize - sizeof(EFI_IMAGE_DATA_DIRECTORY))) {
		Print(L"Invalid image\n");
		return EFI_UNSUPPORTED;
	}

	if (context->SecDir->VirtualAddress >= datasize) {
		Print(L"Malformed security header\n");
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
	int i, size;
	EFI_IMAGE_SECTION_HEADER *Section;
	char *base, *end;
	PE_COFF_LOADER_IMAGE_CONTEXT context;

	/*
	 * The binary header contains relevant context and section pointers
	 */
	efi_status = read_header(data, datasize, &context);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to read header\n");
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

	buffer = AllocatePool(context.ImageSize);

	if (!buffer) {
		Print(L"Failed to allocate image buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	CopyMem(buffer, data, context.SizeOfHeaders);

	/*
	 * Copy the executable's sections to their desired offsets
	 */
	Section = context.FirstSection;
	for (i = 0; i < context.NumberOfSections; i++) {
		size = Section->Misc.VirtualSize;

		if (size > Section->SizeOfRawData)
			size = Section->SizeOfRawData;

		base = ImageAddress (buffer, context.ImageSize, Section->VirtualAddress);
		end = ImageAddress (buffer, context.ImageSize, Section->VirtualAddress + size - 1);

		if (!base || !end) {
			Print(L"Invalid section size\n");
			return EFI_UNSUPPORTED;
		}

		if (Section->VirtualAddress < context.SizeOfHeaders ||
				Section->PointerToRawData < context.SizeOfHeaders) {
			Print(L"Section is inside image headers\n");
			return EFI_UNSUPPORTED;
		}

		if (Section->SizeOfRawData > 0)
			CopyMem(base, data + Section->PointerToRawData, size);

		if (size < Section->Misc.VirtualSize)
			ZeroMem (base + size, Section->Misc.VirtualSize - size);

		Section += 1;
	}

	/*
	 * Run the relocation fixups
	 */
	efi_status = relocate_coff(&context, buffer);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Relocation failed\n");
		FreePool(buffer);
		return efi_status;
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
		Print(L"Invalid entry point\n");
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
	CHAR16 *bootpath;
	EFI_FILE_IO_INTERFACE *fio = NULL;
	EFI_FILE *vh;
	EFI_FILE *fh;
	EFI_STATUS rc;

	rc = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
				       &loaded_image_protocol, (void **)&li);
	if (EFI_ERROR(rc)) {
		Print(L"Could not get image for bootx64.efi: %d\n", rc);
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
		return 0;

	pathlen = StrLen(bootpath);
	if (pathlen < 5 || StrCaseCmp(bootpath + pathlen - 4, L".EFI"))
		return 0;

	rc = uefi_call_wrapper(BS->HandleProtocol, 3, li->DeviceHandle,
			       &FileSystemProtocol, (void **)&fio);
	if (EFI_ERROR(rc)) {
		Print(L"Could not get fio for li->DeviceHandle: %d\n", rc);
		return 0;
	}
	
	rc = uefi_call_wrapper(fio->OpenVolume, 2, fio, &vh);
	if (EFI_ERROR(rc)) {
		Print(L"Could not open fio volume: %d\n", rc);
		return 0;
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
		return 0;
	}
	uefi_call_wrapper(fh->Close, 1, fh);
	uefi_call_wrapper(vh->Close, 1, vh);

	return 1;
}

/*
 * Generate the path of an executable given shim's path and the name
 * of the executable
 */
static EFI_STATUS generate_path(EFI_LOADED_IMAGE *li, CHAR16 *ImagePath,
				EFI_DEVICE_PATH **grubpath, CHAR16 **PathName)
{
	EFI_DEVICE_PATH *devpath;
	EFI_HANDLE device;
	int i, j, last = -1;
	unsigned int pathlen = 0;
	EFI_STATUS efi_status = EFI_SUCCESS;
	CHAR16 *bootpath;

	device = li->DeviceHandle;
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
		Print(L"Failed to allocate path buffer\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto error;
	}

	*PathName[0] = '\0';
	if (StrnCaseCmp(bootpath, ImagePath, StrLen(bootpath)))
		StrCat(*PathName, bootpath);
	StrCat(*PathName, ImagePath);

	*grubpath = FileDevicePath(device, *PathName);

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
		Print(L"Failed to find fs\n");
		goto error;
	}

	efi_status = uefi_call_wrapper(drive->OpenVolume, 2, drive, &root);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to open fs\n");
		goto error;
	}

	/*
	 * And then open the file
	 */
	efi_status = uefi_call_wrapper(root->Open, 5, root, &grub, PathName,
				       EFI_FILE_MODE_READ, 0);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to open %s - %lx\n", PathName, efi_status);
		goto error;
	}

	fileinfo = AllocatePool(buffersize);

	if (!fileinfo) {
		Print(L"Unable to allocate file info buffer\n");
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
			Print(L"Unable to allocate file info buffer\n");
			efi_status = EFI_OUT_OF_RESOURCES;
			goto error;
		}
		efi_status = uefi_call_wrapper(grub->GetInfo, 4, grub,
					       &file_info_id, &buffersize,
					       fileinfo);
	}

	if (efi_status != EFI_SUCCESS) {
		Print(L"Unable to get file info\n");
		goto error;
	}

	buffersize = fileinfo->FileSize;

	*data = AllocatePool(buffersize);

	if (!*data) {
		Print(L"Unable to allocate file buffer\n");
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
		Print(L"Unexpected return from initial read: %x, buffersize %x\n", efi_status, buffersize);
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
	EFI_STATUS status;
	PE_COFF_LOADER_IMAGE_CONTEXT context;

	loader_is_participating = 1;

	if (!secure_mode())
		return EFI_SUCCESS;

	status = read_header(buffer, size, &context);

	if (status != EFI_SUCCESS)
		return status;

	status = verify_buffer(buffer, size, &context);

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
	EFI_DEVICE_PATH *path;
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
		Print(L"Unable to init protocol\n");
		return efi_status;
	}

	/*
	 * Build a new path from the existing one plus the executable name
	 */
	efi_status = generate_path(li, ImagePath, &path, &PathName);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Unable to generate path: %s\n", ImagePath);
		goto done;
	}

	if (findNetboot(image_handle)) {
		efi_status = parseNetbootinfo(image_handle);
		if (efi_status != EFI_SUCCESS) {
			Print(L"Netboot parsing failed: %d\n", efi_status);
			return EFI_PROTOCOL_ERROR;
		}
		efi_status = FetchNetbootimage(image_handle, &sourcebuffer,
					       &sourcesize);
		if (efi_status != EFI_SUCCESS) {
			Print(L"Unable to fetch TFTP image\n");
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
			Print(L"Failed to load image\n");
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
		Print(L"Failed to load image\n");
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

	if (should_use_fallback(image_handle))
		efi_status = start_image(image_handle, FALLBACK);
	else
		efi_status = start_image(image_handle, second_stage);

	if (efi_status != EFI_SUCCESS)
		efi_status = start_image(image_handle, MOK_MANAGER);

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

	FullDataSize = DataSize
		     + sizeof (*CertList)
		     + sizeof (EFI_GUID)
		     + vendor_cert_size
		     ;
	FullData = AllocatePool(FullDataSize);
	if (!FullData) {
		Print(L"Failed to allocate space for MokListRT\n");
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

	efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokListRT",
				       &shim_lock_guid,
				       EFI_VARIABLE_BOOTSERVICE_ACCESS
				       | EFI_VARIABLE_RUNTIME_ACCESS,
				       FullDataSize, FullData);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to set MokListRT %d\n", efi_status);
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
	    check_var(L"MokDel") || check_var(L"MokDB")) {
		efi_status = start_image(image_handle, MOK_MANAGER);

		if (efi_status != EFI_SUCCESS) {
			Print(L"Failed to start MokManager\n");
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
	UINT8 *MokSBState = NULL;
	UINTN MokSBStateSize = 0;
	UINT32 attributes;

	insecure_mode = 0;
	ignore_db = 0;

	status = get_variable_attr(L"MokSBState", &MokSBState, &MokSBStateSize,
				   shim_lock_guid, &attributes);

	if (status != EFI_SUCCESS)
		return EFI_ACCESS_DENIED;

	/*
	 * Delete and ignore the variable if it's been set from or could be
	 * modified by the OS
	 */
	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		Print(L"MokSBState is compromised! Clearing it\n");
		if (LibDeleteVariable(L"MokSBState", &shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to erase MokSBState\n");
		}
		status = EFI_ACCESS_DENIED;
	} else {
		if (*(UINT8 *)MokSBState == 1) {
			insecure_mode = 1;
		}
	}

	FreePool(MokSBState);

	return status;
}

/*
 * Verify that MokDBState is valid, and if appropriate set ignore db mode
 */

static EFI_STATUS check_mok_db (void)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS status = EFI_SUCCESS;
	UINT8 *MokDBState = NULL;
	UINTN MokDBStateSize = 0;
	UINT32 attributes;

	status = get_variable_attr(L"MokDBState", &MokDBState, &MokDBStateSize,
			shim_lock_guid, &attributes);

	if (status != EFI_SUCCESS)
		return EFI_ACCESS_DENIED;

	ignore_db = 0;

	/*
	 * Delete and ignore the variable if it's been set from or could be
	 * modified by the OS
	 */
	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		Print(L"MokDBState is compromised! Clearing it\n");
		if (LibDeleteVariable(L"MokDBState", &shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to erase MokDBState\n");
		}
		status = EFI_ACCESS_DENIED;
	} else {
		if (*(UINT8 *)MokDBState == 1) {
			ignore_db = 1;
		}
	}

	FreePool(MokDBState);

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
			Print(L"Failed to set MokIgnoreDB %d\n", efi_status);
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
	int i, remaining_size = 0;
	CHAR16 *loader_str = NULL;
	int loader_len = 0;

	second_stage = DEFAULT_LOADER;
	load_options = NULL;
	load_options_size = 0;

	status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
				   &LoadedImageProtocol, (void **) &li);
	if (status != EFI_SUCCESS) {
		Print (L"Failed to get load options\n");
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
			Print(L"Failed to allocate loader string\n");
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

EFI_STATUS efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *passed_systab)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	static SHIM_LOCK shim_lock_interface;
	EFI_HANDLE handle = NULL;
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
	shim_lock_interface.Hash = generate_hash;
	shim_lock_interface.Context = read_header;

	systab = passed_systab;

	/*
	 * Ensure that gnu-efi functions are available
	 */
	InitializeLib(image_handle, systab);

	setup_console(1);
	setup_verbosity();

	dprinta(shim_version);

	/* Set the second stage loader */
	set_second_stage (image_handle);

	/*
	 * Check whether the user has configured the system to run in
	 * insecure mode
	 */
	check_mok_sb();

	/*
	 * Tell the user that we're in insecure mode if necessary
	 */
	if (!secure_mode()) {
		Print(L"Booting in insecure mode\n");
		uefi_call_wrapper(BS->Stall, 1, 2000000);
	} else {
		/*
		 * Install our hooks for ExitBootServices() and StartImage()
		 */
		hook_system_services(systab);
		loader_is_participating = 0;
	}

	/*
	 * Install the protocol
	 */
	efi_status = uefi_call_wrapper(BS->InstallProtocolInterface, 4,
			  &handle, &shim_lock_guid, EFI_NATIVE_INTERFACE,
			  &shim_lock_interface);
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

	/*
	 * Enter MokManager if necessary
	 */
	efi_status = check_mok_request(image_handle);

	/*
	 * Copy the MOK list to a runtime variable so the kernel can make
	 * use of it
	 */
	efi_status = mirror_mok_list();

	/*
	 * Create the runtime MokIgnoreDB variable so the kernel can make
	 * use of it
	 */
	efi_status = mok_ignore_db();

	/*
	 * Hand over control to the second stage bootloader
	 */

	efi_status = init_grub(image_handle);

#if defined(OVERRIDE_SECURITY_POLICY)
	/*
	 * Clean up the security protocol hook
	 */
	security_policy_uninstall();
#endif

	/*
	 * If we're back here then clean everything up before exiting
	 */
	uefi_call_wrapper(BS->UninstallProtocolInterface, 3, handle,
			  &shim_lock_guid, &shim_lock_interface);


	/*
	 * Remove our hooks from system services.
	 */
	unhook_system_services();

	/*
	 * Free the space allocated for the alternative 2nd stage loader
	 */
	if (load_options_size > 0)
		FreePool(second_stage);

	setup_console(0);

	return efi_status;
}
