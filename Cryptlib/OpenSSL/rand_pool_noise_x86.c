/** @file
  Provide rand noise source.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <efi.h>
#include <efilib.h>

#include "rand_pool_noise.h"

#define BIT8		0x00000100
#define BIT10		0x00000400
#define BIT11		0x00000800

#define APIC_SVR	0x0f0
#define APIC_TMCCT	0x390

/**
  Reads the current value of Time Stamp Counter (TSC).

  Reads and returns the current value of TSC. This function is only available
  on IA-32 and X64.

  @return The current value of TSC

**/
static UINT64
AsmReadTsc (VOID)
{
  UINT32  LowData;
  UINT32  HiData;

  __asm__ __volatile__ (
    "rdtsc"
    : "=a" (LowData),
      "=d" (HiData)
    );

  return (((UINT64)HiData) << 32) | LowData;
}

/**
  Returns a 64-bit Machine Specific Register(MSR).

  Reads and returns the 64-bit MSR specified by Index. No parameter checking is
  performed on Index, and some Index values may cause CPU exceptions. The
  caller must either guarantee that Index is valid, or the caller must set up
  exception handlers to catch the exceptions. This function is only available
  on IA-32 and X64.

  @param  Index The 32-bit MSR index to read.

  @return The value of the MSR identified by Index.

**/
static UINT64
AsmReadMsr64 (UINT32 Index)
{
  UINT32 LowData;
  UINT32 HighData;

  __asm__ __volatile__ (
    "rdmsr"
    : "=a" (LowData),   // %0
      "=d" (HighData)   // %1
    : "c"  (Index)      // %2
    );

  return (((UINT64)HighData) << 32) | LowData;
}

/**
  Used to serialize load and store operations.

  All loads and stores that proceed calls to this function are guaranteed to be
  globally visible when this function returns.

**/
static inline VOID
MemoryFence (VOID)
{
  // This is a little bit of overkill and it is more about the compiler that it is
  // actually processor synchronization. This is like the _ReadWriteBarrier
  // Microsoft specific intrinsic
  __asm__ __volatile__ ("":::"memory");
}

/**
  Reads a 32-bit MMIO register.

  Reads the 32-bit MMIO register specified by Address. The 32-bit read value is
  returned. This function must guarantee that all MMIO read and write
  operations are serialized.

  If 32-bit MMIO register operations are not supported, then ASSERT().
  If Address is not aligned on a 32-bit boundary, then ASSERT().

  @param  Address The MMIO register to read.

  @return The value read.

**/
static UINT32
MmioRead32 (UINTN Address)
{
  UINT32 Value;

  ASSERT ((Address & 3) == 0);

  MemoryFence ();
  Value = *(volatile UINT32*)Address;
  MemoryFence ();

  return Value;
}

/**
  Internal function to retrieve the base address of local APIC.

  This function will ASSERT if:
  The local APIC is not globally enabled.
  The local APIC is not working under XAPIC mode.
  The local APIC is not software enabled.

  @return The base address of local APIC

**/
static UINTN
InternalX86GetApicBase (VOID)
{
  UINTN MsrValue;
  UINTN ApicBase;

  MsrValue = (UINTN) AsmReadMsr64 (27);
  ApicBase = MsrValue & 0xffffff000ULL;

  //
  // Check the APIC Global Enable bit (bit 11) in IA32_APIC_BASE MSR.
  // This bit will be 1, if local APIC is globally enabled.
  //
  ASSERT ((MsrValue & BIT11) != 0);

  //
  // Check the APIC Extended Mode bit (bit 10) in IA32_APIC_BASE MSR.
  // This bit will be 0, if local APIC is under XAPIC mode.
  //
  ASSERT ((MsrValue & BIT10) == 0);

  //
  // Check the APIC Software Enable/Disable bit (bit 8) in Spurious-Interrupt
  // Vector Register.
  // This bit will be 1, if local APIC is software enabled.
  //
  ASSERT ((MmioRead32 (ApicBase + APIC_SVR) & BIT8) != 0);

  return ApicBase;
}

/**
  Internal function to read the current tick counter of local APIC.

  @param  ApicBase  The base address of memory mapped registers of local APIC.

  @return The tick counter read.

**/
static INT32
InternalX86GetTimerTick (UINTN ApicBase)
{
  return MmioRead32 (ApicBase + APIC_TMCCT);
}

/**
  Get 64-bit noise source

  @param[out] Rand         Buffer pointer to store 64-bit noise source

  @retval TRUE             Get randomness successfully.
  @retval FALSE            Failed to generate
**/
BOOLEAN
GetRandomNoise64 (UINT64 *Rand)
{
  UINT32 Index;
  UINT32 *RandPtr;

  if (NULL == Rand) {
    return FALSE;
  }

  RandPtr = (UINT32 *)Rand;

  for (Index = 0; Index < 2; Index ++) {
    *RandPtr = (UINT32) ((AsmReadTsc ()) & 0xFF);
    RandPtr++;
    MicroSecondDelay (10);
  }

  return TRUE;
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
  return (UINT64)(UINT32)InternalX86GetTimerTick (InternalX86GetApicBase ());
}
