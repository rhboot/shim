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
#include "signature.h"

#define SECOND_STAGE L"\\grub.efi"
#define MOK_MANAGER L"\\MokManager.efi"

static EFI_SYSTEM_TABLE *systab;
static EFI_STATUS (EFIAPI *entry_point) (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table);

/*
 * The vendor certificate used for validating the second stage loader
 */
extern UINT8 vendor_cert[];
extern UINT32 vendor_cert_size;

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

static EFI_STATUS get_variable (CHAR16 *name, EFI_GUID guid, UINT32 *attributes,
				UINTN *size, void **buffer)
{
	EFI_STATUS efi_status;
	char allocate = !(*size);

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, name, &guid,
				       attributes, size, buffer);

	if (efi_status != EFI_BUFFER_TOO_SMALL || !allocate) {
		return efi_status;
	}

	if (allocate)
		*buffer = AllocatePool(*size);

	if (!*buffer) {
		Print(L"Unable to allocate variable buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, name, &guid,
				       attributes, size, *buffer);

	return efi_status;
}

static MokListNode *build_mok_list(UINT32 num, void *Data, UINTN DataSize) {
	MokListNode *list;
	int i, remain = DataSize;
	void *ptr;

	if (DataSize < sizeof(UINT32))
		return NULL;

	list = AllocatePool(sizeof(MokListNode) * num);

	if (!list) {
		Print(L"Unable to allocate MOK list\n");
		return NULL;
	}

	ptr = Data;
	for (i = 0; i < num; i++) {
		CopyMem(&list[i].MokSize, ptr, sizeof(UINT32));
		remain -= sizeof(UINT32) + list[i].MokSize;

		if (remain < 0) {
			Print(L"MOK list was corrupted\n");
			FreePool(list);
			return NULL;
		}

		ptr += sizeof(UINT32);
		list[i].Mok = ptr;
		ptr += list[i].MokSize;
	}

	return list;
}

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

	while (RelocBase < RelocBaseEnd) {
		Reloc = (UINT16 *) ((char *) RelocBase + sizeof (EFI_IMAGE_BASE_RELOCATION));
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

static CHECK_STATUS check_db_cert(CHAR16 *dbname, WIN_CERTIFICATE_EFI_PKCS *data, UINT8 *hash)
{
	EFI_STATUS efi_status;
	EFI_GUID secure_var = EFI_IMAGE_SECURITY_DATABASE_GUID;
	EFI_SIGNATURE_LIST *CertList;
	EFI_SIGNATURE_DATA *Cert;
	UINTN dbsize = 0;
	UINTN CertCount, Index;
	UINT32 attributes;
	BOOLEAN IsFound = FALSE;
	void *db;
	EFI_GUID CertType = EfiCertX509Guid;

	efi_status = get_variable(dbname, secure_var, &attributes, &dbsize, &db);

	if (efi_status != EFI_SUCCESS)
		return VAR_NOT_FOUND;

	CertList = db;

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		if (CompareGuid (&CertList->SignatureType, &CertType) == 0) {
			CertCount = (CertList->SignatureListSize - CertList->SignatureHeaderSize) / CertList->SignatureSize;
			Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
			for (Index = 0; Index < CertCount; Index++) {
				IsFound = AuthenticodeVerify (data->CertData,
							      data->Hdr.dwLength - sizeof(data->Hdr),
							      Cert->SignatureData,
							      CertList->SignatureSize,
							      hash, SHA256_DIGEST_SIZE);
				if (IsFound)
					break;
			}

			Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
		}

		dbsize -= CertList->SignatureListSize;
		CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
	}

	FreePool(db);

	if (IsFound)
		return DATA_FOUND;

	return DATA_NOT_FOUND;
}

