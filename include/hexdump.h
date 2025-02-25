// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef STATIC_HEXDUMP_H
#define STATIC_HEXDUMP_H

#include "shim.h"
#include "include/console.h"

static inline unsigned long UNUSED
prepare_hex(const void *data, size_t size, char *buf, unsigned int position)
{
	char hexchars[] = "0123456789abcdef";
	int offset = 0;
	unsigned long i;
	unsigned long j;
	unsigned long ret;

	unsigned long before = (position % 16);
	unsigned long after = (before+size >= 16) ? 0 : 16 - (before+size);

	for (i = 0; i < before; i++) {
		buf[offset++] = 'X';
		buf[offset++] = 'X';
		buf[offset++] = ' ';
		if (i == 7)
			buf[offset++] = ' ';
	}
	for (j = 0; j < 16 - after - before; j++) {
		uint8_t d = ((uint8_t *)data)[j];
		buf[offset++] = hexchars[(d & 0xf0) >> 4];
		buf[offset++] = hexchars[(d & 0x0f)];
		if (i+j != 15)
			buf[offset++] = ' ';
		if (i+j == 7)
			buf[offset++] = ' ';
	}
	ret = 16 - after - before;
	j += i;
	for (i = 0; i < after; i++) {
		buf[offset++] = 'X';
		buf[offset++] = 'X';
		if (i+j != 15)
			buf[offset++] = ' ';
		if (i+j == 7)
			buf[offset++] = ' ';
	}
	buf[offset] = '\0';
	return ret;
}

static inline void UNUSED
prepare_text(const void *data, size_t size, char *buf, unsigned int position)
{
	int offset = 0;
	unsigned long i;
	unsigned long j;

	unsigned long before = position % 16;
	unsigned long after = (before+size > 16) ? 0 : 16 - (before+size);

	if (size == 0) {
		buf[0] = '\0';
		return;
	}
	for (i = 0; i < before; i++)
		buf[offset++] = 'X';
	buf[offset++] = '|';
	for (j = 0; j < 16 - after - before; j++) {
		if (isprint(((uint8_t *)data)[j]))
			buf[offset++] = ((uint8_t *)data)[j];
		else
			buf[offset++] = '.';
	}
	buf[offset++] = '|';
	buf[offset] = '\0';
}

/*
 * variadic hexdump formatted
 * think of it as: printf("%s%s\n", vformat(fmt, ap), hexdump(data,size));
 */
static inline void UNUSED EFIAPI
vhexdumpf(const char *file, int line, const char *func, const CHAR16 *const fmt,
          const void *data, unsigned long size, size_t at, ms_va_list ap)
{
	unsigned long display_offset = at;
	unsigned long offset = 0;

	if (verbose == 0)
		return;

	if (!data) {
		dprint(L"hexdump of a NULL pointer!\n");
		return;
	}
	if (!size) {
		dprint(L"hexdump of a 0 size region!\n");
		return;
	}

	while (offset < size) {
		char hexbuf[49];
		char txtbuf[19];
		unsigned long sz;

		sz = prepare_hex(data+offset, size-offset, hexbuf,
				 (unsigned long)data+offset);
		if (sz == 0)
			return;

		prepare_text(data+offset, size-offset, txtbuf,
			     (unsigned long)data+offset);
		if (fmt && fmt[0] != 0)
			vdprint_(fmt, file, line, func, ap);
		dprint_(L"%a:%d:%a() %08lx  %a  %a\n", file, line, func, display_offset, hexbuf, txtbuf);

		display_offset += sz;
		offset += sz;
	}
}

/*
 * hexdump formatted
 * think of it as: printf("%s%s", format(fmt, ...), hexdump(data,size)[lineN]);
 */
static inline void UNUSED EFIAPI
hexdumpf(const char *file, int line, const char *func, const CHAR16 *const fmt,
         const void *data, unsigned long size, size_t at, ...)
{
	ms_va_list ap;

	ms_va_start(ap, at);
	vhexdumpf(file, line, func, fmt, data, size, at, ap);
	ms_va_end(ap);
}

static inline void UNUSED
hexdump(const char *file, int line, const char *func, const void *data, unsigned long size)
{
	hexdumpf(file, line, func, L"", data, size, (intptr_t)data);
}

static inline void UNUSED
hexdumpat(const char *file, int line, const char *func, const void *data, unsigned long size, size_t at)
{
	hexdumpf(file, line, func, L"", data, size, at);
}

#if defined(SHIM_UNIT_TEST)
#define LogHexDump(data, ...)
#define dhexdump(data, ...)
#define dhexdumpat(data, ...)
#define dhexdumpf(fmt, ...)
#else
#define LogHexdump(data, sz) LogHexdump_(__FILE__, __LINE__, __func__, data, sz)
#define dhexdump(data, sz)   hexdump(__FILE__, __LINE__, __func__, data, sz)
#define dhexdumpat(data, sz, at) \
	hexdumpat(__FILE__, __LINE__ - 1, __func__, data, sz, at)
#define dhexdumpf(fmt, data, sz, at, ...) \
	hexdumpf(__FILE__, __LINE__ - 1, __func__, fmt, data, sz, at, ##__VA_ARGS__)
#endif

#endif /* STATIC_HEXDUMP_H */
// vim:fenc=utf-8:tw=75:noet
