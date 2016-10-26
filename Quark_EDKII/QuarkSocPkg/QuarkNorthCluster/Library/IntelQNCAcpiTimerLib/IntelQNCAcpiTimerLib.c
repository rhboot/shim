/** @file
  QNC ACPI Timer implements one instance of Timer Library.

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

//
// Include common header file for this module.
//
#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>

#define  V_ACPI_TMR_FREQUENCY        3579545
#define  V_ACPI_TMR_COUNT_SIZE       0x01000000
#define  LPC_PCI_BUS                 0x00
#define  LPC_PCI_DEV                 0x1F
#define  LPC_PCI_FUNC                0x00
#define  R_LPC_PM1BLK                0x48
#define  R_PM1BLK_PM1T               0x08

/**
  The constructor function enables ACPI IO space.

  If ACPI I/O space not enabled, this function will enable it.
  It will always return RETURN_SUCCESS.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
   EFIAPI
   IntelQNCAcpiTimerLibConstructor (
   VOID
   )
{  
   if ((PciRead32 (PCI_LIB_ADDRESS (LPC_PCI_BUS, LPC_PCI_DEV, LPC_PCI_FUNC, R_LPC_PM1BLK)) & BIT31) == 0) {
     //
     // If ACPI I/O space is not enabled, program ACPI I/O base address and enable it.
     //
     PciWrite32 (PCI_LIB_ADDRESS (LPC_PCI_BUS, LPC_PCI_DEV, LPC_PCI_FUNC, R_LPC_PM1BLK), BIT31 | PcdGet16 (PcdPm1blkIoBaseAddress));
   }
   return RETURN_SUCCESS;
}

/**
  Internal function to read the current tick counter of ACPI.

  Internal function to read the current tick counter of ACPI.

  @return The tick counter read.

**/
STATIC
   UINT32
   InternalAcpiGetTimerTick (
   VOID
   )
{
   return IoRead32 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_PM1BLK_PM1T);
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
STATIC
   VOID
   InternalAcpiDelay (
   IN      UINT32                    Delay
   )
{
   UINT32                            Ticks;
   UINT32                            Times;

   Times    = Delay >> 22;
   Delay   &= BIT22 - 1;
   do {
      //
      // The target timer count is calculated here
      //
      Ticks    = InternalAcpiGetTimerTick () + Delay;
      Delay    = BIT22;
      //
      // Wait until time out
      // Delay >= 2^23 could not be handled by this function
      // Timer wrap-arounds are handled correctly by this function
      //
      while ( ((Ticks - InternalAcpiGetTimerTick ()) & BIT23) == 0 ) {
         CpuPause ();
      }
   } while ( Times-- > 0 );
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
   InternalAcpiDelay (
      (UINT32)DivU64x32 (
      MultU64x32 (
      MicroSeconds,
      V_ACPI_TMR_FREQUENCY
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
   InternalAcpiDelay (
      (UINT32)DivU64x32 (
      MultU64x32 (
      NanoSeconds,
      V_ACPI_TMR_FREQUENCY
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
  return  AsmReadTsc();
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
   if ( StartValue != NULL ) {
      *StartValue = 0;
   }

   if ( EndValue != NULL ) {
      *EndValue = V_ACPI_TMR_COUNT_SIZE - 1;
   }

   return V_ACPI_TMR_FREQUENCY;
}