static CHECK_STATUS check_db_hash(CHAR16 *dbname, UINT8 *data,
				  int SignatureSize, EFI_GUID CertType)
{
	EFI_STATUS efi_status;
	EFI_GUID secure_var = EFI_IMAGE_SECURITY_DATABASE_GUID;
	EFI_SIGNATURE_LIST *CertList;
	EFI_SIGNATURE_DATA *Cert;
	UINTN dbsize = 0;
	UINTN CertCount, Index;
	UINT32 attributes;
	BOOLEAN IsFound = FALSE;
	void *db;

	efi_status = get_variable(dbname, secure_var, &attributes, &dbsize, &db);

	if (efi_status != EFI_SUCCESS) {
		return VAR_NOT_FOUND;
	}

	CertList = db;

	while ((dbsize > 0) && (dbsize >= CertList->SignatureListSize)) {
		CertCount = (CertList->SignatureListSize - CertList->SignatureHeaderSize) / CertList->SignatureSize;
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

	FreePool(db);

	if (IsFound)
		return DATA_FOUND;

	return DATA_NOT_FOUND;
}

static EFI_STATUS check_blacklist (WIN_CERTIFICATE_EFI_PKCS *cert,
				   UINT8 *sha256hash, UINT8 *sha1hash)
{
	if (check_db_hash(L"db", sha256hash, SHA256_DIGEST_SIZE,
			  EfiHashSha256Guid) == DATA_FOUND)
		return EFI_ACCESS_DENIED;
	if (check_db_hash(L"db", sha1hash, SHA1_DIGEST_SIZE,
			  EfiHashSha1Guid) == DATA_FOUND)
		return EFI_ACCESS_DENIED;
	if (check_db_cert(L"dbx", cert, sha256hash) == DATA_FOUND)
		return EFI_ACCESS_DENIED;

	return EFI_SUCCESS;
}

static EFI_STATUS check_whitelist (WIN_CERTIFICATE_EFI_PKCS *cert,
				   UINT8 *sha256hash, UINT8 *sha1hash)
{
	if (check_db_hash(L"db", sha256hash, SHA256_DIGEST_SIZE,
			  EfiHashSha256Guid) == DATA_FOUND)
		return EFI_SUCCESS;
	if (check_db_hash(L"db", sha1hash, SHA1_DIGEST_SIZE,
			  EfiHashSha1Guid) == DATA_FOUND)
		return EFI_SUCCESS;
	if (check_db_cert(L"db", cert, sha256hash) == DATA_FOUND)
		return EFI_SUCCESS;

	return EFI_ACCESS_DENIED;
}

/*
 * Check whether we're in Secure Boot and user mode
 */

static BOOLEAN secure_mode (void)
{
	EFI_STATUS status;
	EFI_GUID global_var = EFI_GLOBAL_VARIABLE;
	UINTN charsize = sizeof(char);
	UINT8 sb, setupmode;
	UINT32 attributes;

	status = get_variable(L"SecureBoot", global_var, &attributes, &charsize,
			      (void *)&sb);

	/* FIXME - more paranoia here? */
	if (status != EFI_SUCCESS || sb != 1) {
		Print(L"Secure boot not enabled\n");
		return FALSE;
	}

	status = get_variable(L"SetupMode", global_var, &attributes, &charsize,
			      (void *)&setupmode);

	if (status == EFI_SUCCESS && setupmode == 1) {
		Print(L"Platform is in setup mode\n");
		return FALSE;
	}

	return TRUE;
}

/*
 * Check that the signature is valid and matches the binary
 */
static EFI_STATUS verify_buffer (char *data, int datasize,
			 PE_COFF_LOADER_IMAGE_CONTEXT *context, int whitelist)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	unsigned int size = datasize;
	unsigned int sha256ctxsize, sha1ctxsize;
	void *sha256ctx = NULL, *sha1ctx = NULL;
	UINT8 sha256hash[SHA256_DIGEST_SIZE];
	UINT8 sha1hash[SHA1_DIGEST_SIZE];
	EFI_STATUS status = EFI_ACCESS_DENIED;
	char *hashbase;
	unsigned int hashsize;
	WIN_CERTIFICATE_EFI_PKCS *cert;
	unsigned int SumOfBytesHashed, SumOfSectionBytes;
	unsigned int index, pos;
	EFI_IMAGE_SECTION_HEADER  *Section;
	EFI_IMAGE_SECTION_HEADER  *SectionHeader = NULL;
	EFI_IMAGE_SECTION_HEADER  *SectionCache;
	unsigned int i;
	void *MokListData = NULL;
	UINTN MokListDataSize = 0;
	UINT32 MokNum, attributes;
	MokListNode *list = NULL;

	cert = ImageAddress (data, size, context->SecDir->VirtualAddress);

	if (!cert) {
		Print(L"Certificate located outside the image\n");
		return EFI_INVALID_PARAMETER;
	}

	if (cert->Hdr.wCertificateType != WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
		Print(L"Unsupported certificate type %x\n",
		      cert->Hdr.wCertificateType);
		return EFI_UNSUPPORTED;
	}

	/* FIXME: Check which kind of hash */

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
			return EFI_INVALID_PARAMETER;
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

	status = check_blacklist(cert, sha256hash, sha1hash);

	if (status != EFI_SUCCESS) {
		Print(L"Binary is blacklisted\n");
		goto done;
	}

	if (whitelist) {
		status = check_whitelist(cert, sha256hash, sha1hash);

		if (status == EFI_SUCCESS) {
			Print(L"Binary is whitelisted\n");
			goto done;
		}
	}

	if (AuthenticodeVerify(cert->CertData,
			       context->SecDir->Size - sizeof(cert->Hdr),
			       vendor_cert, vendor_cert_size, sha256hash,
			       SHA256_DIGEST_SIZE)) {
		status = EFI_SUCCESS;
		Print(L"Binary is verified by the vendor certificate\n");
		goto done;
	}

	status = get_variable(L"MokList", shim_lock_guid, &attributes,
			      &MokListDataSize, &MokListData);

	if (status != EFI_SUCCESS || MokListDataSize < sizeof(UINT32)) {
		status = EFI_ACCESS_DENIED;
		Print(L"Invalid signature\n");
		goto done;
	}

	if (attributes & EFI_VARIABLE_RUNTIME_ACCESS) {
		Print(L"MokList is compromised!\nErase all keys in MokList!\n");
		if (LibDeleteVariable(L"MokList", &shim_lock_guid) != EFI_SUCCESS) {
			Print(L"Failed to erase MokList\n");
		}
		status = EFI_ACCESS_DENIED;
		goto done;
	}

	CopyMem(&MokNum, MokListData, sizeof(UINT32));
	if (MokNum == 0) {
		status = EFI_ACCESS_DENIED;
		goto done;
	}

	list = build_mok_list(MokNum,
			      (void *)MokListData + sizeof(UINT32),
			      MokListDataSize - sizeof(UINT32));

	if (!list) {
		Print(L"Failed to construct MOK list\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	for (i = 0; i < MokNum; i++) {
		if (AuthenticodeVerify(cert->CertData,
				       context->SecDir->Size - sizeof(cert->Hdr),
				       list[i].Mok, list[i].MokSize, sha256hash,
				       SHA256_DIGEST_SIZE)) {
			status = EFI_SUCCESS;
			Print(L"Binary is verified by the machine owner key\n");
			goto done;
		}
	}
	Print(L"Invalid signature\n");
	status = EFI_ACCESS_DENIED;

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
 * Read the binary header and grab appropriate information from it
 */
static EFI_STATUS read_header(void *data, unsigned int datasize,
			      PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	EFI_IMAGE_DOS_HEADER *DosHdr = data;
	EFI_IMAGE_OPTIONAL_HEADER_UNION *PEHdr = data;

	if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE)
		PEHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((char *)data + DosHdr->e_lfanew);

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

	if (context->SecDir->VirtualAddress >= datasize) {
		Print(L"Malformed security header\n");
		return EFI_INVALID_PARAMETER;
	}

	if (context->SecDir->Size == 0) {
		Print(L"Empty security header\n");
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

	efi_status = read_header(data, datasize, &context);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to read header\n");
		return efi_status;
	}

	if (secure_mode ()) {
		efi_status = verify_buffer(data, datasize, &context, 0);

		if (efi_status != EFI_SUCCESS) {
			Print(L"Verification failed\n");
			return efi_status;
		}
	}

	buffer = AllocatePool(context.ImageSize);

	if (!buffer) {
		Print(L"Failed to allocate image buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	CopyMem(buffer, data, context.SizeOfHeaders);

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

		if (Section->SizeOfRawData > 0)
			CopyMem(base, data + Section->PointerToRawData, size);

		if (size < Section->Misc.VirtualSize)
			ZeroMem (base + size, Section->Misc.VirtualSize - size);

		Section += 1;
	}

	efi_status = relocate_coff(&context, buffer);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Relocation failed\n");
		FreePool(buffer);
		return efi_status;
	}

	entry_point = ImageAddress(buffer, context.ImageSize, context.EntryPoint);
	li->ImageBase = buffer;
	li->ImageSize = context.ImageSize;

	if (!entry_point) {
		Print(L"Invalid entry point\n");
		FreePool(buffer);
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

static EFI_STATUS generate_path(EFI_LOADED_IMAGE *li, CHAR16 *ImagePath,
				EFI_DEVICE_PATH **grubpath, CHAR16 **PathName)
{
	EFI_DEVICE_PATH *devpath;
	EFI_HANDLE device;
	int i;
	unsigned int pathlen = 0;
	EFI_STATUS efi_status = EFI_SUCCESS;
	CHAR16 *bootpath;

	device = li->DeviceHandle;
	devpath = li->FilePath;

	bootpath = DevicePathToStr(devpath);

	pathlen = StrLen(bootpath);

	for (i=pathlen; i>0; i--) {
		if (bootpath[i] == '\\')
			break;
	}

	bootpath[i+1] = '\0';

	if (bootpath[i-i] == '\\')
		bootpath[i] = '\0';

	*PathName = AllocatePool(StrSize(bootpath) + StrSize(ImagePath));

	if (!*PathName) {
		Print(L"Failed to allocate path buffer\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto error;
	}

	*PathName[0] = '\0';
	StrCat(*PathName, bootpath);
	StrCat(*PathName, ImagePath);

	*grubpath = FileDevicePath(device, *PathName);

error:
	return efi_status;
}

/*
 * Locate the second stage bootloader and read it into a buffer
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

	efi_status = uefi_call_wrapper(BS->HandleProtocol, 3, device,
				       &simple_file_system_protocol, &drive);	

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to find fs\n");
		goto error;
	}

	efi_status = uefi_call_wrapper(drive->OpenVolume, 2, drive, &root);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to open fs\n");
		goto error;
	}

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

	efi_status = uefi_call_wrapper(grub->GetInfo, 4, grub, &file_info_id,
				       &buffersize, fileinfo);

	if (efi_status == EFI_BUFFER_TOO_SMALL) {
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

	return EFI_SUCCESS;
error:
	if (*data) {
		FreePool(*data);
		*data = NULL;
	}
	if (PathName)
		FreePool(PathName);
	if (fileinfo)
		FreePool(fileinfo);
	return efi_status;
}

EFI_STATUS shim_verify (void *buffer, UINT32 size)
{
	EFI_STATUS status;
	PE_COFF_LOADER_IMAGE_CONTEXT context;

	if (!secure_mode())
		return EFI_SUCCESS;

	status = read_header(buffer, size, &context);

	if (status != EFI_SUCCESS)
		return status;

	status = verify_buffer(buffer, size, &context, 1);

	return status;
}

EFI_STATUS start_image(EFI_HANDLE image_handle, CHAR16 *ImagePath)
{
	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	EFI_STATUS efi_status;
	EFI_LOADED_IMAGE *li, li_bak;
	EFI_HANDLE handle = NULL;
	EFI_DEVICE_PATH *path;
	CHAR16 *PathName;
	void *data = NULL;
	int datasize;

	efi_status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
				       &loaded_image_protocol, &li);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Unable to init protocol\n");
		return efi_status;
	}

	efi_status = generate_path(li, ImagePath, &path, &PathName);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Unable to generate path: %s\n", ImagePath);
		goto done;
	}

	efi_status = uefi_call_wrapper(BS->LoadImage, 6, FALSE, image_handle,
				       path, NULL, 0, &handle);

	if (efi_status == EFI_SUCCESS) {
		/* Image validates - start it */
		Print(L"Starting file via StartImage\n");
		efi_status = uefi_call_wrapper(BS->StartImage, 3, handle, NULL,
					       NULL);
		uefi_call_wrapper(BS->UnloadImage, 1, handle);
		goto done;
	}

	efi_status = load_image(li, &data, &datasize, PathName);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to load image\n");
		goto done;
	}

	CopyMem(&li_bak, li, sizeof(li_bak));

	efi_status = handle_image(data, datasize, li);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to load image\n");
		CopyMem(li, &li_bak, sizeof(li_bak));
		goto done;
	}

	efi_status = uefi_call_wrapper(entry_point, 3, image_handle, systab);

	CopyMem(li, &li_bak, sizeof(li_bak));
done:
	return efi_status;
}

