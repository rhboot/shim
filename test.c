// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test.c - stuff we need for test harnesses
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

UINT8 in_protocol = 0;
int debug = DEFAULT_DEBUG_PRINT_STATE;

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

EFI_STATUS EFIAPI
LogError_(const char *file, int line, const char *func, const CHAR16 *fmt, ...)
{
	assert(0);
	return EFI_SUCCESS;
}

#ifndef HAVE_GET_VARIABLE_ATTR
EFI_STATUS
get_variable_attr(const CHAR16 * const var, UINT8 **data, UINTN *len,
		  EFI_GUID owner, UINT32 *attributes)
{
	return EFI_UNSUPPORTED;
}
#endif

#ifndef HAVE_GET_VARIABLE
EFI_STATUS
get_variable(const CHAR16 * const var, UINT8 **data, UINTN *len, EFI_GUID owner)
{
	return get_variable_attr(var, data, len, owner, NULL);
}
#endif

#ifndef HAVE_SHIM_LOCK_GUID
EFI_GUID SHIM_LOCK_GUID = {0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23 } };
#endif

UINTN EFIAPI
console_print(const CHAR16 *fmt, ...)
{
	return 0;
}

// vim:fenc=utf-8:tw=75:noet
