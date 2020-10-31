/*
 * delay.c
 * Copyright 2019 Peter Jones <pjones@redhat.com>
 *
 */
#include <efi.h>
#include <efilib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "shim.h"

UINTN
EFIAPI
MicroSecondDelay(UINTN MicroSeconds)
{
	gBS->Stall(MicroSeconds);
	return MicroSeconds;
}

UINTN
EFIAPI
NanoSecondDelay(UINTN NanoSeconds)
{
	gBS->Stall(NanoSeconds / 1000);
	return NanoSeconds;
}

// vim:fenc=utf-8:tw=75:noet
