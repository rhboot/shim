// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * Copyright Red Hat, Inc.
 * Copyright Peter Jones <pjones@redhat.com>
 */
#include "shim.h"

#define NO_REBOOT L"FB_NO_REBOOT"

EFI_LOADED_IMAGE *this_image = NULL;

int
get_fallback_verbose(void)
{
#ifdef FALLBACK_VERBOSE
	return 1;
#else
	UINT8 *data = NULL;
	UINTN dataSize = 0;
	EFI_STATUS efi_status;
	unsigned int i;
	static int state = -1;

	if (state != -1)
		return state;

	efi_status = get_variable(FALLBACK_VERBOSE_VAR_NAME,
				  &data, &dataSize, SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
		state = 0;
		return state;
	}

	state = 0;
	for (i = 0; i < dataSize; i++) {
		if (data[i]) {
			state = 1;
			break;
		}
	}

	if (data)
		FreePool(data);
	return state;
#endif
}

#define VerbosePrintUnprefixed(fmt, ...)				\
	({								\
		UINTN ret_ = 0;						\
		if (get_fallback_verbose())				\
			ret_ = console_print((fmt), ##__VA_ARGS__);	\
		ret_;							\
	})

#define VerbosePrint(fmt, ...)                                      \
	({                                                          \
		UINTN line_ = __LINE__ - 2;                         \
		UINTN ret_ = 0;                                     \
		if (get_fallback_verbose()) {                       \
			console_print(L"%a:%d: ", __func__, line_); \
			ret_ = console_print((fmt), ##__VA_ARGS__); \
		}                                                   \
		ret_;                                               \
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

EFI_STATUS
make_full_path(CHAR16 *dirname, CHAR16 *filename, CHAR16 **out, UINT64 *outlen)
{
	UINT64 len;

	len = StrLen(L"\\EFI\\") + StrLen(dirname)
	    + StrLen(L"\\") + StrLen(filename)
	    + 2;

	CHAR16 *fullpath = AllocateZeroPool(len*sizeof(CHAR16));
	if (!fullpath) {
		console_print(L"Could not allocate memory\n");
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

UINT16 *bootorder = NULL;
UINTN nbootorder = 0;

EFI_DEVICE_PATH *first_new_option = NULL;
VOID *first_new_option_args = NULL;
UINTN first_new_option_size = 0;

EFI_STATUS
add_boot_option(EFI_DEVICE_PATH *hddp, EFI_DEVICE_PATH *fulldp,
		CHAR16 *filename, CHAR16 *label, CHAR16 *arguments,
		UINT16 **newbootentries, UINTN *nnewbootentries)
{
	static int i = 0;
	CHAR16 varname[] = L"Boot0000";
	CHAR16 hexmap[] = L"0123456789ABCDEF";
	EFI_STATUS efi_status;

	for(; i <= 0xffff; i++) {
		varname[4] = hexmap[(i & 0xf000) >> 12];
		varname[5] = hexmap[(i & 0x0f00) >> 8];
		varname[6] = hexmap[(i & 0x00f0) >> 4];
		varname[7] = hexmap[(i & 0x000f) >> 0];

		void *var = LibGetVariable(varname, &GV_GUID);
		if (!var) {
			int arg_size = StrLen(arguments) ? StrLen(arguments) * sizeof (CHAR16) +
				sizeof (CHAR16) : 0;
			int size = sizeof(UINT32) + sizeof (UINT16) +
				StrLen(label)*2 + 2 + DevicePathSize(hddp) +
				arg_size;

			CHAR8 *data, *cursor;
			cursor = data = AllocateZeroPool(size + 2);
			if (!data)
				return EFI_OUT_OF_RESOURCES;

			*(UINT32 *)cursor = LOAD_OPTION_ACTIVE;
			cursor += sizeof (UINT32);
			*(UINT16 *)cursor = DevicePathSize(hddp);
			cursor += sizeof (UINT16);
			StrCpy((CHAR16 *)cursor, label);
			cursor += StrLen(label)*2 + 2;
			CopyMem(cursor, hddp, DevicePathSize(hddp));
			cursor += DevicePathSize(hddp);
			StrCpy((CHAR16 *)cursor, arguments);

			VerbosePrint(L"Creating boot entry \"%s\" with label \"%s\" "
				     L"for file \"%s\"\n",
				     varname, label, filename);

			if (!first_new_option) {
				first_new_option = DuplicateDevicePath(fulldp);
				first_new_option_args = StrDuplicate(arguments);
				first_new_option_size = arg_size;
			}

			efi_status = RT->SetVariable(varname, &GV_GUID,
						EFI_VARIABLE_NON_VOLATILE |
						EFI_VARIABLE_BOOTSERVICE_ACCESS |
						EFI_VARIABLE_RUNTIME_ACCESS,
						size, data);

			FreePool(data);

			if (EFI_ERROR(efi_status)) {
				console_print(L"Could not create variable: %r\n",
					      efi_status);
				return efi_status;
			}

			UINT16 *newbootorder = AllocateZeroPool(sizeof (UINT16) * (*nnewbootentries + 1));
			if (!newbootorder)
				return EFI_OUT_OF_RESOURCES;

			UINTN j = 0;
			CopyMem(newbootorder, *newbootentries, sizeof (UINT16) * (*nnewbootentries));
			newbootorder[*nnewbootentries] = i & 0xffff;
			if (*newbootentries)
				FreePool(*newbootentries);
			*newbootentries = newbootorder;
			*nnewbootentries += 1;
			VerbosePrint(L"nnewbootentries: %d\nnewbootentries: ",
				      *nnewbootentries);
			for (j = 0 ; j < *nnewbootentries ; j++)
				VerbosePrintUnprefixed(L"%04x ", (*newbootentries)[j]);
			VerbosePrintUnprefixed(L"\n");

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
	unsigned int label_size = StrLen(label)*2 + 2;
	int arg_size = StrLen(arguments) ? StrLen(arguments) * sizeof (CHAR16) +
		sizeof (CHAR16) : 0;
	unsigned int size = sizeof(UINT32) + sizeof (UINT16) +
		label_size + DevicePathSize(dp) +
		arg_size;

	CHAR8 *data = AllocateZeroPool(size + 2);
	if (!data)
		return EFI_OUT_OF_RESOURCES;
	CHAR8 *cursor = data;
	*(UINT32 *)cursor = LOAD_OPTION_ACTIVE;
	cursor += sizeof (UINT32);
	*(UINT16 *)cursor = DevicePathSize(dp);
	cursor += sizeof (UINT16);
	StrCpy((CHAR16 *)cursor, label);
	cursor += label_size;
	CopyMem(cursor, dp, DevicePathSize(dp));
	cursor += DevicePathSize(dp);
	StrCpy((CHAR16 *)cursor, arguments);

	EFI_STATUS efi_status;
	EFI_GUID vendor_guid = NullGuid;
	UINTN buffer_size = 256 * sizeof(CHAR16);
	CHAR16 *varname = AllocateZeroPool(buffer_size);
	if (!varname)
		return EFI_OUT_OF_RESOURCES;

	UINTN max_candidate_size = calc_masked_boot_option_size(size);
	CHAR8 *candidate = AllocateZeroPool(max_candidate_size);
	if (!candidate) {
		FreePool(data);
		return EFI_OUT_OF_RESOURCES;
	}

	while (1) {
		UINTN varname_size = buffer_size;
		efi_status = RT->GetNextVariableName(&varname_size, varname,
						     &vendor_guid);
		if (EFI_ERROR(efi_status)) {
			if (efi_status == EFI_BUFFER_TOO_SMALL) {
				VerbosePrint(L"Buffer too small for next variable name, re-allocating it to be %d bytes and retrying\n",
					     varname_size);
				varname = ReallocatePool(varname,
							 buffer_size,
							 varname_size);
				if (!varname)
					return EFI_OUT_OF_RESOURCES;
				buffer_size = varname_size;
				continue;
			}

			if (efi_status == EFI_DEVICE_ERROR)
				VerbosePrint(L"The next variable name could not be retrieved due to a hardware error\n");

			if (efi_status == EFI_INVALID_PARAMETER)
				VerbosePrint(L"Invalid parameter to GetNextVariableName: varname_size=%d, varname=%s\n",
					     varname_size, varname);

			/* EFI_NOT_FOUND means we listed all variables */
			VerbosePrint(L"Checked all boot entries\n");
			break;
		}

		if (StrLen(varname) != 8 || StrnCmp(varname, L"Boot", 4) ||
		    !isxdigit(varname[4]) || !isxdigit(varname[5]) ||
		    !isxdigit(varname[6]) || !isxdigit(varname[7]))
			continue;

		UINTN candidate_size = max_candidate_size;
		efi_status = RT->GetVariable(varname, &GV_GUID, NULL,
					     &candidate_size, candidate);
		if (EFI_ERROR(efi_status))
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
			first_new_option_args = StrDuplicate(arguments);
			first_new_option_size = arg_size;
		}

		*optnum = xtoi(varname + 4);
		FreePool(candidate);
		FreePool(data);
		return EFI_SUCCESS;
	}
	FreePool(candidate);
	FreePool(data);
	FreePool(varname);
	return efi_status;
}

EFI_STATUS
set_boot_order(void)
{
	UINT16 *oldbootorder;
	UINTN size;

	oldbootorder = LibGetVariableAndSize(L"BootOrder", &GV_GUID, &size);
	if (oldbootorder) {
		UINTN i;
		nbootorder = size / sizeof (UINT16);
		bootorder = oldbootorder;

		VerbosePrint(L"Original nbootorder: %d\nOriginal BootOrder: ",
			     nbootorder);
		for (i = 0 ; i < nbootorder ; i++)
			VerbosePrintUnprefixed(L"%04x ", bootorder[i]);
		VerbosePrintUnprefixed(L"\n");
	}
	return EFI_SUCCESS;

}

EFI_STATUS
update_boot_order(UINT16 *newbootentries, UINTN nnewbootentries)
{
	UINTN size;
	UINTN len = 0;
	UINT16 *newbootorder = NULL;
	EFI_STATUS efi_status;
	UINTN i;

	VerbosePrint(L"old boot order: ");
	for (i = 0; i < nbootorder; i++)
		VerbosePrintUnprefixed(L"%04x ", bootorder[i]);
	VerbosePrintUnprefixed(L"\n");
	VerbosePrint(L"new boot entries: ");
	for (i = 0; i < nnewbootentries; i++)
		VerbosePrintUnprefixed(L"%04x ", newbootentries[i]);
	VerbosePrintUnprefixed(L"\n");

	size = nbootorder * sizeof(UINT16) + nnewbootentries * sizeof(UINT16);
	newbootorder = AllocateZeroPool(size);
	if (!newbootorder)
		return EFI_OUT_OF_RESOURCES;
	for (i = 0 ; i < nnewbootentries; i++) {
		newbootorder[i] = newbootentries[i];
	}
	CopyMem(&newbootorder[i], bootorder, nbootorder * sizeof(UINT16));

	if (bootorder)
		FreePool(bootorder);
	nbootorder = nnewbootentries + nbootorder;
	bootorder = newbootorder;

	VerbosePrint(L"updated nbootorder: %d\n", nbootorder);
	VerbosePrint(L"updated bootoder: ");
	for (i = 0; i < nbootorder; i++)
		VerbosePrintUnprefixed(L"%04x ", bootorder[i]);
	VerbosePrintUnprefixed(L"\n");
	efi_status = RT->GetVariable(L"BootOrder", &GV_GUID, NULL, &len, NULL);
	if (efi_status == EFI_BUFFER_TOO_SMALL)
		LibDeleteVariable(L"BootOrder", &GV_GUID);

	efi_status = RT->SetVariable(L"BootOrder", &GV_GUID,
				     EFI_VARIABLE_NON_VOLATILE |
				     EFI_VARIABLE_BOOTSERVICE_ACCESS |
				     EFI_VARIABLE_RUNTIME_ACCESS,
				     size, bootorder);
	return efi_status;
}

EFI_STATUS
add_to_boot_list(CHAR16 *dirname, CHAR16 *filename, CHAR16 *label, CHAR16 *arguments,
		 UINT16 **newbootentries, UINTN *nnewbootentries)
{
	CHAR16 *fullpath = NULL;
	UINT64 pathlen = 0;
	EFI_STATUS efi_status;

	efi_status = make_full_path(dirname, filename, &fullpath, &pathlen);
	if (EFI_ERROR(efi_status))
		return efi_status;

	EFI_DEVICE_PATH *full_device_path = NULL;
	EFI_DEVICE_PATH *dp = NULL;
	CHAR16 *dps;

	full_device_path = FileDevicePath(this_image->DeviceHandle, fullpath);
	if (!full_device_path) {
		efi_status = EFI_OUT_OF_RESOURCES;
		goto done;
	}
	dps = DevicePathToStr(full_device_path);
	VerbosePrint(L"file DP: %s\n", dps);
	FreePool(dps);

	efi_status = FindSubDevicePath(full_device_path,
				       MEDIA_DEVICE_PATH, MEDIA_HARDDRIVE_DP,
				       &dp);
	if (EFI_ERROR(efi_status)) {
		if (efi_status == EFI_NOT_FOUND) {
			dp = full_device_path;
		} else {
			efi_status = EFI_OUT_OF_RESOURCES;
			goto done;
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
	efi_status = find_boot_option(dp, full_device_path, fullpath, label,
				      arguments, &option);
	if (EFI_ERROR(efi_status)) {
		add_boot_option(dp, full_device_path, fullpath, label,
				arguments, newbootentries, nnewbootentries);
		goto done;
	}

	UINT16 bootnum;
	UINT16 *newbootorder;
	/* Search for the option in the current bootorder */
	for (bootnum = 0; bootnum < nbootorder; bootnum++)
		if (bootorder[bootnum] == option)
			break;
	if (bootnum == nbootorder) {
		/* Option not found, prepend option and copy the rest */
		newbootorder = AllocateZeroPool(sizeof(UINT16)
						* (nbootorder + 1));
		if (!newbootorder) {
			efi_status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
		newbootorder[0] = option;
		CopyMem(newbootorder + 1, bootorder,
			sizeof(UINT16) * nbootorder);
		FreePool(bootorder);
		bootorder = newbootorder;
		nbootorder += 1;
	} else {
		/* Option found, put first and slice the rest */
		newbootorder = AllocateZeroPool(
			sizeof(UINT16) * nbootorder);
		if (!newbootorder) {
			efi_status = EFI_OUT_OF_RESOURCES;
			goto done;
		}
		newbootorder[0] = option;
		CopyMem(newbootorder + 1, bootorder,
			sizeof(UINT16) * bootnum);
		CopyMem(newbootorder + 1 + bootnum,
			bootorder + bootnum + 1,
			sizeof(UINT16) * (nbootorder - bootnum - 1));
		FreePool(bootorder);
		bootorder = newbootorder;
	}
	VerbosePrint(L"New nbootorder: %d\nBootOrder: ",
		      nbootorder);
	for (UINTN i = 0 ; i < nbootorder ; i++)
		VerbosePrintUnprefixed(L"%04x ", bootorder[i]);
	VerbosePrintUnprefixed(L"\n");

done:
	if (full_device_path)
		FreePool(full_device_path);
	if (dp && dp != full_device_path)
		FreePool(dp);
	if (fullpath)
		FreePool(fullpath);
	return efi_status;
}

EFI_STATUS
populate_stanza(CHAR16 *dirname, CHAR16 *filename UNUSED, CHAR16 *csv,
		UINT16 **newbootentries, UINTN *nnewbootentries)
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

	add_to_boot_list(dirname, file, label, arguments, newbootentries, nnewbootentries);

	return EFI_SUCCESS;
}

EFI_STATUS
try_boot_csv(EFI_FILE_HANDLE fh, CHAR16 *dirname, CHAR16 *filename,
	     UINT16 **newbootentries, UINTN *nnewbootentries)
{
	CHAR16 *fullpath = NULL;
	UINT64 pathlen = 0;
	EFI_STATUS efi_status;

	efi_status = make_full_path(dirname, filename, &fullpath, &pathlen);
	if (EFI_ERROR(efi_status))
		return efi_status;

	VerbosePrint(L"Found file \"%s\"\n", fullpath);

	CHAR16 *buffer;
	UINT64 bs;
	efi_status = read_file(fh, fullpath, &buffer, &bs);
	if (EFI_ERROR(efi_status)) {
		console_print(L"Could not read file \"%s\": %r\n",
			      fullpath, efi_status);
		FreePool(fullpath);
		return efi_status;
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

		populate_stanza(dirname, filename, start, newbootentries, nnewbootentries);

		start[l] = c;
		start += l;
	}

	FreePool(buffer);
	return EFI_SUCCESS;
}

EFI_STATUS
find_boot_csv(EFI_FILE_HANDLE fh, CHAR16 *dirname,
	      UINT16 **newbootentries, UINTN *nnewbootentries)
{
	EFI_STATUS efi_status;
	void *buffer = NULL;
	UINTN bs = 0;

	/* The API here is "Call it once with bs=0, it fills in bs,
	 * then allocate a buffer and ask again to get it filled. */
	efi_status = fh->GetInfo(fh, &EFI_FILE_INFO_GUID, &bs, NULL);
	if (EFI_ERROR(efi_status) && efi_status != EFI_BUFFER_TOO_SMALL) {
		console_print(L"Could not get directory info for \\EFI\\%s\\: %r\n",
			      dirname, efi_status);
		return efi_status;
	}
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
		console_print(L"Could not get info for \"%s\": %r\n", dirname,
			      efi_status);
		if (buffer)
			FreePool(buffer);
		return efi_status;
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
		efi_status = fh->Read(fh, &bs, NULL);
		if (EFI_ERROR(efi_status) &&
		    efi_status != EFI_BUFFER_TOO_SMALL) {
			console_print(L"Could not read \\EFI\\%s\\: %r\n",
				      dirname, efi_status);
			return efi_status;
		}
		/* If there's no data to read, don't try to allocate 0 bytes
		 * and read the data... */
		if (bs == 0)
			break;

		buffer = AllocateZeroPool(bs);
		if (!buffer) {
			console_print(L"Could not allocate memory\n");
			return EFI_OUT_OF_RESOURCES;
		}

		efi_status = fh->Read(fh, &bs, buffer);
		if (EFI_ERROR(efi_status)) {
			console_print(L"Could not read \\EFI\\%s\\: %r\n",
				      dirname, efi_status);
			FreePool(buffer);
			return efi_status;
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

	efi_status = EFI_SUCCESS;
	if (bootarchcsv) {
		EFI_FILE_HANDLE fh2;
		efi_status = fh->Open(fh, &fh2, bootarchcsv,
				      EFI_FILE_READ_ONLY, 0);
		if (EFI_ERROR(efi_status) || fh2 == NULL) {
			console_print(L"Couldn't open \\EFI\\%s\\%s: %r\n",
				      dirname, bootarchcsv, efi_status);
		} else {
			efi_status = try_boot_csv(fh2, dirname, bootarchcsv,
						  newbootentries, nnewbootentries);
			fh2->Close(fh2);
			if (EFI_ERROR(efi_status))
				console_print(L"Could not process \\EFI\\%s\\%s: %r\n",
					      dirname, bootarchcsv, efi_status);
		}
	}
	if ((EFI_ERROR(efi_status) || !bootarchcsv) && bootcsv) {
		EFI_FILE_HANDLE fh2;
		efi_status = fh->Open(fh, &fh2, bootcsv,
				      EFI_FILE_READ_ONLY, 0);
		if (EFI_ERROR(efi_status) || fh2 == NULL) {
			console_print(L"Couldn't open \\EFI\\%s\\%s: %r\n",
				      dirname, bootcsv, efi_status);
		} else {
			efi_status = try_boot_csv(fh2, dirname, bootcsv,
						  newbootentries, nnewbootentries);
			fh2->Close(fh2);
			if (EFI_ERROR(efi_status))
				console_print(L"Could not process \\EFI\\%s\\%s: %r\n",
					      dirname, bootarchcsv, efi_status);
		}
	}
	return EFI_SUCCESS;
}

EFI_STATUS
find_boot_options(EFI_HANDLE device)
{
	EFI_STATUS efi_status;
	EFI_FILE_IO_INTERFACE *fio = NULL;
	UINT16 *newbootentries = NULL;
	UINTN nnewbootentries = 0;

	efi_status = BS->HandleProtocol(device, &FileSystemProtocol,
					(void **) &fio);
	if (EFI_ERROR(efi_status)) {
		console_print(L"Couldn't find file system: %r\n", efi_status);
		return efi_status;
	}

	/* EFI_FILE_HANDLE is a pointer to an EFI_FILE, and I have
	 * *no idea* what frees the memory allocated here. Hopefully
	 * Close() does. */
	EFI_FILE_HANDLE fh = NULL;
	efi_status = fio->OpenVolume(fio, &fh);
	if (EFI_ERROR(efi_status) || fh == NULL) {
		console_print(L"Couldn't open file system: %r\n", efi_status);
		return efi_status;
	}

	EFI_FILE_HANDLE fh2 = NULL;
	efi_status = fh->Open(fh, &fh2, L"EFI", EFI_FILE_READ_ONLY, 0);
	if (EFI_ERROR(efi_status) || fh2 == NULL) {
		console_print(L"Couldn't open EFI: %r\n", efi_status);
		fh->Close(fh);
		return efi_status;
	}
	efi_status = fh2->SetPosition(fh2, 0);
	if (EFI_ERROR(efi_status)) {
		console_print(L"Couldn't set file position: %r\n", efi_status);
		fh2->Close(fh2);
		fh->Close(fh);
		return efi_status;
	}

	void *buffer;
	UINTN bs;
	do {
		bs = 0;
		efi_status = fh2->Read(fh2, &bs, NULL);
		if (EFI_ERROR(efi_status) && efi_status != EFI_BUFFER_TOO_SMALL) {
			console_print(L"Could not read \\EFI\\: %r\n", efi_status);
			return efi_status;
		}
		if (bs == 0)
			break;

		buffer = AllocateZeroPool(bs);
		if (!buffer) {
			console_print(L"Could not allocate memory\n");
			/* sure, this might work, why not? */
			fh2->Close(fh2);
			fh->Close(fh);
			return EFI_OUT_OF_RESOURCES;
		}

		efi_status = fh2->Read(fh2, &bs, buffer);
		if (EFI_ERROR(efi_status)) {
			if (buffer) {
				FreePool(buffer);
				buffer = NULL;
			}
			fh2->Close(fh2);
			fh->Close(fh);
			return efi_status;
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
		efi_status = fh2->Open(fh2, &fh3, fi->FileName,
				      EFI_FILE_READ_ONLY, 0);
		if (EFI_ERROR(efi_status)) {
			console_print(L"%d Couldn't open %s: %r\n", __LINE__,
				      fi->FileName, efi_status);
			FreePool(buffer);
			buffer = NULL;
			continue;
		}

		efi_status = find_boot_csv(fh3, fi->FileName,
					   &newbootentries, &nnewbootentries);
		fh3->Close(fh3);
		FreePool(buffer);
		buffer = NULL;
		if (efi_status == EFI_OUT_OF_RESOURCES)
			break;

	} while (1);

	if (!EFI_ERROR(efi_status) && (nbootorder > 0 || nnewbootentries > 0))
		efi_status = update_boot_order(newbootentries, nnewbootentries);

	fh2->Close(fh2);
	fh->Close(fh);
	return efi_status;
}

static EFI_STATUS
try_start_first_option(EFI_HANDLE parent_image_handle)
{
	EFI_STATUS efi_status;
	EFI_HANDLE image_handle;

	if (get_fallback_verbose()) {
		unsigned long fallback_verbose_wait = 500000; /* default to 0.5s */
#ifdef FALLBACK_VERBOSE_WAIT
		fallback_verbose_wait = FALLBACK_VERBOSE_WAIT;
#endif
		console_print(L"Verbose enabled, sleeping for %d mseconds... "
			      L"Press the Pause key now to hold for longer.\n",
			      fallback_verbose_wait);
		usleep(fallback_verbose_wait);
	}

	if (!first_new_option) {
		return EFI_SUCCESS;
	}

	efi_status = BS->LoadImage(0, parent_image_handle, first_new_option,
				   NULL, 0, &image_handle);
	if (EFI_ERROR(efi_status)) {
		CHAR16 *dps = DevicePathToStr(first_new_option);
		UINTN s = DevicePathSize(first_new_option);
		unsigned int i;
		UINT8 *dpv = (void *)first_new_option;
		console_print(L"LoadImage failed: %r\nDevice path: \"%s\"\n",
			      efi_status, dps);
		for (i = 0; i < s; i++) {
			if (i > 0 && i % 16 == 0)
				console_print(L"\n");
			console_print(L"%02x ", dpv[i]);
		}
		console_print(L"\n");

		usleep(500000000);
		return efi_status;
	}

	EFI_LOADED_IMAGE *image;
	efi_status = BS->HandleProtocol(image_handle, &LoadedImageProtocol,
					(void *) &image);
	if (!EFI_ERROR(efi_status)) {
		image->LoadOptions = first_new_option_args;
		image->LoadOptionsSize = first_new_option_size;
	}

	efi_status = BS->StartImage(image_handle, NULL, NULL);
	if (EFI_ERROR(efi_status)) {
		console_print(L"StartImage failed: %r\n", efi_status);
		usleep(500000000);
	}
	return efi_status;
}

static UINT32
get_fallback_no_reboot(void)
{
	EFI_STATUS efi_status;
	UINT32 no_reboot;
	UINTN size = sizeof(UINT32);

	efi_status = RT->GetVariable(NO_REBOOT, &SHIM_LOCK_GUID,
				     NULL, &size, &no_reboot);
	if (!EFI_ERROR(efi_status)) {
		return no_reboot;
	}
	return 0;
}

#ifndef FALLBACK_NONINTERACTIVE
static EFI_STATUS
set_fallback_no_reboot(void)
{
	EFI_STATUS efi_status;
	UINT32 no_reboot = 1;
	efi_status = RT->SetVariable(NO_REBOOT, &SHIM_LOCK_GUID,
				     EFI_VARIABLE_NON_VOLATILE |
				     EFI_VARIABLE_BOOTSERVICE_ACCESS |
				     EFI_VARIABLE_RUNTIME_ACCESS,
				     sizeof(UINT32), &no_reboot);
	return efi_status;
}

static int
draw_countdown(void)
{
	CHAR16 *title = L"Boot Option Restoration";
	CHAR16 *message = L"Press any key to stop system reset";
	int timeout;

	timeout = console_countdown(title, message, 5);

	return timeout;
}

static int
get_user_choice(void)
{
	int choice;
	CHAR16 *title[] = {L"Boot Option Restored", NULL};
	CHAR16 *menu_strings[] = {
		L"Reset system",
		L"Continue boot",
		L"Always continue boot",
		NULL
	};

	do {
		choice = console_select(title, menu_strings, 0);
	} while (choice < 0 || choice > 2);

	return choice;
}
#endif

extern EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab);

static void
__attribute__((__optimize__("0")))
debug_hook(void)
{
	UINT8 *data = NULL;
	UINTN dataSize = 0;
	EFI_STATUS efi_status;
	register volatile int x = 0;
	extern char _etext, _edata;

	efi_status = get_variable(DEBUG_VAR_NAME, &data, &dataSize,
				  SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
		return;
	}

	if (data)
		FreePool(data);
	if (x)
		return;

	x = 1;
	console_print(L"add-symbol-file "DEBUGDIR
		      L"fb" EFI_ARCH L".efi.debug %p -s .data %p\n",
		      &_etext, &_edata);
}

EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	EFI_STATUS efi_status;

	InitializeLib(image, systab);

	/*
	 * if SHIM_DEBUG is set, wait for a debugger to attach.
	 */
	debug_hook();

	efi_status = BS->HandleProtocol(image, &LoadedImageProtocol,
					(void *) &this_image);
	if (EFI_ERROR(efi_status)) {
		console_print(L"Error: could not find loaded image: %r\n",
			      efi_status);
		return efi_status;
	}

	VerbosePrint(L"System BootOrder not found.  Initializing defaults.\n");

	set_boot_order();

	efi_status = find_boot_options(this_image->DeviceHandle);
	if (EFI_ERROR(efi_status)) {
		console_print(L"Error: could not find boot options: %r\n",
			      efi_status);
		return efi_status;
	}

	efi_status = fallback_should_prefer_reset();
	if (EFI_ERROR(efi_status)) {
		VerbosePrint(L"tpm not present, starting the first image\n");
		try_start_first_option(image);
	} else {
		if (get_fallback_no_reboot() == 1) {
			VerbosePrint(L"NO_REBOOT is set, starting the first image\n");
			try_start_first_option(image);
		}

#ifndef FALLBACK_NONINTERACTIVE
		int timeout = draw_countdown();
		if (timeout == 0)
			goto reset;

		int choice = get_user_choice();
		if (choice == 0) {
			goto reset;
		} else if (choice == 2) {
			efi_status = set_fallback_no_reboot();
			if (EFI_ERROR(efi_status))
				goto reset;
		}
		VerbosePrint(L"tpm present, starting the first image\n");
		try_start_first_option(image);
reset:
#endif
		VerbosePrint(L"tpm present, resetting system\n");
	}

	console_print(L"Reset System\n");

	if (get_fallback_verbose()) {
		unsigned long fallback_verbose_wait = 500000; /* default to 0.5s */
#ifdef FALLBACK_VERBOSE_WAIT
		fallback_verbose_wait = FALLBACK_VERBOSE_WAIT;
#endif
		console_print(L"Verbose enabled, sleeping for %d mseconds... "
			      L"Press the Pause key now to hold for longer.\n",
			      fallback_verbose_wait);
		usleep(fallback_verbose_wait);
	}

	RT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

	return EFI_SUCCESS;
}
