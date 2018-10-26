/*
 * Copyright 2018 Ruslan Nikolaev <nruslandev@gmail.com>
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
 */

#include <efilib.h>

#define SMALL_SIZE	47

#if defined(__i386__) || defined(__x86_64__)
# define STRICT_ALIGN_REQUIRED	0
#else
# define STRICT_ALIGN_REQUIRED	1
#endif

void *memset(void *dst, int ch, UINTN len)
{
	UINT8 *cur = (UINT8 *) dst;
	UINT8 *end = cur + len;
	UINTN val = (UINT8) ch * (UINTN) 0x0101010101010101ULL; /* truncate */

	if (len > SMALL_SIZE) {
		UINT8 *ptr = (UINT8 *) (((UINTN) cur + (sizeof(UINTN) - 1)) &
							  ~(sizeof(UINTN) - 1));
		for (; cur != ptr; cur++)
			*cur = (UINT8) val;
		ptr = (UINT8 *) ((UINTN) end & ~(sizeof(UINTN) - 1));
		do {
			*((UINTN *) cur) = val;
			cur += sizeof(UINTN);
		} while (cur != ptr);
	}
	while (cur != end)
		*cur++ = (UINT8) val;
	return dst;
}

void *memcpy(void *_dst, const void *_src, UINTN len)
{
	CONST UINT8 *src = (CONST UINT8 *) _src;
	UINT8 *dst = (UINT8 *) _dst;
	UINT8 *end = dst + len;

#if STRICT_ALIGN_REQUIRED == 1
	if (len > SMALL_SIZE &&
		!(((UINTN) src ^ (UINTN) dst) & (sizeof(UINTN) - 1)))
#else
	if (len > SMALL_SIZE)
#endif
	{
		UINT8 *ptr = (UINT8 *) (((UINTN) dst + sizeof(UINTN) - 1) &
							  ~(sizeof(UINTN) - 1));
		for (; dst != ptr; dst++)
			*dst = *src++;
		ptr = (UINT8 *) ((UINTN) end & ~(sizeof(UINTN) - 1));
		do {
			*((UINTN *) dst) = *((UINTN *) src);
			src += sizeof(UINTN);
			dst += sizeof(UINTN);
		} while (dst != ptr);
	}
	while (dst != end)
		*dst++ = *src++;
	return _dst;
}

void *memmove(void *_dst, const void *_src, UINTN len)
{
	if (_dst > _src) {
		CONST UINT8 *src = (CONST UINT8 *) _src + len;
		UINT8 *dst = (UINT8 *) _dst;
		UINT8 *end = dst + len;

#if STRICT_ALIGN_REQUIRED == 1
		if (len > SMALL_SIZE &&
			!(((UINTN) src ^ (UINTN) end) & (sizeof(UINTN) - 1)))
#else
		if (len > SMALL_SIZE)
#endif
		{
			UINT8 *ptr;

			ptr = (UINT8 *) ((UINTN) end & ~(sizeof(UINTN) - 1));
			while (end != ptr)
				*--end = *--src;
			ptr = (UINT8 *) (((UINTN) dst + sizeof(UINTN) - 1) &
							~(sizeof(UINTN) - 1));
			do {
				end -= sizeof(UINTN);
				src -= sizeof(UINTN);
				*((UINTN *) end) = *((UINTN *) src);
			} while (end != ptr);
		}
		while (end != dst)
			*--end = *--src;
		return dst;
	}
	return memcpy(_dst, _src, len);
}

int memcmp(const void *src1, const void *src2, UINTN len)
{
	CONST UINT8 *s1 = (CONST UINT8 *) src1;
	CONST UINT8 *s2 = (CONST UINT8 *) src2;
	CONST UINT8 *s1e = s1 + len;

#if STRICT_ALIGN_REQUIRED == 1
	if (len > SMALL_SIZE &&
		!(((UINTN) s1 ^ (UINTN) s2) & (sizeof(UINTN) - 1)))
#else
	if (len > SMALL_SIZE)
#endif
	{
		CONST UINT8 *ptr = (CONST UINT8 *) (((UINTN) s1 +
				sizeof(UINTN) - 1) & ~(sizeof(UINTN) - 1));
		for (; s1 != ptr; s1++, s2++) {
			if (*s1 != *s2)
				return (int) *s1 - (int) *s2;
		}
		ptr = (CONST UINT8 *) ((UINTN) s1e & ~(sizeof(UINTN) - 1));
		do {
			if (*(UINTN *) s1 != *(UINTN *) s2)
				break;
			s1 += sizeof(UINTN);
			s2 += sizeof(UINTN);
		} while (s1 != ptr);
	}
	for (; s1 != s1e; s1++, s2++) {
		if (*s1 != *s2)
			return (int) *s1 - (int) *s2;
	}
	return 0;
}
