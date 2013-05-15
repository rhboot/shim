/*
 * Copyright 2012-2013 Red Hat, Inc.
 * All rights reserved.
 *
 * See "COPYING" for license terms.
 *
 * Author(s): Peter Jones <pjones@redhat.com>
 */

#include <efi.h>
#include <efilib.h>

#include "ucs2.h"

EFI_LOADED_IMAGE *this_image = NULL;

static EFI_STATUS
get_file_size(EFI_FILE_HANDLE fh, UINT64 *retsize)
{
	EFI_STATUS rc;
	void *buffer = NULL;
	UINTN bs = 0;
	EFI_GUID finfo = EFI_FILE_INFO_ID;

	/* The API here is "Call it once with bs=0, it fills in bs,
	 * then allocate a buffer and ask again to get it filled. */
	rc = uefi_call_wrapper(fh->GetInfo, 4, fh, &finfo, &bs, NULL);
	if (rc == EFI_BUFFER_TOO_SMALL) {
		buffer = AllocateZeroPool(bs);
		if (!buffer) {
			Print(L"Could not allocate memory\n");
			return EFI_OUT_OF_RESOURCES;
		}
		rc = uefi_call_wrapper(fh->GetInfo, 4, fh, &finfo,
					&bs, buffer);
	}
	/* This checks *either* the error from the first GetInfo, if it isn't
	 * the EFI_BUFFER_TOO_SMALL we're expecting, or the second GetInfo call
	 * in *any* case. */
	if (EFI_ERROR(rc)) {
		Print(L"Could not get file info: %d\n", rc);
		if (buffer)
			FreePool(buffer);
		return rc;
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
	EFI_STATUS rc = uefi_call_wrapper(fh->Open, 5, fh, &fh2, fullpath,
				EFI_FILE_READ_ONLY, 0);
	if (EFI_ERROR(rc)) {
		Print(L"Couldn't open \"%s\": %d\n", fullpath, rc);
		return rc;
	}

	UINT64 len = 0;
	CHAR16 *b = NULL;
	rc = get_file_size(fh2, &len);
	if (EFI_ERROR(rc)) {
		uefi_call_wrapper(fh2->Close, 1, fh2);
		return rc;
	}

	b = AllocateZeroPool(len + 2);
	if (!buffer) {
		Print(L"Could not allocate memory\n");
		uefi_call_wrapper(fh2->Close, 1, fh2);
		return EFI_OUT_OF_RESOURCES;
	}

	rc = uefi_call_wrapper(fh->Read, 3, fh, &len, b);
	if (EFI_ERROR(rc)) {
		FreePool(buffer);
		uefi_call_wrapper(fh2->Close, 1, fh2);
		Print(L"Could not read file: %d\n", rc);
		return rc;
	}
	*buffer = b;
	*bs = len;
	uefi_call_wrapper(fh2->Close, 1, fh2);
	return EFI_SUCCESS;
}

EFI_STATUS
make_full_path(CHAR16 *dirname, CHAR16 *filename, CHAR16 **out, UINT64 *outlen)
{
	UINT64 len;
	
	len = StrLen(dirname) + StrLen(filename) + StrLen(L"\\EFI\\\\") + 2;

	CHAR16 *fullpath = AllocateZeroPool(len*sizeof(CHAR16));
	if (!fullpath) {
		Print(L"Could not allocate memory\n");
		return EFI_OUT_OF_RESOURCES;
	}

	StrCat(fullpath, L"\\EFI\\");
	StrCat(fullpath, dirname);
	StrCat(fullpath, L"\\");
	StrCat(fullpath, filename);

	*out = fullpath;
	*outlen = len;
	return EFI_SUCCESS;
}

CHAR16 *bootorder = NULL;
int nbootorder = 0;

EFI_DEVICE_PATH *first_new_option = NULL;
VOID *first_new_option_args = NULL;
UINTN first_new_option_size = 0;

EFI_STATUS
add_boot_option(EFI_DEVICE_PATH *dp, CHAR16 *filename, CHAR16 *label, CHAR16 *arguments)
{
	static int i = 0;
	CHAR16 varname[] = L"Boot0000";
	CHAR16 hexmap[] = L"0123456789ABCDEF";
	EFI_GUID global = EFI_GLOBAL_VARIABLE;
	EFI_STATUS rc;

	for(; i <= 0xffff; i++) {
		varname[4] = hexmap[(i & 0xf000) >> 12];
		varname[5] = hexmap[(i & 0x0f00) >> 8];
		varname[6] = hexmap[(i & 0x00f0) >> 4];
		varname[7] = hexmap[(i & 0x000f) >> 0];

		void *var = LibGetVariable(varname, &global);
		if (!var) {
			int size = sizeof(UINT32) + sizeof (UINT16) +
				StrLen(label)*2 + 2 + DevicePathSize(dp) +
				StrLen(arguments) * 2 + 2;

			CHAR8 *data = AllocateZeroPool(size);
			CHAR8 *cursor = data;
			*(UINT32 *)cursor = LOAD_OPTION_ACTIVE;
			cursor += sizeof (UINT32);
			*(UINT16 *)cursor = DevicePathSize(dp);
			cursor += sizeof (UINT16);
			StrCpy((CHAR16 *)cursor, label);
			cursor += StrLen(label)*2 + 2;
			CopyMem(cursor, dp, DevicePathSize(dp));
			cursor += DevicePathSize(dp);
			StrCpy((CHAR16 *)cursor, arguments);

			Print(L"Creating boot entry \"%s\" with label \"%s\" "
					L"for file \"%s\"\n",
				varname, label, filename);
			rc = uefi_call_wrapper(RT->SetVariable, 5, varname,
				&global, EFI_VARIABLE_NON_VOLATILE |
					 EFI_VARIABLE_BOOTSERVICE_ACCESS |
					 EFI_VARIABLE_RUNTIME_ACCESS,
				size, data);

			FreePool(data);

			if (EFI_ERROR(rc)) {
				Print(L"Could not create variable: %d\n", rc);
				return rc;
			}

			CHAR16 *newbootorder = AllocateZeroPool(sizeof (CHAR16)
							* (nbootorder + 1));
			if (!newbootorder)
				return EFI_OUT_OF_RESOURCES;

			int j = 0;
			if (nbootorder) {
				for (j = 0; j < nbootorder; j++)
					newbootorder[j] = bootorder[j];
				FreePool(bootorder);
			}
			newbootorder[j] = i & 0xffff;
			bootorder = newbootorder;
			nbootorder += 1;
#ifdef DEBUG_FALLBACK
			Print(L"nbootorder: %d\nBootOrder: ", nbootorder);
			for (j = 0 ; j < nbootorder ; j++)
				Print(L"%04x ", bootorder[j]);
			Print(L"\n");
#endif

			return EFI_SUCCESS;
		}
	}
	return EFI_OUT_OF_RESOURCES;
}

EFI_STATUS
update_boot_order(void)
{
	CHAR16 *oldbootorder;
	UINTN size;
	EFI_GUID global = EFI_GLOBAL_VARIABLE;
	CHAR16 *newbootorder = NULL;

	oldbootorder = LibGetVariableAndSize(L"BootOrder", &global, &size);
	if (oldbootorder) {
		int n = size / sizeof (CHAR16) + nbootorder;

		newbootorder = AllocateZeroPool(n * sizeof (CHAR16));
		if (!newbootorder)
			return EFI_OUT_OF_RESOURCES;
		CopyMem(newbootorder, bootorder, nbootorder * sizeof (CHAR16));
		CopyMem(newbootorder + nbootorder, oldbootorder, size);
		size = n * sizeof (CHAR16);
	} else {
		size = nbootorder * sizeof(CHAR16);
		newbootorder = AllocateZeroPool(size);
		if (!newbootorder)
			return EFI_OUT_OF_RESOURCES;
		CopyMem(newbootorder, bootorder, size);
	}

#ifdef DEBUG_FALLBACK
	Print(L"nbootorder: %d\nBootOrder: ", size / sizeof (CHAR16));
	int j;
	for (j = 0 ; j < size / sizeof (CHAR16); j++)
		Print(L"%04x ", newbootorder[j]);
	Print(L"\n");
#endif

	if (oldbootorder) {
		LibDeleteVariable(L"BootOrder", &global);
		FreePool(oldbootorder);
	}

	EFI_STATUS rc;
	rc = uefi_call_wrapper(RT->SetVariable, 5, L"BootOrder", &global,
					EFI_VARIABLE_NON_VOLATILE |
					 EFI_VARIABLE_BOOTSERVICE_ACCESS |
					 EFI_VARIABLE_RUNTIME_ACCESS,
					size, newbootorder);
	FreePool(newbootorder);
	return rc;
}

EFI_STATUS
add_to_boot_list(EFI_FILE_HANDLE fh, CHAR16 *dirname, CHAR16 *filename, CHAR16 *label, CHAR16 *arguments)
{
	CHAR16 *fullpath = NULL;
	UINT64 pathlen = 0;
	EFI_STATUS rc = EFI_SUCCESS;

	rc = make_full_path(dirname, filename, &fullpath, &pathlen);
	if (EFI_ERROR(rc))
		return rc;
	
	EFI_DEVICE_PATH *dph = NULL, *dpf = NULL, *dp = NULL;
	
	dph = DevicePathFromHandle(this_image->DeviceHandle);
	if (!dph) {
		rc = EFI_OUT_OF_RESOURCES;
		goto err;
	}

	dpf = FileDevicePath(fh, fullpath);
	if (!dpf) {
		rc = EFI_OUT_OF_RESOURCES;
		goto err;
	}

	dp = AppendDevicePath(dph, dpf);
	if (!dp) {
		rc = EFI_OUT_OF_RESOURCES;
		goto err;
	}

#ifdef DEBUG_FALLBACK
	UINTN s = DevicePathSize(dp);
	int i;
	UINT8 *dpv = (void *)dp;
	for (i = 0; i < s; i++) {
		if (i > 0 && i % 16 == 0)
			Print(L"\n");
		Print(L"%02x ", dpv[i]);
	}
	Print(L"\n");

	CHAR16 *dps = DevicePathToStr(dp);
	Print(L"device path: \"%s\"\n", dps);
#endif
	if (!first_new_option) {
		CHAR16 *dps = DevicePathToStr(dp);
		Print(L"device path: \"%s\"\n", dps);
		first_new_option = DuplicateDevicePath(dp);
		first_new_option_args = arguments;
		first_new_option_size = StrLen(arguments) * sizeof (CHAR16);
	}

	add_boot_option(dp, fullpath, label, arguments);

err:
	if (dpf)
		FreePool(dpf);
	if (dp)
		FreePool(dp);
	if (fullpath)
		FreePool(fullpath);
	return rc;
}

EFI_STATUS
populate_stanza(EFI_FILE_HANDLE fh, CHAR16 *dirname, CHAR16 *filename, CHAR16 *csv)
{
#ifdef DEBUG_FALLBACK
	Print(L"CSV data: \"%s\"\n", csv);
#endif
	CHAR16 *file = csv;

	UINTN comma0 = StrCSpn(csv, L",");
	if (comma0 == 0)
		return EFI_INVALID_PARAMETER;
	file[comma0] = L'\0';
#ifdef DEBUG_FALLBACK
	Print(L"filename: \"%s\"\n", file);
#endif

	CHAR16 *label = csv + comma0 + 1;
	UINTN comma1 = StrCSpn(label, L",");
	if (comma1 == 0)
		return EFI_INVALID_PARAMETER;
	label[comma1] = L'\0';
#ifdef DEBUG_FALLBACK
	Print(L"label: \"%s\"\n", label);
#endif

	CHAR16 *arguments = csv + comma0 +1 + comma1 +1;
	UINTN comma2 = StrCSpn(arguments, L",");
	arguments[comma2] = L'\0';
	/* This one is optional, so don't check if comma2 is 0 */
#ifdef DEBUG_FALLBACK
	Print(L"arguments: \"%s\"\n", arguments);
#endif

	add_to_boot_list(fh, dirname, file, label, arguments);

	return EFI_SUCCESS;
}

EFI_STATUS
try_boot_csv(EFI_FILE_HANDLE fh, CHAR16 *dirname, CHAR16 *filename)
{
	CHAR16 *fullpath = NULL;
	UINT64 pathlen = 0;
	EFI_STATUS rc;

	rc = make_full_path(dirname, filename, &fullpath, &pathlen);
	if (EFI_ERROR(rc))
		return rc;

#ifdef DEBUG_FALLBACK
	Print(L"Found file \"%s\"\n", fullpath);
#endif

	CHAR16 *buffer;
	UINT64 bs;
	rc = read_file(fh, fullpath, &buffer, &bs);
	if (EFI_ERROR(rc)) {
		Print(L"Could not read file \"%s\": %d\n", fullpath, rc);
		FreePool(fullpath);
		return rc;
	}
	FreePool(fullpath);

#ifdef DEBUG_FALLBACK
	Print(L"File looks like:\n%s\n", buffer);
#endif

	CHAR16 *start = buffer;
	/* The file may or may not start with the Unicode byte order marker.
	 * Sadness ensues.  Since UEFI is defined as LE, I'm going to decree
	 * that these files must also be LE.
	 *
	 * IT IS THUS SO.
	 *
	 * But if we find the LE byte order marker, just skip it.
	 */
	if (*start == 0xfeff)
		start++;
	while (*start) {
		while (*start == L'\r' || *start == L'\n')
			start++;
		UINTN l = StrCSpn(start, L"\r\n");
		if (l == 0) {
			if (start[l] == L'\0')
				break;
			start++;
			continue;
		}
		CHAR16 c = start[l];
		start[l] = L'\0';

		populate_stanza(fh, dirname, filename, start);

		start[l] = c;
		start += l;
	}

	FreePool(buffer);
	return EFI_SUCCESS;
}

EFI_STATUS
find_boot_csv(EFI_FILE_HANDLE fh, CHAR16 *dirname)
{
	EFI_STATUS rc;
	void *buffer = NULL;
	UINTN bs = 0;
	EFI_GUID finfo = EFI_FILE_INFO_ID;

	/* The API here is "Call it once with bs=0, it fills in bs,
	 * then allocate a buffer and ask again to get it filled. */
	rc = uefi_call_wrapper(fh->GetInfo, 4, fh, &finfo, &bs, NULL);
	if (rc == EFI_BUFFER_TOO_SMALL) {
		buffer = AllocateZeroPool(bs);
		if (!buffer) {
			Print(L"Could not allocate memory\n");
			return EFI_OUT_OF_RESOURCES;
		}
		rc = uefi_call_wrapper(fh->GetInfo, 4, fh, &finfo,
					&bs, buffer);
	}
	/* This checks *either* the error from the first GetInfo, if it isn't
	 * the EFI_BUFFER_TOO_SMALL we're expecting, or the second GetInfo call
	 * in *any* case. */
	if (EFI_ERROR(rc)) {
		Print(L"Could not get info for \"%s\": %d\n", dirname, rc);
		if (buffer)
			FreePool(buffer);
		return rc;
	}

	EFI_FILE_INFO *fi = buffer;
	if (!(fi->Attribute & EFI_FILE_DIRECTORY)) {
		FreePool(buffer);
		return EFI_SUCCESS;
	}
	FreePool(buffer);

	bs = 0;
	do {
		bs = 0;
		rc = uefi_call_wrapper(fh->Read, 3, fh, &bs, NULL);
		if (rc == EFI_BUFFER_TOO_SMALL) {
			buffer = AllocateZeroPool(bs);
			if (!buffer) {
				Print(L"Could not allocate memory\n");
				return EFI_OUT_OF_RESOURCES;
			}

			rc = uefi_call_wrapper(fh->Read, 3, fh, &bs, buffer);
		}
		if (EFI_ERROR(rc)) {
			Print(L"Could not read \\EFI\\%s\\: %d\n", dirname, rc);
			FreePool(buffer);
			return rc;
		}
		if (bs == 0)
			break;

		fi = buffer;

		if (!StrCaseCmp(fi->FileName, L"boot.csv")) {
			EFI_FILE_HANDLE fh2;
			rc = uefi_call_wrapper(fh->Open, 5, fh, &fh2,
						fi->FileName,
						EFI_FILE_READ_ONLY, 0);
			if (EFI_ERROR(rc) || fh2 == NULL) {
				Print(L"Couldn't open \\EFI\\%s\\%s: %d\n",
					dirname, fi->FileName, rc);
				FreePool(buffer);
				buffer = NULL;
				continue;
			}
			rc = try_boot_csv(fh2, dirname, fi->FileName);
			uefi_call_wrapper(fh2->Close, 1, fh2);
		}

		FreePool(buffer);
		buffer = NULL;
	} while (bs != 0);

	rc = EFI_SUCCESS;

	return rc;
}

EFI_STATUS
find_boot_options(EFI_HANDLE device)
{
	EFI_STATUS rc = EFI_SUCCESS;

	EFI_FILE_IO_INTERFACE *fio = NULL;
	rc = uefi_call_wrapper(BS->HandleProtocol, 3, device,
				&FileSystemProtocol, (void **)&fio);
	if (EFI_ERROR(rc)) {
		Print(L"Couldn't find file system: %d\n", rc);
		return rc;
	}

	/* EFI_FILE_HANDLE is a pointer to an EFI_FILE, and I have
	 * *no idea* what frees the memory allocated here. Hopefully
	 * Close() does. */
	EFI_FILE_HANDLE fh = NULL;
	rc = uefi_call_wrapper(fio->OpenVolume, 2, fio, &fh);
	if (EFI_ERROR(rc) || fh == NULL) {
		Print(L"Couldn't open file system: %d\n", rc);
		return rc;
	}

	EFI_FILE_HANDLE fh2 = NULL;
	rc = uefi_call_wrapper(fh->Open, 5, fh, &fh2, L"EFI",
						EFI_FILE_READ_ONLY, 0);
	if (EFI_ERROR(rc) || fh2 == NULL) {
		Print(L"Couldn't open EFI: %d\n", rc);
		uefi_call_wrapper(fh->Close, 1, fh);
		return rc;
	}
	rc = uefi_call_wrapper(fh2->SetPosition, 2, fh2, 0);
	if (EFI_ERROR(rc)) {
		Print(L"Couldn't set file position: %d\n", rc);
		uefi_call_wrapper(fh2->Close, 1, fh2);
		uefi_call_wrapper(fh->Close, 1, fh);
		return rc;
	}

	void *buffer;
	UINTN bs;
	do {
		bs = 0;
		rc = uefi_call_wrapper(fh2->Read, 3, fh2, &bs, NULL);
		if (rc == EFI_BUFFER_TOO_SMALL ||
				(rc == EFI_SUCCESS && bs != 0)) {
			buffer = AllocateZeroPool(bs);
			if (!buffer) {
				Print(L"Could not allocate memory\n");
				/* sure, this might work, why not? */
				uefi_call_wrapper(fh2->Close, 1, fh2);
				uefi_call_wrapper(fh->Close, 1, fh);
				return EFI_OUT_OF_RESOURCES;
			}

			rc = uefi_call_wrapper(fh2->Read, 3, fh2, &bs, buffer);
		}
		if (bs == 0)
			break;

		if (EFI_ERROR(rc)) {
			Print(L"Could not read \\EFI\\: %d\n", rc);
			if (buffer) {
				FreePool(buffer);
				buffer = NULL;
			}
			uefi_call_wrapper(fh2->Close, 1, fh2);
			uefi_call_wrapper(fh->Close, 1, fh);
			return rc;
		}
		EFI_FILE_INFO *fi = buffer;

		if (!(fi->Attribute & EFI_FILE_DIRECTORY)) {
			FreePool(buffer);
			buffer = NULL;
			continue;
		}
		if (!StrCmp(fi->FileName, L".") ||
				!StrCmp(fi->FileName, L"..") ||
				!StrCaseCmp(fi->FileName, L"BOOT")) {
			FreePool(buffer);
			buffer = NULL;
			continue;
		}
#ifdef DEBUG_FALLBACK
		Print(L"Found directory named \"%s\"\n", fi->FileName);
#endif

		EFI_FILE_HANDLE fh3;
		rc = uefi_call_wrapper(fh->Open, 5, fh2, &fh3, fi->FileName,
						EFI_FILE_READ_ONLY, 0);
		if (EFI_ERROR(rc)) {
			Print(L"%d Couldn't open %s: %d\n", __LINE__, fi->FileName, rc);
			FreePool(buffer);
			buffer = NULL;
			continue;
		}

		rc = find_boot_csv(fh3, fi->FileName);
		FreePool(buffer);
		buffer = NULL;
		if (rc == EFI_OUT_OF_RESOURCES)
			break;

	} while (1);

	if (rc == EFI_SUCCESS && nbootorder > 0)
		rc = update_boot_order();

	uefi_call_wrapper(fh2->Close, 1, fh2);
	uefi_call_wrapper(fh->Close, 1, fh);
	return rc;
}

static EFI_STATUS
try_start_first_option(EFI_HANDLE parent_image_handle)
{
	EFI_STATUS rc;
	EFI_HANDLE image_handle;

	if (!first_new_option) {
		return EFI_SUCCESS;
	}

	rc = uefi_call_wrapper(BS->LoadImage, 6, 0, parent_image_handle,
			       first_new_option, NULL, 0,
			       &image_handle);
	if (EFI_ERROR(rc)) {
		Print(L"LoadImage failed: %d\n", rc);
		uefi_call_wrapper(BS->Stall, 1, 2000000);
		return rc;
	}

	EFI_LOADED_IMAGE *image;
	rc = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &LoadedImageProtocol, (void *)&image);
	if (!EFI_ERROR(rc)) {
		image->LoadOptions = first_new_option_args;
		image->LoadOptionsSize = first_new_option_size;
	}

	rc = uefi_call_wrapper(BS->StartImage, 3, image_handle, NULL, NULL);
	if (EFI_ERROR(rc)) {
		Print(L"StartImage failed: %d\n", rc);
		uefi_call_wrapper(BS->Stall, 1, 2000000);
	}
	return rc;
}

EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	EFI_STATUS rc;

	InitializeLib(image, systab);

	rc = uefi_call_wrapper(BS->HandleProtocol, 3, image, &LoadedImageProtocol, (void *)&this_image);
	if (EFI_ERROR(rc)) {
		Print(L"Error: could not find loaded image: %d\n", rc);
		return rc;
	}

	Print(L"System BootOrder not found.  Initializing defaults.\n");

	rc = find_boot_options(this_image->DeviceHandle);
	if (EFI_ERROR(rc)) {
		Print(L"Error: could not find boot options: %d\n", rc);
		return rc;
	}

	try_start_first_option(image);

	Print(L"Reset System\n");
	uefi_call_wrapper(RT->ResetSystem, 4, EfiResetCold,
			  EFI_SUCCESS, 0, NULL);

	return EFI_SUCCESS;
}
