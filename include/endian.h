// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * endian.h - bswap decls that can't go in compiler.h
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef ENDIAN_H_
#define ENDIAN_H_

#include <stdint.h>

#include "system/builtins_begin_.h"
mkbi1_(uint16_t, bswap16, uint16_t, x)
mkbi1_(uint32_t, bswap32, uint32_t, x)
mkbi1_(uint64_t, bswap64, uint64_t, x)
#include "system/builtins_end_.h"

#endif /* !ENDIAN_H_ */
// vim:fenc=utf-8:tw=75:noet
