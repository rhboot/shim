// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * elf-aa64.h - aa64-specific thingimawhatsits for elf
 * Copyright Peter Jones <pjones@redhat.com>
 */

#pragma once
#if defined(__aarch64__)

#include "elf64.h"

#define R_AARCH64_NONE		0
#define REL_ARCH_NONE		R_AARCH64_NONE
#define R_AARCH64_RELATIVE	1027
#define REL_ARCH_RELATIVE	R_AARCH64_RELATIVE

#endif /* !__aarch64__ */
// vim:fenc=utf-8:tw=75:noet
