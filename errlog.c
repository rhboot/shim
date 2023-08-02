// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * errlog.c
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

static CHAR16 **errs = NULL;
static UINTN nerrs = 0;

EFI_STATUS EFIAPI
vdprint_(const CHAR16 *fmt, const char *file, int line, const char *func,
         ms_va_list args)
{
	ms_va_list args2;
	EFI_STATUS efi_status = EFI_SUCCESS;

	if (verbose) {
		ms_va_copy(args2, args);
		console_print(L"%a:%d:%a() ", file, line, func);
		efi_status = VPrint(fmt, args2);
		ms_va_end(args2);
	}
	return efi_status;
}

EFI_STATUS EFIAPI
VLogError(const char *file, int line, const char *func, const CHAR16 *fmt,
          ms_va_list args)
{
	ms_va_list args2;
	CHAR16 **newerrs;

	if (file == NULL || func == NULL || fmt == NULL)
		return EFI_INVALID_PARAMETER;

	newerrs = ReallocatePool(errs, (nerrs + 1) * sizeof(*errs),
				       (nerrs + 3) * sizeof(*errs));
	if (!newerrs)
		return EFI_OUT_OF_RESOURCES;

	newerrs[nerrs] = PoolPrint(L"%a:%d %a() ", file, line, func);
	if (!newerrs[nerrs])
		return EFI_OUT_OF_RESOURCES;
	ms_va_copy(args2, args);
	newerrs[nerrs+1] = VPoolPrint(fmt, args2);
	if (!newerrs[nerrs+1])
		return EFI_OUT_OF_RESOURCES;
	ms_va_end(args2);

	nerrs += 2;
	newerrs[nerrs] = NULL;
	errs = newerrs;

	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
LogError_(const char *file, int line, const char *func, const CHAR16 *fmt, ...)
{
	ms_va_list args;
	EFI_STATUS efi_status;

	ms_va_start(args, fmt);
	efi_status = VLogError(file, line, func, fmt, args);
	ms_va_end(args);

	return efi_status;
}

VOID
LogHexdump_(const char *file, int line, const char *func, const void *data, size_t sz)
{
	hexdumpat(file, line, func, data, sz, 0);
}

VOID
PrintErrors(VOID)
{
	UINTN i;

	if (!verbose)
		return;

	for (i = 0; i < nerrs; i++)
		console_print(L"%s", errs[i]);
}

VOID
ClearErrors(VOID)
{
	UINTN i;

	for (i = 0; i < nerrs; i++)
		FreePool(errs[i]);
	FreePool(errs);
	nerrs = 0;
	errs = NULL;
}

// vim:fenc=utf-8:tw=75
