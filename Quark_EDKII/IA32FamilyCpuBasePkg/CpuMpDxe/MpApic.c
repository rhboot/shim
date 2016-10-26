/** @file

  Code for APIC feature.

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

  Module Name:  MpApic.c

**/

#include "MpApic.h"

/**
  Sort the APIC ID of all processors.

  This function sorts the APIC ID of all processors so that processor number is assigned in the
  ascending order of APIC ID which eases MP debugging. SMBIOS logic also depends on this assumption.

**/
VOID
SortApicId (
  VOID
  )
{
  UINTN   Index1;
  UINTN   Index2;
  UINTN   Index3;
  UINT32  ApicId;
  UINT32  Bist;

  if (mExchangeInfo->ApCount != 0) {
    mCpuConfigConextBuffer.NumberOfProcessors += mExchangeInfo->ApCount;

    for (Index1 = 0; Index1 < mExchangeInfo->ApCount; Index1++) {
      Index3 = Index1;
      //
      // Sort key is the hardware default APIC ID
      //
      ApicId = mExchangeInfo->BistBuffer[Index1].ApicId;
      for (Index2 = Index1 + 1; Index2 <= mExchangeInfo->ApCount; Index2++) {
        if (ApicId > mExchangeInfo->BistBuffer[Index2].ApicId) {
          Index3 = Index2;
          ApicId = mExchangeInfo->BistBuffer[Index2].ApicId;
        }
      }
      if (Index3 != Index1) {
        mExchangeInfo->BistBuffer[Index3].ApicId = mExchangeInfo->BistBuffer[Index1].ApicId;
        mExchangeInfo->BistBuffer[Index1].ApicId = ApicId;
        Bist = mExchangeInfo->BistBuffer[Index3].Bist;
        mExchangeInfo->BistBuffer[Index3].Bist = mExchangeInfo->BistBuffer[Index1].Bist;
        mExchangeInfo->BistBuffer[Index1].Bist = Bist;
      }
    }

    //
    // Get the processor number for the BSP
    //
    ApicId = GetInitialApicId ();
    for (Index1 = 0; Index1 < mCpuConfigConextBuffer.NumberOfProcessors; Index1++) {
      if (mExchangeInfo->BistBuffer[Index1].ApicId == ApicId) {
        mCpuConfigConextBuffer.BspNumber = Index1;
        break;
      }
    }
  }
}

/**
  Check if there is legacy APIC ID conflict among all processors.

  @retval EFI_SUCCESS      No coflict
  @retval EFI_UNSUPPORTED  There is legacy APIC ID conflict and can't be rsolved in xAPIC mode
**/
EFI_STATUS
CheckApicId (
  VOID
  )
{
  UINTN               Index1;
  UINTN               Index2;
  BOOLEAN             X2ApicCapable;

  X2ApicCapable = FALSE;

  //
  // If xAPIC mode is configured, then check if there is any APIC ID conflict.
  //
  ASSERT (mCpuConfigConextBuffer.NumberOfProcessors <= FixedPcdGet32 (PcdCpuMaxLogicalProcessorNumber));

  if (mCpuConfigConextBuffer.NumberOfProcessors > 255) {
    DEBUG ((EFI_D_ERROR, "Number of processors > 255!\n"));
    return EFI_UNSUPPORTED;
  } else {
    //
    // Check if there is 8-bit legacy APIC ID conflict with hardware default socket IDs.
    //
    for (Index1 = 0; Index1 < mCpuConfigConextBuffer.NumberOfProcessors; Index1++) {
     for (Index2 = Index1 + 1; Index2 < mCpuConfigConextBuffer.NumberOfProcessors; Index2++) {
       if ((UINT8) mExchangeInfo->BistBuffer[Index1].ApicId == (UINT8) mExchangeInfo->BistBuffer[Index2].ApicId) {
          break;
        }
      }
      if (Index2 < mCpuConfigConextBuffer.NumberOfProcessors) {
        DEBUG ((EFI_D_ERROR, "Legacy APIC ID conflict with hardware socket ID but x2APIC mode is not supported!\n"));
        return EFI_UNSUPPORTED;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Assign a package scope BSP responsible for package scope programming.

  This functions assigns the processor with the least APIC ID within a processor package as
  the package BSP.

**/
VOID
AssignPackageBsp (
  VOID
  )
{
  UINTN  Index;
  UINT32 PackageId;

  mCpuConfigConextBuffer.CollectedDataBuffer[0].PackageBsp = TRUE;
  PackageId = mCpuConfigConextBuffer.CollectedDataBuffer[0].ProcessorLocation.Package;
  for (Index = 1; Index < mCpuConfigConextBuffer.NumberOfProcessors; Index++) {
    if (PackageId != mCpuConfigConextBuffer.CollectedDataBuffer[Index].ProcessorLocation.Package) {
      PackageId = mCpuConfigConextBuffer.CollectedDataBuffer[Index].ProcessorLocation.Package;
      mCpuConfigConextBuffer.CollectedDataBuffer[Index].PackageBsp = TRUE;
    }
  }
}
