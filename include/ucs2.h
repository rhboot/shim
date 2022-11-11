// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * ucs2.h - UCS-2 string functions
 * Copyright Red Hat, Inc
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_UCS2_H
#define SHIM_UCS2_H

#include <stdbool.h>

static inline INTN
__attribute__((unused))
StrCaseCmp(CHAR16 *s0, CHAR16 *s1)
{
	CHAR16 c0, c1;
	while (1) {
		if (*s0 == L'\0' || *s1 == L'\0')
			return *s1 - *s0;
		c0 = (*s0 >= L'a' && *s0 <= L'z') ? *s0 - 32 : *s0;
		c1 = (*s1 >= L'a' && *s1 <= L'z') ? *s1 - 32 : *s1;
		if (c0 != c1)
			return c1 - c0;
		s0++;
		s1++;
	}
	return 0;
}

static inline INTN
__attribute__((unused))
StrnCaseCmp(CHAR16 *s0, CHAR16 *s1, int n)
{
	CHAR16 c0, c1;
	int x = 0;
	while (n > x++) {
		if (*s0 == L'\0' || *s1 == L'\0')
			return *s1 - *s0;
		c0 = (*s0 >= L'a' && *s0 <= L'z') ? *s0 - 32 : *s0;
		c1 = (*s1 >= L'a' && *s1 <= L'z') ? *s1 - 32 : *s1;
		if (c0 != c1)
			return c1 - c0;
		s0++;
		s1++;
	}
	return 0;
}

static inline UINTN
__attribute__((unused))
StrCSpn(const CHAR16 *s, const CHAR16 *reject)
{
	UINTN ret;

	for (ret = 0; s[ret] != L'\0'; ret++) {
		int i;
		for (i = 0; reject[i] != L'\0'; i++) {
			if (reject[i] == s[ret])
				return ret;
		}
	}
	return ret;
}

#endif /* SHIM_UCS2_H */
