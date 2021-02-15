// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifdef SHIM_UNIT_TEST
#include_next <alloca.h>
#else
#ifndef _ALLOCA_H
#define _ALLOCA_H

#include <builtins_begin_.h>
mkbi1_(void *, alloca, size_t, size)
#define alloca_with_align(size, alignment) __builtin_alloca_with_align(size, alignment)
#define alloca_with_align_and_max(size, alignment, max) __builtin_alloca_with_align_and_max(size, alignment, max)
#include <builtins_end_.h>

#endif /* !_ALLOCA_H */
#endif
// vim:fenc=utf-8:tw=75:noet
