// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifdef SHIM_UNIT_TEST
#include_next <string.h>

__typeof__(strlen) shim_strlen;
__typeof__(strcmp) shim_strcmp;
__typeof__(strncmp) shim_strncmp;
__typeof__(strncasecmp) shim_strncasecmp;
__typeof__(strcasecmp) shim_strcasecmp;
__typeof__(strrchr) shim_strrchr;
__typeof__(strrchr) shim_strrchr;
__typeof__(strnlen) shim_strnlen;
__typeof__(strcpy) shim_strcpy;
__typeof__(strncpy) shim_strncpy;
__typeof__(strdup) shim_strdup;
__typeof__(strndup) shim_strndup;
__typeof__(stpcpy) shim_stpcpy;
__typeof__(strchrnul) shim_strchrnul;
__typeof__(strchr) shim_strchr;

#else
#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

#include <builtins_begin_.h>

mkbi1_(long int, ffsl, long int, x)
mkbi1_(long int, clzl, long int, x)
mkbi1_(long int, ctzl, long int, x)
mkbi1_(long int, clrsbl, long int, x)
mkbi1_(long int, popcountl, long int, x)
mkbi1_(long int, parityl, long int, x)
mkbi1_(long long int, ffsll, long long int, x)
mkbi1_(long long int, clzll, long long int, x)
mkbi1_(long long int, ctzll, long long int, x)
mkbi1_(long long int, clrsbll, long long int, x)
mkbi1_(long long int, popcountll, long long int, x)
mkbi1_(long long int, parityll, long long int, x)

mkbi3_(int, bcmp, const void *, s1, const void *, s2, size_t, n)
mkbi3_(void, bcopy, const void *, src, void *, dest, size_t, n)
mkbi2_(void, bzero, void *, s, size_t, n)
mkdepbi2_(char *, index, const char *, s, int, c)
mkbi3_(void *, memchr, const void *, s, int, c, size_t, n)
mkbi3_(int, memcmp, const void *, s1, const void *, s2, size_t, n)
mkbi3_(void *, memcpy, void *, dest, const void *, src, size_t, n)
mkbi3_(void *, memmove, void *, dest, const void *, src, size_t, n)
mkbi3_(void *, mempcpy, void *, dest, const void *, src, size_t, n)
mkdepbi2_(char *, rindex, const char *, s, int, c)
mkdepbi2_(char *, stpcpy, char *, dest, const char *, src)
mkbi3_(char *, stpncpy, char *, dest, const char *, src, size_t, n)
mkdepbi2_(int, strcasecmp, const char *, s1, const char *, s2)
mkdepbi2_(char *, strcat, char *, dest, const char *, src)
mkdepbi2_(char *, strchr, const char *, s, int, c)
mkdepbi2_(int, strcmp, const char *, s1, const char *, s2)
mkdepbi2_(char *, strcpy, char *, dest, const char *, src)
mkdepbi2_(size_t, strcspn, const char *, s, const char *, reject)
mkdepbi1_(char *, strdup, const char *, s)
mkbi2_(char *, strndup, const char *, s, size_t, n)
mkdepbi1_(size_t, strlen, const char *, s)
mkbi3_(int, strncasecmp, const char *, s1, const char *, s2, size_t, n)
mkbi3_(char *, strncat, char *, dest, const char *, src, size_t, n)
mkbi3_(int, strncmp, const char *, s1, const char *, s2, size_t, n)
mkbi3_(char *, strncpy, char *, dest, const char *, src, size_t, n)
#if defined(__GNUC__) && __GNUC__ >= 9
mkbi2_(size_t, strnlen, const char *, s1, size_t, n)
#else
size_t strnlen(const char * s1, size_t n);
#endif
mkdepbi2_(char *, strpbrk, const char *, s, const char *, accept)
mkdepbi2_(char *, strrchr, const char *, s, int, c)
mkdepbi2_(size_t, strspn, const char *, s, const char *, accept)
mkdepbi2_(char *, strstr, const char *, haystack, const char *, needle)

mkbi3_(void *, memset, void *, s, int, c, size_t, n);

#include <builtins_end_.h>

#endif /* _STRING_H */
#endif