EFI_STATUS init_grub(EFI_HANDLE image_handle)
{
	EFI_STATUS efi_status;

	efi_status = start_image(image_handle, SECOND_STAGE);

	if (efi_status != EFI_SUCCESS) {
		if (efi_status == EFI_ACCESS_DENIED)
			efi_status = start_image(image_handle, MOK_MANAGER);
		else
			Print(L"Failed to start grub\n");
	}
done:

	return efi_status;
}

EFI_STATUS mirror_mok_list()
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINT32 attributes;
	void *Data = NULL;
	UINTN DataSize = 0;

	efi_status = get_variable(L"MokList", shim_lock_guid, &attributes,
				  &DataSize, &Data);

	if (efi_status != EFI_SUCCESS) {
		goto done;
	}

	efi_status = uefi_call_wrapper(RT->SetVariable, 5, L"MokListRT",
				       &shim_lock_guid,
				       EFI_VARIABLE_BOOTSERVICE_ACCESS
				       | EFI_VARIABLE_RUNTIME_ACCESS,
				       DataSize, Data);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to set MokListRT %d\n", efi_status);
	}

done:
	return efi_status;
}

EFI_STATUS check_mok_request(EFI_HANDLE image_handle)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	EFI_STATUS efi_status;
	UINTN size = sizeof(UINT32);
	UINT32 MokNew;
	UINT32 attributes;

	if (!secure_mode())
		return EFI_SUCCESS;

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, L"MokNew",
				       &shim_lock_guid, &attributes,
				       &size, (void *)&MokNew);

	if (efi_status != EFI_SUCCESS && efi_status != EFI_BUFFER_TOO_SMALL)
		goto done;

	efi_status = start_image(image_handle, MOK_MANAGER);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to start MokManager\n");
		goto done;
	}

done:
	return efi_status;
}

EFI_STATUS efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *passed_systab)
{
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	static SHIM_LOCK shim_lock_interface;
	EFI_HANDLE handle = NULL;
	EFI_STATUS efi_status;

	shim_lock_interface.Verify = shim_verify;

	systab = passed_systab;

	InitializeLib(image_handle, systab);

	efi_status = check_mok_request(image_handle);

	efi_status = mirror_mok_list();

	uefi_call_wrapper(BS->InstallProtocolInterface, 4, &handle,
			  &shim_lock_guid, EFI_NATIVE_INTERFACE,
			  &shim_lock_interface);

	efi_status = init_grub(image_handle);

	uefi_call_wrapper(BS->UninstallProtocolInterface, 3, handle,
			  &shim_lock_guid, &shim_lock_interface);

	return efi_status;
}
