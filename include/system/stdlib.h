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

#endif /* !_STDLIB_H */
#endif
// vim:fenc=utf-8:tw=75:noet
