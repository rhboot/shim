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
#include "variables.h"
#include "tpm.h"

EFI_LOADED_IMAGE *this_image = NULL;

EFI_GUID SHIM_LOCK_GUID = { 0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23} };

int
get_fallback_verbose(void)
{
	EFI_GUID guid = SHIM_LOCK_GUID;
	UINT8 *data = NULL;
	UINTN dataSize = 0;
	EFI_STATUS efi_status;
	unsigned int i;
	static int state = -1;

	if (state != -1)
		return state;

	efi_status = get_variable(L"FALLBACK_VERBOSE",
				  &data, &dataSize, guid);
	if (EFI_ERROR(efi_status)) {
		state = 0;
		return state;
	}

	for (i = 0; i < dataSize; i++) {
		if (data[i]) {
			state = 1;
			return state;
		}
	}

	state = 0;
	return state;
}

#define VerbosePrintUnprefixed(fmt, ...)				\
	({								\
		UINTN ret_ = 0;						\
		if (get_fallback_verbose())				\
			ret_ = Print((fmt), ##__VA_ARGS__);		\
		ret_;							\
	})

#define VerbosePrint(fmt, ...)						\
	({	UINTN line_ = __LINE__;					\
		UINTN ret_ = 0;						\
		if (get_fallback_verbose()) {				\
			Print(L"%a:%d: ", __func__, line_);		\
			ret_ = Print((fmt), ##__VA_ARGS__);		\
		}							\
		ret_;							\
	})

static EFI_STATUS
FindSubDevicePath(EFI_DEVICE_PATH *In, UINT8 Type, UINT8 SubType,
		  EFI_DEVICE_PATH **Out)
{
	EFI_DEVICE_PATH *dp = In;
	if (!In || !Out)
		return EFI_INVALID_PARAMETER;

	CHAR16 *dps = DevicePathToStr(In);
	VerbosePrint(L"input device path: \"%s\"\n", dps);
	FreePool(dps);

	for (dp = In; !IsDevicePathEnd(dp); dp = NextDevicePathNode(dp)) {
		if (DevicePathType(dp) == Type &&
				DevicePathSubType(dp) == SubType) {
			dps = DevicePathToStr(dp);
			VerbosePrint(L"sub-path (%hhd,%hhd): \"%s\"\n",
				     Type, SubType, dps);
			FreePool(dps);

			*Out = DuplicateDevicePath(dp);
			if (!*Out)
				return EFI_OUT_OF_RESOURCES;
			return EFI_SUCCESS;
		}
	}
	*Out = NULL;
	return EFI_NOT_FOUND;
}

static EFI_STATUS
get_file_size(EFI_FILE_HANDLE fh, UINTN *retsize)
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

	UINTN len = 0;
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

	len = StrLen(L"\\EFI\\") + StrLen(dirname)
	    + StrLen(L"\\") + StrLen(filename)
	    + 2;

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
add_boot_option(EFI_DEVICE_PATH *hddp, EFI_DEVICE_PATH *fulldp,
		CHAR16 *filename, CHAR16 *label, CHAR16 *arguments)
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
				StrLen(label)*2 + 2 + DevicePathSize(hddp) +
				StrLen(arguments) * 2;

			CHAR8 *data = AllocateZeroPool(size + 2);
			CHAR8 *cursor = data;
			*(UINT32 *)cursor = LOAD_OPTION_ACTIVE;
			cursor += sizeof (UINT32);
			*(UINT16 *)cursor = DevicePathSize(hddp);
			cursor += sizeof (UINT16);
			StrCpy((CHAR16 *)cursor, label);
			cursor += StrLen(label)*2 + 2;
			CopyMem(cursor, hddp, DevicePathSize(hddp));
			cursor += DevicePathSize(hddp);
			StrCpy((CHAR16 *)cursor, arguments);

			Print(L"Creating boot entry \"%s\" with label \"%s\" "
					L"for file \"%s\"\n",
				varname, label, filename);

			if (!first_new_option) {
				first_new_option = DuplicateDevicePath(fulldp);
				first_new_option_args = arguments;
				first_new_option_size = StrLen(arguments) * sizeof (CHAR16);
			}

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
			newbootorder[0] = i & 0xffff;
			if (nbootorder) {
				for (j = 0; j < nbootorder; j++)
					newbootorder[j+1] = bootorder[j];
				FreePool(bootorder);
			}
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

/*
 * AMI BIOS (e.g, Intel NUC5i3MYHE) may automatically hide and patch BootXXXX
 * variables with ami_masked_device_path_guid. We can get the valid device path
 * if just skipping it and its next end path.
 */

static EFI_GUID ami_masked_device_path_guid = {
	0x99e275e7, 0x75a0, 0x4b37,
	{ 0xa2, 0xe6, 0xc5, 0x38, 0x5e, 0x6c, 0x0, 0xcb }
};

static unsigned int
calc_masked_boot_option_size(unsigned int size)
{
	return size + sizeof(EFI_DEVICE_PATH) +
	       sizeof(ami_masked_device_path_guid) + sizeof(EFI_DEVICE_PATH);
}

static int
check_masked_boot_option(CHAR8 *candidate, unsigned int candidate_size,
			 CHAR8 *data, unsigned int data_size)
{
	/*
	 * The patched BootXXXX variables contain a hardware device path and
	 * an end path, preceding the real device path.
	 */
	if (calc_masked_boot_option_size(data_size) != candidate_size)
		return 1;

	CHAR8 *cursor = candidate;

	/* Check whether the BootXXXX is patched */
	cursor += sizeof(UINT32) + sizeof(UINT16);
	cursor += StrSize((CHAR16 *)cursor);

	unsigned int min_valid_size = cursor - candidate + sizeof(EFI_DEVICE_PATH);

	if (candidate_size <= min_valid_size)
		return 1;

	EFI_DEVICE_PATH *dp = (EFI_DEVICE_PATH *)cursor;
	unsigned int node_size = DevicePathNodeLength(dp) - sizeof(EFI_DEVICE_PATH);

	min_valid_size += node_size;
	if (candidate_size <= min_valid_size ||
	    DevicePathType(dp) != HARDWARE_DEVICE_PATH ||
	    DevicePathSubType(dp) != HW_VENDOR_DP ||
	    node_size != sizeof(ami_masked_device_path_guid) ||
	    CompareGuid((EFI_GUID *)(cursor + sizeof(EFI_DEVICE_PATH)),
		        &ami_masked_device_path_guid))
		return 1;

	/* Check whether the patched guid is followed by an end path */
	min_valid_size += sizeof(EFI_DEVICE_PATH);
	if (candidate_size <= min_valid_size)
		return 1;

	dp = NextDevicePathNode(dp);
	if (!IsDevicePathEnd(dp))
		return 1;

	/*
	 * OK. We may really get a masked BootXXXX variable. The next
	 * step is to test whether it is hidden.
	 */
	UINT32 attrs = *(UINT32 *)candidate;
#ifndef LOAD_OPTION_HIDDEN
#  define LOAD_OPTION_HIDDEN	0x00000008
#endif
        if (!(attrs & LOAD_OPTION_HIDDEN))
		return 1;

	attrs &= ~LOAD_OPTION_HIDDEN;

	/* Compare the field Attributes */
	if (attrs != *(UINT32 *)data)
		return 1;

	/* Compare the field FilePathListLength */
	data += sizeof(UINT32);
	candidate += sizeof(UINT32);
	if (calc_masked_boot_option_size(*(UINT16 *)data) !=
					 *(UINT16 *)candidate)
		return 1;

	/* Compare the field Description */
	data += sizeof(UINT16);
	candidate += sizeof(UINT16);
	if (CompareMem(candidate, data, cursor - candidate))
		return 1;

	/* Compare the filed FilePathList */
	cursor = (CHAR8 *)NextDevicePathNode(dp);
	data += sizeof(UINT16);
	data += StrSize((CHAR16 *)data);

	return CompareMem(cursor, data, candidate_size - min_valid_size);
}

EFI_STATUS
find_boot_option(EFI_DEVICE_PATH *dp, EFI_DEVICE_PATH *fulldp,
                 CHAR16 *filename, CHAR16 *label, CHAR16 *arguments,
                 UINT16 *optnum)
{
	unsigned int size = sizeof(UINT32) + sizeof (UINT16) +
		StrLen(label)*2 + 2 + DevicePathSize(dp) +
		StrLen(arguments) * 2;

	CHAR8 *data = AllocateZeroPool(size + 2);
	if (!data)
		return EFI_OUT_OF_RESOURCES;
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

	int i = 0;
	CHAR16 varname[] = L"Boot0000";
	CHAR16 hexmap[] = L"0123456789ABCDEF";
	EFI_GUID global = EFI_GLOBAL_VARIABLE;
	EFI_STATUS rc;

	UINTN max_candidate_size = calc_masked_boot_option_size(size);
	CHAR8 *candidate = AllocateZeroPool(max_candidate_size);
	if (!candidate) {
		FreePool(data);
		return EFI_OUT_OF_RESOURCES;
	}

	for(i = 0; i < nbootorder && i < 0x10000; i++) {
		varname[4] = hexmap[(bootorder[i] & 0xf000) >> 12];
		varname[5] = hexmap[(bootorder[i] & 0x0f00) >> 8];
		varname[6] = hexmap[(bootorder[i] & 0x00f0) >> 4];
		varname[7] = hexmap[(bootorder[i] & 0x000f) >> 0];

		UINTN candidate_size = max_candidate_size;
		rc = uefi_call_wrapper(RT->GetVariable, 5, varname, &global,
					NULL, &candidate_size, candidate);
		if (EFI_ERROR(rc))
			continue;

		if (candidate_size != size) {
			if (check_masked_boot_option(candidate, candidate_size,
						     data, size))
				continue;
		} else if (CompareMem(candidate, data, size))
			continue;

		VerbosePrint(L"Found boot entry \"%s\" with label \"%s\" "
			     L"for file \"%s\"\n", varname, label, filename);

		/* at this point, we have duplicate data. */
		if (!first_new_option) {
			first_new_option = DuplicateDevicePath(fulldp);
			first_new_option_args = arguments;
			first_new_option_size = StrLen(arguments) * sizeof (CHAR16);
		}

		*optnum = i;
		FreePool(candidate);
		FreePool(data);
		return EFI_SUCCESS;
	}
	FreePool(candidate);
	FreePool(data);
	return EFI_NOT_FOUND;
}

EFI_STATUS
set_boot_order(void)
{
	CHAR16 *oldbootorder;
	UINTN size;
	EFI_GUID global = EFI_GLOBAL_VARIABLE;

	oldbootorder = LibGetVariableAndSize(L"BootOrder", &global, &size);
	if (oldbootorder) {
		nbootorder = size / sizeof (CHAR16);
		bootorder = oldbootorder;
	}
	return EFI_SUCCESS;

}

EFI_STATUS
update_boot_order(void)
{
	UINTN size;
	UINTN len = 0;
	EFI_GUID global = EFI_GLOBAL_VARIABLE;
	CHAR16 *newbootorder = NULL;
	EFI_STATUS rc;

	size = nbootorder * sizeof(CHAR16);
	newbootorder = AllocateZeroPool(size);
	if (!newbootorder)
		return EFI_OUT_OF_RESOURCES;
	CopyMem(newbootorder, bootorder, size);

	VerbosePrint(L"nbootorder: %d\nBootOrder: ", size / sizeof (CHAR16));
	UINTN j;
	for (j = 0 ; j < size / sizeof (CHAR16); j++)
		VerbosePrintUnprefixed(L"%04x ", newbootorder[j]);
	Print(L"\n");
	rc = uefi_call_wrapper(RT->GetVariable, 5, L"BootOrder", &global,
			       NULL, &len, NULL);
	if (rc == EFI_BUFFER_TOO_SMALL)
		LibDeleteVariable(L"BootOrder", &global);

	rc = uefi_call_wrapper(RT->SetVariable, 5, L"BootOrder", &global,
					EFI_VARIABLE_NON_VOLATILE |
					 EFI_VARIABLE_BOOTSERVICE_ACCESS |
					 EFI_VARIABLE_RUNTIME_ACCESS,
					size, newbootorder);
	FreePool(newbootorder);
	return rc;
}

EFI_STATUS
add_to_boot_list(CHAR16 *dirname, CHAR16 *filename, CHAR16 *label, CHAR16 *arguments)
{
	CHAR16 *fullpath = NULL;
	UINT64 pathlen = 0;
	EFI_STATUS rc = EFI_SUCCESS;

	rc = make_full_path(dirname, filename, &fullpath, &pathlen);
	if (EFI_ERROR(rc))
		return rc;

	EFI_DEVICE_PATH *full_device_path = NULL;
	EFI_DEVICE_PATH *dp = NULL;
	CHAR16 *dps;

	full_device_path = FileDevicePath(this_image->DeviceHandle, fullpath);
	if (!full_device_path) {
		rc = EFI_OUT_OF_RESOURCES;
		goto err;
	}
	dps = DevicePathToStr(full_device_path);
	VerbosePrint(L"file DP: %s\n", dps);
	FreePool(dps);

	rc = FindSubDevicePath(full_device_path,
				MEDIA_DEVICE_PATH, MEDIA_HARDDRIVE_DP, &dp);
	if (EFI_ERROR(rc)) {
		if (rc == EFI_NOT_FOUND) {
			dp = full_device_path;
		} else {
			rc = EFI_OUT_OF_RESOURCES;
			goto err;
		}
	}

	{
		UINTN s = DevicePathSize(dp);
		UINTN i;
		UINT8 *dpv = (void *)dp;
		for (i = 0; i < s; i++) {
			if (i % 16 == 0) {
				if (i > 0)
					VerbosePrintUnprefixed(L"\n");
				VerbosePrint(L"");
			}
			VerbosePrintUnprefixed(L"%02x ", dpv[i]);
		}
		VerbosePrintUnprefixed(L"\n");

		CHAR16 *dps = DevicePathToStr(dp);
		VerbosePrint(L"device path: \"%s\"\n", dps);
		FreePool(dps);
	}

	UINT16 option;
	rc = find_boot_option(dp, full_device_path, fullpath, label, arguments, &option);
	if (EFI_ERROR(rc)) {
		add_boot_option(dp, full_device_path, fullpath, label, arguments);
	} else if (option != 0) {
		CHAR16 *newbootorder;
		newbootorder = AllocateZeroPool(sizeof (CHAR16) * nbootorder);
		if (!newbootorder)
			return EFI_OUT_OF_RESOURCES;

		newbootorder[0] = bootorder[option];
		CopyMem(newbootorder + 1, bootorder, sizeof (CHAR16) * option);
		CopyMem(newbootorder + option + 1, bootorder + option + 1,
			sizeof (CHAR16) * (nbootorder - option - 1));
		FreePool(bootorder);
		bootorder = newbootorder;
	}

err:
	if (full_device_path)
		FreePool(full_device_path);
	if (dp && dp != full_device_path)
		FreePool(dp);
	if (fullpath)
		FreePool(fullpath);
	return rc;
}

EFI_STATUS
populate_stanza(CHAR16 *dirname, CHAR16 *filename, CHAR16 *csv)
{
	CHAR16 *file = csv;
	VerbosePrint(L"CSV data: \"%s\"\n", csv);

	UINTN comma0 = StrCSpn(csv, L",");
	if (comma0 == 0)
		return EFI_INVALID_PARAMETER;
	file[comma0] = L'\0';
	VerbosePrint(L"filename: \"%s\"\n", file);

	CHAR16 *label = csv + comma0 + 1;
	UINTN comma1 = StrCSpn(label, L",");
	if (comma1 == 0)
		return EFI_INVALID_PARAMETER;
	label[comma1] = L'\0';
	VerbosePrint(L"label: \"%s\"\n", label);

	CHAR16 *arguments = csv + comma0 +1 + comma1 +1;
	UINTN comma2 = StrCSpn(arguments, L",");
	arguments[comma2] = L'\0';
	/* This one is optional, so don't check if comma2 is 0 */
	VerbosePrint(L"arguments: \"%s\"\n", arguments);

	add_to_boot_list(dirname, file, label, arguments);

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

	VerbosePrint(L"Found file \"%s\"\n", fullpath);

	CHAR16 *buffer;
	UINT64 bs;
	rc = read_file(fh, fullpath, &buffer, &bs);
	if (EFI_ERROR(rc)) {
		Print(L"Could not read file \"%s\": %d\n", fullpath, rc);
		FreePool(fullpath);
		return rc;
	}
	FreePool(fullpath);

	VerbosePrint(L"File looks like:\n%s\n", buffer);

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

		populate_stanza(dirname, filename, start);

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
	buffer = NULL;

	CHAR16 *bootcsv=NULL, *bootarchcsv=NULL;

	bs = 0;
	do {
		bs = 0;
		rc = uefi_call_wrapper(fh->Read, 3, fh, &bs, NULL);
		if (EFI_ERROR(rc) && rc != EFI_BUFFER_TOO_SMALL) {
			Print(L"Could not read \\EFI\\%s\\: %d\n", dirname, rc);
			if (buffer)
				FreePool(buffer);
			return rc;
		}
		/* If there's no data to read, don't try to allocate 0 bytes
		 * and read the data... */
		if (bs == 0)
			break;

		buffer = AllocateZeroPool(bs);
		if (!buffer) {
			Print(L"Could not allocate memory\n");
			return EFI_OUT_OF_RESOURCES;
		}

		rc = uefi_call_wrapper(fh->Read, 3, fh, &bs, buffer);
		if (EFI_ERROR(rc)) {
			Print(L"Could not read \\EFI\\%s\\: %d\n", dirname, rc);
			FreePool(buffer);
			return rc;
		}

		if (bs == 0)
			break;

		fi = buffer;

		if (!bootcsv && !StrCaseCmp(fi->FileName, L"boot.csv"))
			bootcsv = StrDuplicate(fi->FileName);

		if (!bootarchcsv &&
		    !StrCaseCmp(fi->FileName, L"boot" EFI_ARCH L".csv"))
			bootarchcsv = StrDuplicate(fi->FileName);

		FreePool(buffer);
		buffer = NULL;
	} while (bs != 0);

	rc = EFI_SUCCESS;
	if (bootarchcsv) {
		EFI_FILE_HANDLE fh2;
		rc = uefi_call_wrapper(fh->Open, 5, fh, &fh2,
				       bootarchcsv, EFI_FILE_READ_ONLY, 0);
		if (EFI_ERROR(rc) || fh2 == NULL) {
			Print(L"Couldn't open \\EFI\\%s\\%s: %d\n",
			      dirname, bootarchcsv, rc);
		} else {
			rc = try_boot_csv(fh2, dirname, bootarchcsv);
			uefi_call_wrapper(fh2->Close, 1, fh2);
		}
	}
	if ((EFI_ERROR(rc) || !bootarchcsv) && bootcsv) {
		EFI_FILE_HANDLE fh2;
		rc = uefi_call_wrapper(fh->Open, 5, fh, &fh2,
				       bootcsv, EFI_FILE_READ_ONLY, 0);
		if (EFI_ERROR(rc) || fh2 == NULL) {
			Print(L"Couldn't open \\EFI\\%s\\%s: %d\n",
			      dirname, bootcsv, rc);
		} else {
			rc = try_boot_csv(fh2, dirname, bootcsv);
			uefi_call_wrapper(fh2->Close, 1, fh2);
		}
	}
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
		VerbosePrint(L"Found directory named \"%s\"\n", fi->FileName);

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
		CHAR16 *dps = DevicePathToStr(first_new_option);
		UINTN s = DevicePathSize(first_new_option);
		unsigned int i;
		UINT8 *dpv = (void *)first_new_option;
		Print(L"LoadImage failed: %d\nDevice path: \"%s\"\n", rc, dps);
		for (i = 0; i < s; i++) {
			if (i > 0 && i % 16 == 0)
				Print(L"\n");
			Print(L"%02x ", dpv[i]);
		}
		Print(L"\n");

		uefi_call_wrapper(BS->Stall, 1, 500000000);
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
		uefi_call_wrapper(BS->Stall, 1, 500000000);
	}
	return rc;
}

extern EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab);

