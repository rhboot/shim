/*
 * errlog.c
 * Copyright 2017 Peter Jones <pjones@redhat.com>
 */
#include "shim.h"
#include "hexdump.h"

static CHAR16 **errs = NULL;
static UINTN nerrs = 0;

EFI_STATUS
VLogError(const char *file, int line, const char *func, CONST CHAR16 *fmt, VA_LIST args)
{
	VA_LIST args2;
	CHAR16 **newerrs;

	newerrs = ReallocatePool(errs, (nerrs + 1) * sizeof(*errs),
				       (nerrs + 3) * sizeof(*errs));
	if (!newerrs)
		return EFI_OUT_OF_RESOURCES;

	errs = newerrs;

	newerrs[nerrs] = PoolPrint(L"%a:%d %a() ", file, line, func);
	if (!newerrs[nerrs])
		return EFI_OUT_OF_RESOURCES;
	VA_COPY(args2, args);
	newerrs[nerrs+1] = VPoolPrint(fmt, args2);
	if (!newerrs[nerrs+1]) {
		FreePool(newerrs[nerrs]);
		return EFI_OUT_OF_RESOURCES;
	}
	VA_END(args2);

	nerrs += 2;
	newerrs[nerrs] = NULL;

	return EFI_SUCCESS;
}

EFI_STATUS
LogError_(const char *file, int line, const char *func, const CHAR16 *fmt, ...)
{
	VA_LIST args;
	EFI_STATUS efi_status;

	VA_START(args, fmt);
	efi_status = VLogError(file, line, func, fmt, args);
	VA_END(args);

	return efi_status;
}

VOID
LogHexdump_(const char *file, int line, const char *func, const void *data, UINTN sz)
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
