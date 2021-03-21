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

INTN
StrCmp(CONST CHAR16 *s1, CONST CHAR16 *s2) {
	assert(s1 != NULL);
	assert(s2 != NULL);

	int i;
	for (i = 0; s1[i] && s2[i]; i++) {
		if (s1[i] != s2[i])
			return s2[i] - s1[i];
	}
	return 0;
}

INTN
StrnCmp(CONST CHAR16 *s1, CONST CHAR16 *s2, UINTN len) {
	assert(s1 != NULL);
	assert(s2 != NULL);

	UINTN i;
	for (i = 0; i < len && s1[i] && s2[i]; i++) {
		if (s1[i] != s2[i])
			return s2[i] - s1[i];

	}
	return 0;
}

EFI_STATUS
get_variable_attr(const CHAR16 * const var, UINT8 **data, UINTN *len,
		  EFI_GUID owner, UINT32 *attributes)
{
	return EFI_UNSUPPORTED;
}

EFI_STATUS
get_variable(const CHAR16 * const var, UINT8 **data, UINTN *len, EFI_GUID owner)
{
	return get_variable_attr(var, data, len, owner, NULL);
}

EFI_GUID SHIM_LOCK_GUID = {0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23 } };

// vim:fenc=utf-8:tw=75:noet
