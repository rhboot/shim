// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SHIM_STR_H
#define SHIM_STR_H

static inline
__attribute__((unused))
unsigned long strnlena(const CHAR8 *s, unsigned long n)
{
	unsigned long i;
	for (i = 0; i <= n; i++)
		if (s[i] == '\0')
			break;
	return i;
}

static inline
__attribute__((unused))
CHAR8 *
strncpya(CHAR8 *dest, const CHAR8 *src, unsigned long n)
{
	unsigned long i;

	for (i = 0; i < n && src[i] != '\0'; i++)
		dest[i] = src[i];
	for (; i < n; i++)
		dest[i] = '\0';

	return dest;
}

static inline
__attribute__((unused))
CHAR8 *
strcata(CHAR8 *dest, const CHAR8 *src)
{
	unsigned long dest_len = strlena(dest);
	unsigned long i;

	for (i = 0; src[i] != '\0'; i++)
		dest[dest_len + i] = src[i];
	dest[dest_len + i] = '\0';

	return dest;
}

static inline
__attribute__((unused))
CHAR8 *
translate_slashes(CHAR8 *out, const char *str)
{
	int i;
	int j;
	if (str == NULL || out == NULL)
		return NULL;

	for (i = 0, j = 0; str[i] != '\0'; i++, j++) {
		if (str[i] == '\\') {
			out[j] = '/';
			if (str[i+1] == '\\')
				i++;
		} else
			out[j] = str[i];
	}
	out[j] = '\0';
	return out;
}

#endif /* SHIM_STR_H */
