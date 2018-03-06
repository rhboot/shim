/*
 * shim - trivial UEFI first-stage bootloader
 *
 * Copyright 2013 Red Hat, Inc <pjones@redhat.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Significant portions of this code are derived from Tianocore
 * (http://tianocore.sf.net) and are Copyright 2009-2012 Intel
 * Corporation.
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

/*
 * Test if an entire buffer is nothing but NUL characters.  This
 * implementation "gracefully" ignores the difference between the
 * UTF-8/ASCII 1-byte NUL and the UCS-2 2-byte NUL.
 */
static inline bool
__attribute__((__unused__))
is_all_nuls(UINT8 *data, UINTN data_size)
{
	UINTN i;

	for (i = 0; i < data_size; i++) {
		if (data[i] != 0)
			return false;
	}
	return true;
}

static inline UINTN
__attribute__((__unused__))
count_ucs2_strings(UINT8 *data, UINTN data_size)
{
	UINTN pos = 0;
	UINTN last_nul_pos = 0;
	UINTN num_nuls = 0;
	UINTN i;

	if (data_size % 2 != 0)
		return 0;

	for (i = pos; i < data_size; i++) {
		if (i % 2 != 0) {
			if (data[i] != 0)
				return 0;
		} else if (data[i] == 0) {
			last_nul_pos = i;
			num_nuls++;
		}
		pos = i;
	}
	if (num_nuls > 0 && last_nul_pos != pos - 1)
		return 0;
	return num_nuls;
}

#endif /* SHIM_UCS2_H */
