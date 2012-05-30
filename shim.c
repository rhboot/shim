#include <efi.h>
#include <efilib.h>
#include <Library/BaseCryptLib.h>
#include "PeImage.h"

static EFI_SYSTEM_TABLE *systab;
static EFI_STATUS (EFIAPI *entry_point) (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table);

#include "cert.h"

static EFI_STATUS load_grub (EFI_HANDLE image_handle, void **grubdata,
			     int *grubsize)
{
	EFI_GUID loaded_image_protocol = LOADED_IMAGE_PROTOCOL;
	EFI_GUID simple_file_system_protocol = SIMPLE_FILE_SYSTEM_PROTOCOL;
	EFI_GUID file_info_id = EFI_FILE_INFO_ID;
	EFI_STATUS efi_status;
	EFI_LOADED_IMAGE *li;
	EFI_FILE_IO_INTERFACE *drive;
	EFI_FILE_INFO *fileinfo = NULL;
	EFI_FILE *root, *grub;
	FILEPATH_DEVICE_PATH *FilePath;	
	CHAR16 *PathName;
	CHAR16 *Dir;
	EFI_HANDLE device;
	unsigned int buffersize = sizeof(EFI_FILE_INFO);
	int i;

	efi_status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle,
				       &loaded_image_protocol, &li);

	if (efi_status != EFI_SUCCESS)
		return efi_status;

	if (DevicePathType(li->FilePath) != MEDIA_DEVICE_PATH ||
	    DevicePathSubType(li->FilePath) != MEDIA_FILEPATH_DP)
		return EFI_NOT_FOUND;

	device = li->DeviceHandle;

	efi_status = uefi_call_wrapper(BS->HandleProtocol, 3, device,
				       &simple_file_system_protocol, &drive);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to find fs\n");
		return efi_status;
	}

	efi_status = uefi_call_wrapper(drive->OpenVolume, 2, drive, &root);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to open fs\n");
		return efi_status;
	}

	FilePath = (FILEPATH_DEVICE_PATH *)li->FilePath;

	efi_status = uefi_call_wrapper(root->Open, 5, root, &grub,
				       FilePath->PathName, EFI_FILE_MODE_READ,
				       0);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to open %s - %lx\n", FilePath->PathName,
		      efi_status);
		return efi_status;
	}

	fileinfo = AllocatePool(buffersize);

	if (!fileinfo) {
		Print(L"Unable to allocate info buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	efi_status = uefi_call_wrapper(grub->GetInfo, 4, grub, &file_info_id,
				       &buffersize, fileinfo);

	if (efi_status == EFI_BUFFER_TOO_SMALL) {
		fileinfo = AllocatePool(buffersize);
		if (!fileinfo) {
			Print(L"Unable to allocate info buffer\n");
			return EFI_OUT_OF_RESOURCES;
		}
		efi_status = uefi_call_wrapper(grub->GetInfo, 4, grub,
					       &file_info_id, &buffersize,
					       fileinfo);
	}

	if (efi_status != EFI_SUCCESS) {
		Print(L"Unable to get file info\n");
		return efi_status;
	}

	efi_status = uefi_call_wrapper(grub->Close, 1, grub);

	if (fileinfo->Attribute & EFI_FILE_DIRECTORY) {
		Dir = FilePath->PathName;
	} else {
		for (i=StrLen(FilePath->PathName); i > 0; i--) {
			if (FilePath->PathName[i] == '\\')
				break;
		}

		if (i) {
			Dir = AllocatePool(i * 2);
			CopyMem(Dir, FilePath->PathName, i * 2);
			Dir[i] = '\0';
		} else {
			Dir = AllocatePool(2);
			Dir[0] = '\0';
		}
	}

	PathName = AllocatePool(StrLen(Dir) + StrLen(L"grub.efi"));

	if (!PathName)
		return EFI_LOAD_ERROR;

	StrCpy(PathName, Dir);

	StrCat(PathName, L"grub.efi");

	efi_status = uefi_call_wrapper(root->Open, 5, root, &grub, PathName,
				       EFI_FILE_MODE_READ, 0);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to open %s - %lx\n", PathName, efi_status);
		return efi_status;
	}

	efi_status = uefi_call_wrapper(grub->GetInfo, 4, grub, &file_info_id,
				       &buffersize, fileinfo);

	if (efi_status == EFI_BUFFER_TOO_SMALL) {
		fileinfo = AllocatePool(buffersize);
		if (!fileinfo) {
			Print(L"Unable to allocate info buffer\n");
			return EFI_OUT_OF_RESOURCES;
		}
		efi_status = uefi_call_wrapper(grub->GetInfo, 4, grub,
					       &file_info_id, &buffersize,
					       fileinfo);
	}

	if (efi_status != EFI_SUCCESS) {
		Print(L"Unable to get file info\n");
		return efi_status;
	}

	buffersize = fileinfo->FileSize;
	*grubdata = AllocatePool(buffersize);

	if (!*grubdata) {
		Print(L"Unable to allocate file buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	efi_status = uefi_call_wrapper(grub->Read, 3, grub, &buffersize,
				       *grubdata);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Unexpected return from initial read: %x, buffersize %x\n", efi_status, buffersize);
		return efi_status;
	}

	if (efi_status != EFI_SUCCESS) {
		Print(L"Unable to read grub\n");
		return efi_status;
	}

	*grubsize = buffersize;

	return EFI_SUCCESS;
}

