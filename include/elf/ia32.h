// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * elf-ia32.h - ia32-specific thingimawhatsits for elf
 * Copyright Peter Jones <pjones@redhat.com>
 */

#pragma once
#if defined(__i386__)

#include "elf32.h"

#define R_386_NONE		0
#define REL_ARCH_NONE		R_386_NONE
#define R_386_RELATIVE		8
#define REL_ARCH_RELATIVE	R_386_RELATIVE

#endif /* !__i386__ */
// vim:fenc=utf-8:tw=75:noet
