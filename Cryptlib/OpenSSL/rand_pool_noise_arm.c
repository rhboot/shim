/** @file
  Provide rand noise source.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <efi.h>
#include <efilib.h>

#include "rand_pool_noise.h"

/**
  Get 64-bit noise source

  @param[out] Rand         Buffer pointer to store 64-bit noise source

  @retval FALSE            Failed to generate
**/
BOOLEAN
GetRandomNoise64 (UINT64 *Rand)
{
  //
  // Return FALSE will fallback to use PerformaceCounter to
  // generate noise.
  //
  return FALSE;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  The counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
GetPerformanceCounter (VOID)
{
	UINT64 counter;
#ifdef MDE_CPU_ARM
	unsigned long tickl, ticku;
	asm volatile("mrrc p15, 0, %0, %1, c14" : "=r" (tickl), "=r" (ticku));
	counter == (UINT64)ticku << 32 | tickl;
#endif
#ifdef MDE_CPU_AARCH64
	asm volatile("mrs %0, cntpct_el0" : "=r" (counter));
#endif
	return counter;
}
