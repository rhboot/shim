/** @file
  Provide rand noise source.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __RAND_POOL_NOISE_H__
#define __RAND_POOL_NOISE_H__

#include <efi.h>
#include <efilib.h>

BOOLEAN GetRandomNoise64 (UINT64 *Rand);
UINT64 GetPerformanceCounter (VOID);
static inline UINTN MicroSecondDelay (UINTN MicroSeconds)
{
	gBS->Stall(MicroSeconds);
	return MicroSeconds;
}

#endif // __RAND_POOL_NOISE_H__