static EFI_STATUS read_header(void *grubdata, PE_COFF_LOADER_IMAGE_CONTEXT *context)
{
	EFI_IMAGE_DOS_HEADER *DosHdr = grubdata;
	EFI_IMAGE_OPTIONAL_HEADER_UNION *PEHdr = grubdata;

	if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE)
		PEHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((char *)grubdata + DosHdr->e_lfanew);

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

static void *ImageAddress (void *image, int size, unsigned int address)
{
	if (address > size)
		return NULL;

	return image + address;
}

static EFI_STATUS verify_grub (PE_COFF_LOADER_IMAGE_CONTEXT *context, char *grubdata, int grubsize)
{
	unsigned int size = grubsize;
	unsigned int ctxsize;
	void *ctx;
	UINT8 hash[SHA256_DIGEST_SIZE];
	EFI_STATUS status = EFI_ACCESS_DENIED;
	char *hashbase;
	unsigned int hashsize;
	WIN_CERTIFICATE_EFI_PKCS *cer;
	unsigned int SumOfBytesHashed, SumOfSectionBytes;
	unsigned int index, pos;
	EFI_IMAGE_SECTION_HEADER  *Section;
	EFI_IMAGE_SECTION_HEADER  *SectionHeader;
	EFI_IMAGE_SECTION_HEADER  *SectionCache;

	cert = ImageAddress (grubdata, size, context->SecDir->VirtualAddress);

	if (cer->Hdr.wCertificateType != WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
		Print(L"Unsupported certificate type %x\n",
		      cert->Hdr.wCertificateType);
		return EFI_UNSUPPORTED;
	}

	/* Check which kind of hash */

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
	hashbase = grubdata;
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
		(int) ((char *) (&context->PEHdr->Pe32Plus.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1]) - grubdata);
	if (!(Sha256Update(ctx, hashbase, hashsize))) {
		Print(L"Unable to generate hash\n");
		status = EFI_OUT_OF_RESOURCES;
		goto done;
	}

	/* Sort sections and hash SizeOfRawData of each section */
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

	if (SumOfSectionBytes >= grubsize) {
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
		hashbase  = ImageAddress(grubdata, size, Section->PointerToRawData);
		hashsize  = (unsigned int) Section->SizeOfRawData;

		if (!(Sha256Update(ctx, hashbase, hashsize))) {
			Print(L"Unable to generate hash\n");
			status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
		SumOfBytesHashed += Section->SizeOfRawData;
	}

	/* Hash all remaining data */
	if (size > SumOfBytesHashed) {
		hashbase = grubdata + SumOfBytesHashed;
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

	if (!AuthenticodeVerify(WinCertificate->CertData,
		          context->SecDir->Size - sizeof(WinCertificate->Hdr),
			  cert, sizeof(cert), hash,
			  SHA256_DIGEST_SIZE)) {
		Print(L"Invalid signature\n");
		status = EFI_ACCESS_DENIED;
	} else {
		status = EFI_SUCCESS;
	}

done:
	if (ctx)
		FreePool(ctx);

	return status;
}

static EFI_STATUS relocate_grub (PE_COFF_LOADER_IMAGE_CONTEXT *context, void *grubdata)
{
	EFI_IMAGE_BASE_RELOCATION *RelocBase, *RelocBaseEnd;
	UINT64 Adjust;
	UINT16 *Reloc, *RelocEnd;
	char *Fixup, *FixupBase, *FixupData = NULL;
	UINT16 *Fixup16;
	UINT32 *Fixup32;
	UINT64 *Fixup64;
	int size = context->ImageSize;
	void *ImageEnd = (char *)grubdata + size;

	context->PEHdr->Pe32Plus.OptionalHeader.ImageBase = (UINT64)grubdata;

	if (context->NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
		Print(L"Image has no relocation entry\n");
		return EFI_UNSUPPORTED;
	}

	RelocBase = ImageAddress(grubdata, size, context->RelocDir->VirtualAddress);
	RelocBaseEnd = ImageAddress(grubdata, size, context->RelocDir->VirtualAddress + context->RelocDir->Size - 1);

	if (!RelocBase || !RelocBaseEnd) {
		Print(L"Reloc table overflows binary\n");
		return EFI_UNSUPPORTED;
	}

	Adjust = (UINT64)grubdata - context->ImageAddress;

	while (RelocBase < RelocBaseEnd) {
		Reloc = (UINT16 *) ((char *) RelocBase + sizeof (EFI_IMAGE_BASE_RELOCATION));
		RelocEnd = (UINT16 *) ((char *) RelocBase + RelocBase->SizeOfBlock);

		if ((void *)RelocEnd < grubdata || (void *)RelocEnd > ImageEnd) {
			Print(L"Reloc entry overflows binary\n");
			return EFI_UNSUPPORTED;
		}

		FixupBase = ImageAddress(grubdata, size, RelocBase->VirtualAddress);
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

static EFI_STATUS handle_grub (void *grubdata, int grubsize)
{
	EFI_STATUS efi_status;
	char *buffer;
	int i, size;
	EFI_IMAGE_SECTION_HEADER *Section;
	char *base, *end;
	PE_COFF_LOADER_IMAGE_CONTEXT context;

	efi_status = read_header(grubdata, &context);
	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to read header\n");
		return efi_status;
	}

	efi_status = verify_grub(&context, grubdata, grubsize);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Verification failed\n");
		return efi_status;
	}

	buffer = AllocatePool(context.ImageSize);

	if (!buffer) {
		Print(L"Failed to allocate image buffer\n");
		return EFI_OUT_OF_RESOURCES;
	}

	CopyMem(buffer, grubdata, context.SizeOfHeaders);

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
			CopyMem(base, grubdata + Section->PointerToRawData, size);

		if (size < Section->Misc.VirtualSize)
			ZeroMem (base + size, Section->Misc.VirtualSize - size);

		Section += 1;
	}

	efi_status = relocate_grub(&context, buffer);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Relocation failed\n");
		return efi_status;
	}

	entry_point = ImageAddress(buffer, context.ImageSize, context.EntryPoint);
	if (!entry_point) {
		Print(L"Invalid entry point\n");
		return EFI_UNSUPPORTED;
	}

	return EFI_SUCCESS;
}

EFI_STATUS efi_main (EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *passed_systab)
{
	EFI_STATUS efi_status;
	void *grubdata;
	int grubsize;

	systab = passed_systab;

	InitializeLib(image_handle, systab);

	efi_status = load_grub(image_handle, &grubdata, &grubsize);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to load grub\n");
		return efi_status;
	}

	efi_status = handle_grub(grubdata, grubsize);

	if (efi_status != EFI_SUCCESS) {
		Print(L"Failed to load grub\n");
		return efi_status;
	}

	return uefi_call_wrapper(entry_point, 3, image_handle, passed_systab);
}
