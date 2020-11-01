#ifndef STATIC_HEXDUMP_H
#define STATIC_HEXDUMP_H

static inline UINTN UNUSED
prepare_hex(const void *data, UINTN size, char *buf, unsigned int position)
{
	char hexchars[] = "0123456789abcdef";
	int offset = 0;
	UINTN i;
	UINTN j;
	UINTN ret;

	UINTN before = (position % 16);
	UINTN after = (before+size >= 16) ? 0 : 16 - (before+size);

	for (i = 0; i < before; i++) {
		buf[offset++] = 'X';
		buf[offset++] = 'X';
		buf[offset++] = ' ';
		if (i == 7)
			buf[offset++] = ' ';
	}
	for (j = 0; j < 16 - after - before; j++) {
		UINT8 d = ((UINT8 *)data)[j];
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

#define isprint(c) ((c) >= 0x20 && (c) <= 0x7e)

static inline void UNUSED
prepare_text(const void *data, UINTN size, char *buf, unsigned int position)
{
	int offset = 0;
	UINTN i;
	UINTN j;

	UINTN before = position % 16;
	UINTN after = (before+size > 16) ? 0 : 16 - (before+size);

	if (size == 0) {
		buf[0] = '\0';
		return;
	}
	for (i = 0; i < before; i++)
		buf[offset++] = 'X';
	buf[offset++] = '|';
	for (j = 0; j < 16 - after - before; j++) {
		if (isprint(((UINT8 *)data)[j]))
			buf[offset++] = ((UINT8 *)data)[j];
		else
			buf[offset++] = '.';
	}
	buf[offset++] = size > 0 ? '|' : 'X';
	buf[offset] = '\0';
}

/*
 * variadic hexdump formatted
 * think of it as: printf("%s%s\n", vformat(fmt, ap), hexdump(data,size));
 */
static inline void UNUSED
vhexdumpf(const char *file, int line, const char *func, const CHAR16 * const fmt, const void *data, UINTN size, UINTN at, VA_LIST ap)
{
	UINTN display_offset = at;
	UINTN offset = 0;

	if (verbose == 0)
		return;

	while (offset < size) {
		char hexbuf[49];
		char txtbuf[19];
		UINTN sz;

		sz = prepare_hex(data+offset, size-offset, hexbuf,
				 (UINTN)data+offset);
		if (sz == 0)
			return;

		prepare_text(data+offset, size-offset, txtbuf,
			     (UINTN)data+offset);
		if (fmt && fmt[0] != 0)
			VLogError(file, line, func, fmt, ap);
		LogError_(file, line, func, L"%08zx  %a  %a\n", display_offset, hexbuf, txtbuf);

		display_offset += sz;
		offset += sz;
	}
}

/*
 * hexdump formatted
 * think of it as: printf("%s%s", format(fmt, ...), hexdump(data,size)[lineN]);
 */
static inline void UNUSED
hexdumpf(const char *file, int line, const char *func, const CHAR16 * const fmt, const void *data, UINTN size, UINTN at, ...)
{
	VA_LIST ap;

	VA_START(ap, at);
	vhexdumpf(file, line, func, fmt, data, size, at, ap);
	VA_END(ap);
}

static inline void UNUSED
hexdump(const char *file, int line, const char *func, const void *data, UINTN size)
{
	hexdumpf(file, line, func, L"", data, size, (INTN)data);
}

static inline void UNUSED
hexdumpat(const char *file, int line, const char *func, const void *data, UINTN size, UINTN at)
{
	hexdumpf(file, line, func, L"", data, size, at);
}

#define LogHexdump(data, sz) LogHexdump_(__FILE__, __LINE__, __func__, data, sz)
#define dhexdump(data, sz) hexdump(__FILE__, __LINE__, __func__, data, sz)
#define dhexdumpat(data, sz, at) hexdumpat(__FILE__, __LINE__, __func__, data, sz, at)
#define dhexdumpf(fmt, data, sz, at, ...) hexdumpf(__FILE__, __LINE__, __func__, fmt, data, sz, at, ##__VA_ARGS__)

#endif /* STATIC_HEXDUMP_H */
// vim:fenc=utf-8:tw=75:noet
