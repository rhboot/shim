/** @file
  Timer Library functions built upon local APIC on IA32/x64.

Copyright (c) 2013 Intel Corporation.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
* Neither the name of Intel Corporation nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <Base.h>

#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SocketLga775Lib.h>
#include <Library/LocalApicLib.h>

//
// The following array is FSB frequencies defined in Pentinum family
// CPUs, its value unit is HZ.
//
GLOBAL_REMOVE_IF_UNREFERENCED
CONST UINT32                          mPentinumFSBFrequencies[] = {
  400000000
};

/**
  The function to get CPU intended FSB frequency.

  This function reads the type of CPU by CPUID and returns FSB frequecny,
  if CPU not supportted, then ASSERT().

  @retval CPU intended FSB frequency.

**/
UINT32
GetIntendFsbFrequency (
  VOID
  )
{
  UINT32              RegEax;
  UINT32              BasicFamilyId;
  UINT32              FamilyId;
  UINT32              ModelId;
  UINT32              ExtendedModelId;

  AsmCpuid (EFI_CPUID_VERSION_INFO, &RegEax, NULL, NULL, NULL);

  //
  // The Extended Family ID needs to be examined only when the Family ID is 0FH.
  //
  BasicFamilyId = BitFieldRead32 (RegEax, 8, 11);
  FamilyId      = BasicFamilyId;
  if (BasicFamilyId == 0x0F) {
    FamilyId += BitFieldRead32 (RegEax, 20, 27);
  }

  //
  // The Extended Model ID needs to be examined only when the Family ID is 06H or 0FH.
  //
  ModelId = BitFieldRead32 (RegEax, 4, 7);
  if (BasicFamilyId == 0x06 || BasicFamilyId == 0x0f) {
    ExtendedModelId = BitFieldRead32 (RegEax, 16, 19);
    ModelId        += ExtendedModelId << 4;
  }

  switch (FamilyId) {
  case PENTIUM_FAMILY_ID:
    //
    // It's Pentinum Family CPUs.
    //
    switch (ModelId) {
    case QUARK_MODEL_ID:
      return mPentinumFSBFrequencies[0];
    }
    break;
  }

  //
  // Always assert() for those unsupported CPUs
  //
  ASSERT (FALSE);

  return (UINT32) -1;
}


/**
  Internal function to return the frequency of the local APIC timer.

  Internal function to return the frequency of the local APIC timer.

  @return The frequency of the timer in Hz.

**/
UINT32
InternalX86GetTimerFrequency (
  VOID
  )
{
  UINT32 Freq;
  UINTN  DivideValue;

  Freq = GetIntendFsbFrequency ();
  GetApicTimerState (&DivideValue, NULL, NULL);
  return Freq / ((UINT32) DivideValue);
}

/**
  Internal function to read the current tick counter of local APIC.

  Internal function to read the current tick counter of local APIC.

  @return The tick counter read.

**/
INT32
InternalX86GetTimerTick (
  VOID
  )
{
  return GetApicTimerCurrentCount ();
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
VOID
InternalX86Delay (
  IN      UINT32                    Delay
  )
{
  INT32                             Ticks;

  //
  // The target timer count is calculated here
  //
  Ticks = InternalX86GetTimerTick () - Delay;

  //
  // Wait until time out
  // Delay > 2^31 could not be handled by this function
  // Timer wrap-arounds are handled correctly by this function
  //
  while (InternalX86GetTimerTick () - Ticks >= 0);
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  InternalX86Delay (
    (UINT32)DivU64x32 (
              MultU64x64 (
                InternalX86GetTimerFrequency (),
                MicroSeconds
                ),
              1000000u
              )
    );
  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return NanoSeconds

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN                     NanoSeconds
  )
{
  InternalX86Delay (
    (UINT32)DivU64x32 (
              MultU64x64 (
                InternalX86GetTimerFrequency (),
                NanoSeconds
                ),
              1000000000u
              )
    );
  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  Retrieves the current value of a 64-bit free running performance counter. The
  counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return (UINT64)(UINT32)InternalX86GetTimerTick ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the performance counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the performance counter in Hz is always returned. If StartValue
  is less than EndValue, then the performance counter counts up. If StartValue
  is greater than EndValue, then the performance counter counts down. For
  example, a 64-bit free running counter that counts up would have a StartValue
  of 0 and an EndValue of 0xFFFFFFFFFFFFFFFF. A 24-bit free running counter
  that counts down would have a StartValue of 0xFFFFFF and an EndValue of 0.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = GetApicTimerInitCount ();
  }

  if (EndValue != NULL) {
    *EndValue = 0;
  }

  return (UINT64) InternalX86GetTimerFrequency ();
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN      UINT64                     Ticks
  )
{
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder = RShiftU64 (Remainder, (UINTN) Shift);
  Frequency = RShiftU64 (Frequency, (UINTN) Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}
