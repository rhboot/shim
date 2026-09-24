// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * elf-arm.h - arm-specific thingimawhatsits for elf
 * Copyright Peter Jones <pjones@redhat.com>
 */

#pragma once
#if defined(__arm__)

#include "elf32.h"

#define R_ARM_NONE		0
#define REL_ARCH_NONE		R_ARM_NONE
#define R_ARM_RELATIVE		8
#define REL_ARCH_RELATIVE	R_ARM_RELATIVE

#endif /* !__arm__ */
// vim:fenc=utf-8:tw=75:noet
