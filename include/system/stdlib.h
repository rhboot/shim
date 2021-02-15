// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifdef SHIM_UNIT_TEST
#include_next <stdlib.h>
#else
#ifndef _STDLIB_H
#define _STDLIB_H

/*
 * I don't know why, but openssl expects to get size_t from stdlib.h
 * instead of stddef.h, so... whatever.
 */
#include <stddef.h>

static inline void abort(void) { }

#include <builtins_begin_.h>
mkbi1_(int, abs, int, j)
mkbi1_(long int, labs, long int, j)
mkbi1_(long long int, llabs, long long int, j)

#ifdef _INTTYPES_H
mkbi1_(intmax_t, imaxabs, intmax_t, j)
#endif /* _INTTYPES_H */
#include <builtins_end_.h>

#endif /* !_STDLIB_H */
#endif
// vim:fenc=utf-8:tw=75:noet
