// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * string.c - implementations we need for string finctions
 */
#define SHIM_STRING_C_
#include "shim.h"

#ifdef SHIM_UNIT_TEST
#define strlen shim_strlen
#ifdef strcmp
#undef strcmp
#endif
#define strcmp shim_strcmp
#ifdef strncmp
#undef strncmp
#endif
#define strncmp shim_strncmp
#define strncasecmp shim_strncasecmp
#define strcasecmp shim_strcasecmp
#define strrchr shim_strrchr
#define strlen shim_strlen
#define strnlen shim_strnlen
#define strcpy shim_strcpy
#ifdef strncpy
#undef strncpy
#endif
#define strncpy shim_strncpy
#ifdef strdup
#undef strdup
#endif
#define strdup shim_strdup
#ifdef strndup
#undef strndup
#endif
#define strndup shim_strndup
#ifdef stpcpy
#undef stpcpy
#endif
#define stpcpy shim_stpcpy
#define strchrnul shim_strchrnul
#ifdef strchr
#undef strchr
#endif
#define strchr shim_strchr
#endif

size_t
strlen(const char *s1)
{
	size_t len;

	for (len = 0; *s1; s1 += 1, len += 1)
		;
	return len;
}

int
strcmp(const char *s1p, const char *s2p)
{
	const uint8_t *s1 = (const uint8_t *)s1p;
	const uint8_t *s2 = (const uint8_t *)s2p;

	while (*s1) {
		if (*s1 != *s2) {
			break;
		}

		s1 += 1;
		s2 += 1;
	}

	return *s1 - *s2;
}

int
strncmp(const char *s1p, const char *s2p, size_t len)
{
	const uint8_t *s1 = (const uint8_t *)s1p;
	const uint8_t *s2 = (const uint8_t *)s2p;

	while (*s1 && len) {
		if (*s1 != *s2) {
			break;
		}

		s1 += 1;
		s2 += 1;
		len -= 1;
	}

	return len ? *s1 - *s2 : 0;
}

/* Based on AsciiStriCmp() in edk2 MdePkg/Library/BaseLib/String.c */
int
strncasecmp(const char *s1p, const char *s2p, size_t n)
{
	const uint8_t *s1 = (const uint8_t *)s1p;
	const uint8_t *s2 = (const uint8_t *)s2p;

	while (*s1 && n) {
		if (toupper(*s1) != toupper(*s2)) {
			break;
		}

		s1 += 1;
		s2 += 1;
		n -= 1;
	}

	return n ? *s1 - *s2 : 0;
}

/* Based on AsciiStriCmp() in edk2 MdePkg/Library/BaseLib/String.c */
int
strcasecmp(const char *str1, const char *str2)
{
	uint8_t c1, c2;

	c1 = toupper(*str1);
	c2 = toupper(*str2);
	while ((*str1 != '\0') && (c1 == c2)) {
		str1++;
		str2++;
		c1 = toupper(*str1);
		c2 = toupper(*str2);
	}

	return c1 - c2;
}

/* Scan a string for the last occurrence of a character */
char *
strrchr(const char *str, int c)
{
	char *save;

	for (save = NULL;; ++str) {
		if (*str == c) {
			save = (char *)str;
		}
		if (*str == 0) {
			return (save);
		}
	}
}

NONNULL(1)
size_t
strnlen(const char *s, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
		if (s[i] == '\0')
			break;
	return i;
}

RETURNS_NONNULL NONNULL(1, 2)
char *
strcpy(char *dest, const char *src)
{
	size_t i;

	for (i = 0; src[i] != '\0'; i++)
		dest[i] = src[i];

	dest[i] = '\0';
	return dest;
}

RETURNS_NONNULL NONNULL(1, 2)
char *
strncpy(char *dest, const char *src, size_t n)
{
	size_t i;

	for (i = 0; i < n && src[i] != '\0'; i++)
		dest[i] = src[i];
	if (i < n)
		dest[i] = '\0';

	return dest;
}

RETURNS_NONNULL NONNULL(1, 2)
char *
strcat(char *dest, const char *src)
{
	size_t dest_len = strlen(dest);
	size_t i;

	for (i = 0; src[i] != '\0'; i++)
		dest[dest_len + i] = src[i];
	dest[dest_len + i] = '\0';

	return dest;
}

NONNULL(1)
char *
strdup(const char *const src)
{
	size_t len;
	char *news = NULL;

	len = strlen(src);
	news = AllocateZeroPool(len + 1);
	if (news)
		strncpy(news, src, len);
	return news;
}

NONNULL(1)
char *
strndup(const char *const src, const size_t srcmax)
{
	size_t len;
	char *news = NULL;

	len = strnlen(src, srcmax);
	news = AllocateZeroPool(len + 1);
	if (news)
		strncpy(news, src, len);
	return news;
}

RETURNS_NONNULL NONNULL(1, 2)
char *
stpcpy(char *dest, const char *const src)
{
	size_t i = 0;
	for (i = 0; src[i]; i++)
		dest[i] = src[i];
	dest[i] = '\000';
	return &dest[i];
}

RETURNS_NONNULL NONNULL(1)
char *
strchrnul(const char *s, int c)
{
	unsigned int i;

	for (i = 0; s[i] != '\000' && s[i] != c; i++)
		;

	return (char *)&s[i];
}

NONNULL(1)
char *
strchr(const char *s, int c)
{
	const char *s1;

	s1 = strchrnul(s, c);
	if (!s1 || s1[0] == '\000')
		return NULL;

	return (char *)s1;
}

char *
translate_slashes(char *out, const char *str)
{
	int i;
	int j;
	if (str == NULL || out == NULL)
		return NULL;

	for (i = 0, j = 0; str[i] != '\0'; i++, j++) {
		if (str[i] == '\\') {
			out[j] = '/';
			if (str[i + 1] == '\\')
				i++;
		} else
			out[j] = str[i];
	}
	out[j] = '\0';
	return out;
}

// vim:fenc=utf-8:tw=75:noet
