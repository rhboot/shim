/** @file

  Memory Operation Functions for IA32 Architecture.

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

  Module Name:  ArchSpecific.c

**/

#include "Cpu.h"
#include "MpService.h"

MP_CPU_EXCHANGE_INFO    *mExchangeInfo;

/**
  Prepares Startup Vector for APs.

  This function prepares Startup Vector for APs.

**/
VOID
PrepareAPStartupVector (
  VOID
  )
{
  MP_ASSEMBLY_ADDRESS_MAP                     AddressMap;

  //
  // Get the address map of startup code for AP,
  // including code size, and offset of long jump instructions to redirect.
  //
  AsmGetAddressMap (&AddressMap);

  //
  // Allocate a 4K-aligned region under 1M for startup vector for AP.
  // The region contains AP startup code and exchange data between BSP and AP.
  //
  AllocateStartupVector (AddressMap.Size + sizeof (MP_CPU_EXCHANGE_INFO));

  //
  // Copy AP startup code to startup vector, and then redirect the long jump
  // instructions for mode switching.
  //
  CopyMem ((VOID *) (UINTN) mStartupVector, AddressMap.RendezvousFunnelAddress, AddressMap.Size);
  RedirectFarJump (&AddressMap, mStartupVector);

  //
  // Get the start address of exchange data between BSP and AP.
  //
  mExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN) (mStartupVector + AddressMap.Size);

  ZeroMem ((VOID *) mExchangeInfo, sizeof (MP_CPU_EXCHANGE_INFO));

  PrepareGdtIdtForAP (
    (IA32_DESCRIPTOR *) (UINTN) &mExchangeInfo->GdtrProfile,
    (IA32_DESCRIPTOR *) (UINTN) &mExchangeInfo->IdtrProfile
    );

  //
  // Assign ApLoopMode during POST phase
  // Set ApLoopMode to ApInHltLoop if ApLoopMode is ApInMwaitLoop
  // because ApInMwaitLoop could be supported only after C-State enabled.
  //
  if (PcdGet8 (PcdCpuApLoopMode) != ApInMwaitLoop) {
    mExchangeInfo->ApLoopMode = (AP_LOOP_MODE) (PcdGet8 (PcdCpuApLoopMode));
  } else {
    mExchangeInfo->ApLoopMode = ApInHltLoop;
  }

  mExchangeInfo->ApFunction  = ApProcEntry;
  mExchangeInfo->BufferStart = (UINT32) mStartupVector;
  mExchangeInfo->InitFlag    = 1;
}

/**
  Sets specified IDT entry with given function pointer.

  This function sets specified IDT entry with given function pointer.

  @param  FunctionPointer  Function pointer for IDT entry.
  @param  IdtEntry         The IDT entry to update.

  @return The original IDT entry value.

**/
UINTN
SetIdtEntry (
  IN  UINTN                       FunctionPointer,
  OUT INTERRUPT_GATE_DESCRIPTOR   *IdtEntry
)
{
  UINTN  OriginalEntry;

  OriginalEntry = ((UINT32) IdtEntry->OffsetHigh << 16) + IdtEntry->OffsetLow;

  IdtEntry->OffsetLow  = (UINT16) FunctionPointer;
  IdtEntry->OffsetHigh = (UINT16) (FunctionPointer >> 16);

  return OriginalEntry;
}

 /**
  Fixup jump instructions in the AP startup code.

  @param AddressMap    Pointer to MP_ASSEMBLY_ADDRESS_MAP.
  @param TargetBuffer  Target address of the startup vector.
**/
VOID
RedirectFarJump (
  IN MP_ASSEMBLY_ADDRESS_MAP *AddressMap,
  IN EFI_PHYSICAL_ADDRESS    TargetBuffer
  )
{
  *(UINT32 *)(UINTN)(mStartupVector + AddressMap->FlatJumpOffset + 3) = (UINT32)(TargetBuffer + AddressMap->PModeEntryOffset);
}