static void
__attribute__((__optimize__("0")))
debug_hook(void)
{
	EFI_GUID guid = SHIM_LOCK_GUID;
	UINT8 *data = NULL;
	UINTN dataSize = 0;
	EFI_STATUS efi_status;
	volatile register int x = 0;
	extern char _etext, _edata;

	efi_status = get_variable(L"SHIM_DEBUG", &data, &dataSize, guid);
	if (EFI_ERROR(efi_status)) {
		return;
	}

	if (x)
		return;

	x = 1;
	Print(L"add-symbol-file "DEBUGDIR
	      L"fb" EFI_ARCH L".efi.debug %p -s .data %p\n", &_etext,
	      &_edata);
}

EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	EFI_STATUS rc;

	InitializeLib(image, systab);

	/*
	 * if SHIM_DEBUG is set, wait for a debugger to attach.
	 */
	debug_hook();

	rc = uefi_call_wrapper(BS->HandleProtocol, 3, image, &LoadedImageProtocol, (void *)&this_image);
	if (EFI_ERROR(rc)) {
		Print(L"Error: could not find loaded image: %d\n", rc);
		return rc;
	}

	Print(L"System BootOrder not found.  Initializing defaults.\n");

	set_boot_order();

	rc = find_boot_options(this_image->DeviceHandle);
	if (EFI_ERROR(rc)) {
		Print(L"Error: could not find boot options: %d\n", rc);
		return rc;
	}

	rc = fallback_should_prefer_reset();
	if (EFI_ERROR(rc)) {
		VerbosePrint(L"tpm not present, starting the first image\n");
		try_start_first_option(image);
	} else {
		VerbosePrint(L"tpm present, resetting system\n");
	}

	Print(L"Reset System\n");

	if (get_fallback_verbose()) {
		Print(L"Verbose enabled, sleeping for half a second\n");
		uefi_call_wrapper(BS->Stall, 1, 500000);
	}

	uefi_call_wrapper(RT->ResetSystem, 4, EfiResetCold,
			  EFI_SUCCESS, 0, NULL);

	return EFI_SUCCESS;
}
