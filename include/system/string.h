// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifdef SHIM_UNIT_TEST
#include_next <string.h>
#else
#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

__typeof__(__builtin_memset) memset;
__typeof__(__builtin_memcpy) memcpy;

#endif /* _STRING_H */
#endif
