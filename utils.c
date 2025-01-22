// SPDX-License-Identifier: BSD-2-Clause-Patent

#include "shim.h"

EFI_STATUS
get_file_size(EFI_FILE_HANDLE fh, UINTN *retsize)
{
	EFI_STATUS efi_status;
	void *buffer = NULL;
	UINTN bs = 0;

	/* The API here is "Call it once with bs=0, it fills in bs,
	 * then allocate a buffer and ask again to get it filled. */
	efi_status = fh->GetInfo(fh, &EFI_FILE_INFO_GUID, &bs, NULL);
	if (EFI_ERROR(efi_status) && efi_status != EFI_BUFFER_TOO_SMALL)
		return efi_status;
	if (bs == 0)
		return EFI_SUCCESS;

	buffer = AllocateZeroPool(bs);
	if (!buffer) {
		console_print(L"Could not allocate memory\n");
		return EFI_OUT_OF_RESOURCES;
	}
	efi_status = fh->GetInfo(fh, &EFI_FILE_INFO_GUID, &bs, buffer);
	/* This checks *either* the error from the first GetInfo, if it isn't
	 * the EFI_BUFFER_TOO_SMALL we're expecting, or the second GetInfo
	 * call in *any* case. */
	if (EFI_ERROR(efi_status)) {
		console_print(L"Could not get file info: %r\n", efi_status);
		if (buffer)
			FreePool(buffer);
		return efi_status;
	}
	EFI_FILE_INFO *fi = buffer;
	*retsize = fi->FileSize;
	FreePool(buffer);
	return EFI_SUCCESS;
}

EFI_STATUS
read_file(EFI_FILE_HANDLE fh, CHAR16 *fullpath, CHAR16 **buffer, UINT64 *bs)
{
	EFI_FILE_HANDLE fh2;
	EFI_STATUS efi_status;

	efi_status = fh->Open(fh, &fh2, fullpath, EFI_FILE_READ_ONLY, 0);
	if (EFI_ERROR(efi_status)) {
		console_print(L"Couldn't open \"%s\": %r\n", fullpath, efi_status);
		return efi_status;
	}

	UINTN len = 0;
	CHAR16 *b = NULL;
	efi_status = get_file_size(fh2, &len);
	if (EFI_ERROR(efi_status)) {
		console_print(L"Could not get file size for \"%s\": %r\n",
			      fullpath, efi_status);
		fh2->Close(fh2);
		return efi_status;
	}

	if (len > 1024 * PAGE_SIZE) {
		fh2->Close(fh2);
		return EFI_BAD_BUFFER_SIZE;
	}

	b = AllocateZeroPool(len + 2);
	if (!b) {
		console_print(L"Could not allocate memory\n");
		fh2->Close(fh2);
		return EFI_OUT_OF_RESOURCES;
	}

	efi_status = fh->Read(fh, &len, b);
	if (EFI_ERROR(efi_status)) {
		FreePool(b);
		fh2->Close(fh2);
		console_print(L"Could not read file: %r\n", efi_status);
		return efi_status;
	}
	*buffer = b;
	*bs = len;
	fh2->Close(fh2);
	return EFI_SUCCESS;
}
