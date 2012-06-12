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

#define SECOND_STAGE L"\\grub.efi"

static EFI_SYSTEM_TABLE *systab;
static EFI_STATUS (EFIAPI *entry_point) (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table);

/*
 * The vendor certificate used for validating the second stage loader
 */

#include "cert.h"

static EFI_STATUS get_variable (CHAR16 *name, EFI_GUID guid,
				unsigned int *size,void **buffer)
{
	EFI_STATUS efi_status;
	UINT32 attributes;
	char allocate = !!(*size);

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, name, &guid,
				       &attributes, size, NULL);

	if (efi_status != EFI_BUFFER_TOO_SMALL || !allocate)
		return efi_status;

	if (allocate)
		*buffer = AllocatePool(*size);

	if (!*buffer) {
		Print(L"Unable to allocate variable buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	efi_status = uefi_call_wrapper(RT->GetVariable, 5, name, &guid,
				       &attributes, size, *buffer);

	return efi_status;
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

/*
 * Check that the signature is valid and matches the binary
 */
static EFI_STATUS verify_buffer (char *data, int datasize,
				 PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	unsigned int size = datasize;
	unsigned int ctxsize;
	void *ctx = NULL;
	UINT8 sb, setupmode;
	UINT8 hash[SHA256_DIGEST_SIZE];
	EFI_STATUS status = EFI_ACCESS_DENIED;
	char *hashbase;
	unsigned int hashsize;
	WIN_CERTIFICATE_EFI_PKCS *cert;
	unsigned int SumOfBytesHashed, SumOfSectionBytes;
	unsigned int index, pos, charsize = sizeof(char);
	EFI_IMAGE_SECTION_HEADER  *Section;
	EFI_IMAGE_SECTION_HEADER  *SectionHeader = NULL;
	EFI_IMAGE_SECTION_HEADER  *SectionCache;
	EFI_GUID global_var = EFI_GLOBAL_VARIABLE;

	status = get_variable(L"SecureBoot", global_var, &charsize, (void *)&sb);

	/* FIXME - more paranoia here? */
	if (status != EFI_SUCCESS || sb != 1) {
		status = EFI_SUCCESS;
		goto done;
	}

	status = get_variable(L"SetupMode", global_var, &charsize, (void *)&setupmode);

	if (status == EFI_SUCCESS && setupmode == 1) {
		goto done;
	}

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

	ctxsize = Sha256GetContextSize();
	ctx = AllocatePool(ctxsize);

	if (!ctx) {
		Print(L"Unable to allocate memory for hash context\n");
		return EFI_OUT_OF_RESOURCES;
	}

	if (!Sha256Init(ctx)) {
		Print(L"Unable to initialise hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash start to checksum */
	hashbase = data;
	hashsize = (char *)&context->PEHdr->Pe32.OptionalHeader.CheckSum -
		hashbase;

	if (!(Sha256Update(ctx, hashbase, hashsize))) {
		Print(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash post-checksum to start of certificate table */
	hashbase = (char *)&context->PEHdr->Pe32.OptionalHeader.CheckSum +
		sizeof (int);
	hashsize = (char *)context->SecDir - hashbase;

	if (!(Sha256Update(ctx, hashbase, hashsize))) {
		Print(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Hash end of certificate table to end of image header */
	hashbase = (char *) &context->PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1];
	hashsize = context->PEHdr->Pe32Plus.OptionalHeader.SizeOfHeaders -
		(int) ((char *) (&context->PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1]) - data);

	if (!(Sha256Update(ctx, hashbase, hashsize))) {
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

		if (!(Sha256Update(ctx, hashbase, hashsize))) {
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

		if (!(Sha256Update(ctx, hashbase, hashsize))) {
			Print(L"Unable to generate hash\n");
			status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
	}

	if (!(Sha256Final(ctx, hash))) {
		Print(L"Unable to finalise hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	if (!AuthenticodeVerify(cert->CertData,
				context->SecDir->Size - sizeof(cert->Hdr),
				vendor_cert, sizeof(cert), hash,
				SHA256_DIGEST_SIZE)) {
		Print(L"Invalid signature\n");
		status = EFI_ACCESS_DENIED;
	} else {
		status = EFI_SUCCESS;
	}

done:
	if (SectionHeader)
		FreePool(SectionHeader);
	if (ctx)
		FreePool(ctx);

	return status;
}

/*
 * Read the binary header and grab appropriate information from it
 */
static EFI_STATUS read_header(void *data,
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

	if (context->SecDir->VirtualAddress >= context->ImageSize) {
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
static EFI_STATUS handle_grub (void *data, int datasize)
{
	EFI_STATUS efi_status;
	char *buffer;
	int i, size;
	EFI_IMAGE_SECTION_HEADER *Section;
	char *base, *end;
	PE_COFF_LOADER_IMAGE_CONTEXT context;

	efi_status = read_header(data, &context);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to read header\n");
		return efi_status;
	}

	efi_status = verify_buffer(data, datasize, &context);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Verification failed\n");
		return efi_status;
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
	if (!entry_point) {
		Print(L"Invalid entry point\n");
		FreePool(buffer);
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

/*
 * Locate the second stage bootloader and read it into a buffer
 */
static EFI_STATUS load_grub (EFI_HANDLE image_handle, void **data,
			     int *datasize)
{
	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	EFI_GUID simple_file_system_protocol = SIMPLE_FILE_SYSTEM_PROTOCOL;
	EFI_GUID file_info_id = EFI_FILE_INFO_ID;
	EFI_STATUS efi_status;
	EFI_DEVICE_PATH *devpath;
	EFI_FILE_INFO *fileinfo = NULL;
	EFI_LOADED_IMAGE *li;
	EFI_FILE_IO_INTERFACE *drive;
	EFI_FILE *root, *grub;
	FILEPATH_DEVICE_PATH *FilePath;
	CHAR16 *PathName = NULL;
	EFI_HANDLE device;
	unsigned int buffersize = sizeof(EFI_FILE_INFO);
	unsigned int pathlen = 0;
	int len;

	efi_status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
				       &loaded_image_protocol, &li);

	if (efi_status != EFI_SUCCESS)
		return efi_status;

	device = li->DeviceHandle;
	devpath = li->FilePath;

	while (!IsDevicePathEnd(devpath) &&
	       !IsDevicePathEnd(NextDevicePathNode(devpath))) {
		FilePath = (FILEPATH_DEVICE_PATH *)devpath;
		len = StrLen(FilePath->PathName);

		pathlen += len;

		if (len == 1 && FilePath->PathName[0] == '\\') {
			devpath = NextDevicePathNode(devpath);
			continue;
		}

		/* If no leading \, need to add one */
		if (FilePath->PathName[0] != '\\')
			pathlen++;

		/* If trailing \, need to strip it */
		if (FilePath->PathName[len-1] == '\\')
			pathlen--;

		devpath = NextDevicePathNode(devpath);
	}

	PathName = AllocatePool(pathlen + StrLen(SECOND_STAGE));
	PathName[0] = '\0';

	if (!PathName) {
		Print(L"Failed to allocate path buffer\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto error;
	}

	devpath = li->FilePath;

	while (!IsDevicePathEnd(devpath) &&
	       !IsDevicePathEnd(NextDevicePathNode(devpath))) {
		CHAR16 *tmpbuffer;
		FilePath = (FILEPATH_DEVICE_PATH *)devpath;
		len = StrLen(FilePath->PathName);

		if (len == 1 && FilePath->PathName[0] == '\\') {
			devpath = NextDevicePathNode(devpath);
			continue;
		}

		tmpbuffer = AllocatePool(len + 1);

		if (!tmpbuffer) {
			Print(L"Unable to allocate temporary buffer\n");
			return EFI_OUT_OF_RESOURCES;
		}

		StrCpy(tmpbuffer, FilePath->PathName);

		/* If no leading \, need to add one */
		if (tmpbuffer[0] != '\\')
			StrCat(PathName, L"\\");

		/* If trailing \, need to strip it */
		if (tmpbuffer[len-1] == '\\')
			tmpbuffer[len=1] = '\0';

		StrCat(PathName, tmpbuffer);
		FreePool(tmpbuffer);
		devpath = NextDevicePathNode(devpath);
	}

	StrCat(PathName, SECOND_STAGE);

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

EFI_STATUS shim_verify (void *buffer, int size)
{
	EFI_STATUS status;
	PE_COFF_LOADER_IMAGE_CONTEXT context;

	status = read_header(buffer, &context);

	if (status != EFI_SUCCESS)
		return status;

	status = verify_buffer(buffer, size, &context);

	return status;
}

EFI_STATUS efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *passed_systab)
{
	EFI_STATUS efi_status;
	EFI_GUID shim_lock_guid = SHIM_LOCK_GUID;
	void *data = NULL;
	int datasize;
	static SHIM_LOCK shim_lock_interface;
	EFI_HANDLE handle = NULL;

	shim_lock_interface.Verify = shim_verify;

	systab = passed_systab;

	InitializeLib(image_handle, systab);

	efi_status = uefi_call_wrapper(BS->InstallProtocolInterface, 4,
				       &handle, &shim_lock_guid,
				       EFI_NATIVE_INTERFACE,
				       &shim_lock_interface);

	efi_status = load_grub(image_handle, &data, &datasize);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to load grub\n");
		return efi_status;
	}

	efi_status = handle_grub(data, datasize);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to load grub\n");
		return efi_status;
	}

	return uefi_call_wrapper(entry_point, 3, image_handle, passed_systab);
}
