// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * dp.c - device path helpers
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

int
is_removable_media_path(EFI_LOADED_IMAGE *li)
{
	unsigned int pathlen = 0;
	CHAR16 *bootpath = NULL;
	int ret = 0;

	bootpath = DevicePathToStr(li->FilePath);

	/* Check the beginning of the string and the end, to avoid
	 * caring about which arch this is. */
	/* I really don't know why, but sometimes bootpath gives us
	 * L"\\EFI\\BOOT\\/BOOTX64.EFI".  So just handle that here...
	 */
	if (StrnCaseCmp(bootpath, L"\\EFI\\BOOT\\BOOT", 14) &&
			StrnCaseCmp(bootpath, L"\\EFI\\BOOT\\/BOOT", 15) &&
			StrnCaseCmp(bootpath, L"EFI\\BOOT\\BOOT", 13) &&
			StrnCaseCmp(bootpath, L"EFI\\BOOT\\/BOOT", 14))
		goto error;

	pathlen = StrLen(bootpath);
	if (pathlen < 5 || StrCaseCmp(bootpath + pathlen - 4, L".EFI"))
		goto error;

	ret = 1;

error:
	if (bootpath)
		FreePool(bootpath);

	return ret;
}


// vim:fenc=utf-8:tw=75:noet
