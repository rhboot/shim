// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * elf-riscv64.h - riscv64-specific thingimawhatsits for elf
 * Copyright Peter Jones <pjones@redhat.com>
 */

#pragma once
#if (defined(__riscv__) || defined(__riscv)) && defined(__LP64__)

#include "elf64.h"

#define R_RISCV_NONE		0
#define REL_ARCH_NONE		R_RISCV_NONE
#define R_RISCV_RELATIVE	3
#define REL_ARCH_RELATIVE	R_RISCV_RELATIVE

#endif /* ! (defined(__riscv__) || defined(__riscv)) && defined(__LP64__) */
// vim:fenc=utf-8:tw=75:noet
