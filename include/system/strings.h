// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifdef SHIM_UNIT_TEST
#include_next <strings.h>
#else
#ifndef _STRINGS_H
#define _STRINGS_H

#include <builtins_begin_.h>
mkbi1_(int, ffs, int, x)
mkbi1_(int, clz, int, x)
mkbi1_(int, ctz, int, x)
mkbi1_(int, clrsb, int, x)
mkbi1_(int, popcount, int, x)
mkbi1_(int, parity, int, x)
#include <builtins_end_.h>

#endif /* !_STRINGS_H */
#endif
// vim:fenc=utf-8:tw=75:noet
