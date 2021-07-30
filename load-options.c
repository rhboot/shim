// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * load-options.c - all the stuff we need to parse the load options
 */

#include "shim.h"

CHAR16 *second_stage;
void *load_options;
UINT32 load_options_size;

/*
 * Generate the path of an executable given shim's path and the name
 * of the executable
 */
EFI_STATUS
generate_path_from_image_path(EFI_LOADED_IMAGE *li,
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
	 * that we don't just use \\, because we (not particularly brightly)
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

	for (i = 0, last = 0; i < StrLen(ImagePath); i++)
		if (ImagePath[i] == '\\')
			last = i + 1;

	ImagePath = ImagePath + last;
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
 * Extract the OptionalData and OptionalData fields from an
 * EFI_LOAD_OPTION.
 */
static inline EFI_STATUS
get_load_option_optional_data(VOID *data, UINT32 data_size,
			      VOID **od, UINT32 *ods)
{
	/*
	 * If it's not at least Attributes + FilePathListLength +
	 * Description=L"" + 0x7fff0400 (EndEntrireDevicePath), it can't
	 * be valid.
	 */
	if (data_size < (sizeof(UINT32) + sizeof(UINT16) + 2 + 4))
		return EFI_INVALID_PARAMETER;

	UINT8 *start = (UINT8 *)data;
	UINT8 *cur = start + sizeof(UINT32);
	UINT16 fplistlen = *(UINT16 *)cur;
	/*
	 * If there's not enough space for the file path list and the
	 * smallest possible description (L""), it's not valid.
	 */
	if (fplistlen > data_size - (sizeof(UINT32) + 2 + 4))
		return EFI_INVALID_PARAMETER;

	cur += sizeof(UINT16);
	UINT32 limit = data_size - (cur - start) - fplistlen;
	UINT32 i;
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

static int
is_our_path(EFI_LOADED_IMAGE *li, CHAR16 *path)
{
	CHAR16 *dppath = NULL;
	CHAR16 *PathName = NULL;
	EFI_STATUS efi_status;
	int ret = 1;

	dppath = DevicePathToStr(li->FilePath);
	if (!dppath)
		return 0;

	efi_status = generate_path_from_image_path(li, path, &PathName);
	if (EFI_ERROR(efi_status)) {
		perror(L"Unable to generate path %s: %r\n", path,
		       efi_status);
		goto done;
	}

	dprint(L"dppath: %s\n", dppath);
	dprint(L"path:   %s\n", path);
	if (StrnCaseCmp(dppath, PathName, StrLen(dppath)))
		ret = 0;

done:
	FreePool(dppath);
	FreePool(PathName);
	return ret;
}

/*
 * Split the supplied load options in to a NULL terminated
 * string representing the path of the second stage loader,
 * and return a pointer to the remaining load options data
 * and its remaining size.
 *
 * This expects the supplied load options to begin with a
 * string that is either NULL terminated or terminated with
 * a space and some optional data. It will return NULL if
 * the supplied load options contains no spaces or NULL
 * terminators.
 */
static CHAR16 *
split_load_options(VOID *in, UINT32 in_size,
		   VOID **remaining,
		   UINT32 *remaining_size) {
	UINTN i;
	CHAR16 *arg0 = NULL;
	CHAR16 *start = (CHAR16 *)in;

	/* Skip spaces */
	for (i = 0; i < in_size / sizeof(CHAR16); i++) {
		if (*start != L' ')
			break;

		start++;
	}

	in_size -= ((VOID *)start - in);

	/*
	 * Ensure that the first argument is NULL terminated by
	 * replacing L' ' with L'\0'.
	 */
	for (i = 0; i < in_size / sizeof(CHAR16); i++) {
		if (start[i] == L' ' || start[i] == L'\0') {
			start[i] = L'\0';
			arg0 = (CHAR16 *)start;
			break;
		}
	}

	if (arg0) {
		UINTN skip = i + 1;
		*remaining_size = in_size - (skip * sizeof(CHAR16));
		*remaining = *remaining_size > 0 ? start + skip : NULL;
	}

	return arg0;
}

/*
 * Check the load options to specify the second stage loader
 */
EFI_STATUS
parse_load_options(EFI_LOADED_IMAGE *li)
{
	EFI_STATUS efi_status;
	VOID *remaining = NULL;
	UINT32 remaining_size;
	CHAR16 *loader_str = NULL;

	dprint(L"full load options:\n");
	dhexdumpat(li->LoadOptions, li->LoadOptionsSize, 0);

	/*
	 * Sanity check since we make several assumptions about the length
	 * Some firmware feeds the following load option when booting from
	 * an USB device:
	 *
	 *    0x46 0x4a 0x00 |FJ.|
	 *
	 * The string is meaningless for shim and so just ignore it.
	 */
	if (li->LoadOptionsSize % 2 != 0)
		return EFI_SUCCESS;

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
	 * Maybe there just aren't any options...
	 */
	if (li->LoadOptionsSize == 0)
		return EFI_SUCCESS;

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
	 * See if this is an EFI_LOAD_OPTION and extract the optional
	 * data if it is. This will return an error if it is not a valid
	 * EFI_LOAD_OPTION.
	 */
	efi_status = get_load_option_optional_data(li->LoadOptions,
						   li->LoadOptionsSize,
						   &li->LoadOptions,
						   &li->LoadOptionsSize);
	if (EFI_ERROR(efi_status)) {
		/*
		 * it's not an EFI_LOAD_OPTION, so it's probably just a string
		 * or list of strings.
		 *
		 * UEFI shell copies the whole line of the command into
		 * LoadOptions. We ignore the first string, i.e. the name of this
		 * program in this case.
		 */
		loader_str = split_load_options(li->LoadOptions,
						li->LoadOptionsSize,
						&remaining,
						&remaining_size);

		if (loader_str && is_our_path(li, loader_str)) {
			li->LoadOptions = remaining;
			li->LoadOptionsSize = remaining_size;
		}
	}

	loader_str = split_load_options(li->LoadOptions, li->LoadOptionsSize,
					&remaining, &remaining_size);

	/*
	 * Set up the name of the alternative loader and the LoadOptions for
	 * the loader
	 */
	if (loader_str) {
		second_stage = loader_str;
		load_options = remaining;
		load_options_size = remaining_size;
	}

	return EFI_SUCCESS;
}

// vim:fenc=utf-8:tw=75:noet
