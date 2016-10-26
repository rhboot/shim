/** @file

  Code for Execute Disable Bit Feature

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

  Module Name:  Xd.c

**/

#include "Xd.h"

BOOLEAN    EnableExecuteDisableBit[FixedPcdGet32(PcdCpuMaxLogicalProcessorNumber)];

/**
  Detect capability of XD feature for specified processor.
  
  This function detects capability of XD feature for specified processor.

  @param  ProcessorNumber   The handle number of specified processor.

**/
VOID
XdDetect (
  UINTN   ProcessorNumber
  )
{
  EFI_CPUID_REGISTER  *CpuidRegisters;

  //
  // Check whether 0x80000001 is supported by CPUID
  //
  if (GetNumberOfCpuidLeafs (ProcessorNumber, ExtendedCpuidLeaf) > 2) {
    //
    // Check CPUID(0x80000001).EDX[20]
    //
    CpuidRegisters = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_EXTENDED_CPU_SIG);
    ASSERT (CpuidRegisters != NULL);

    if (BitFieldRead32 (CpuidRegisters->RegEdx, N_CPUID_XD_BIT_AVAILABLE, N_CPUID_XD_BIT_AVAILABLE) == 1) {
      SetProcessorFeatureCapability (ProcessorNumber, ExecuteDisableBit, NULL);
    }
  }
}

/**
  Configures Processor Feature Lists for XD feature for all processors.
  
  This function configures Processor Feature Lists for XD feature for all processors.

**/
VOID
XdConfigFeatureList (
  VOID
  )
{
  UINTN                 ProcessorNumber;
  BOOLEAN               XdSupported;
  BOOLEAN               Setting;

  XdSupported = TRUE;
  Setting = FALSE;

  //
  // Check whether all logical processors support XD
  //
  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {
    if (!GetProcessorFeatureCapability (ProcessorNumber, ExecuteDisableBit, NULL)) {
      XdSupported = FALSE;
      break;
    }
  }
  
  if (XdSupported) {
    //
    // Set the bit of XD capability
    //
    PcdSet32 (PcdCpuProcessorFeatureCapability, PcdGet32 (PcdCpuProcessorFeatureCapability) | PCD_CPU_EXECUTE_DISABLE_BIT);
    //
    // Checks whether user indicates to enable XD
    //
    if ((PcdGet32 (PcdCpuProcessorFeatureUserConfiguration) & PCD_CPU_EXECUTE_DISABLE_BIT) != 0) {
      //
      // Set the bit of XD setting
      //
      PcdSet32 (PcdCpuProcessorFeatureSetting, PcdGet32 (PcdCpuProcessorFeatureSetting) | PCD_CPU_EXECUTE_DISABLE_BIT);
      Setting = TRUE;
    }
  }

  //
  // If XD is not supported by all logical processors, or user indicates to disable XD, then disable XD on all processors.
  //
  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {
    //
    // The operation can only be performed on the processors supporting XD feature.
    //
    if (GetProcessorFeatureCapability (ProcessorNumber, ExecuteDisableBit, NULL)) {
      EnableExecuteDisableBit[ProcessorNumber] = Setting;
      AppendProcessorFeatureIntoList (ProcessorNumber, ExecuteDisableBit, &(EnableExecuteDisableBit[ProcessorNumber]));
    }
  }
}

/**
  Produces entry of XD feature in Register Table for specified processor.
  
  This function produces entry of XD feature in Register Table for specified processor.

  @param  ProcessorNumber   The handle number of specified processor.
  @param  Attribute         Pointer to the attribute

**/
VOID
XdReg (
  UINTN      ProcessorNumber,
  VOID       *Attribute
  )
{
  BOOLEAN    *Enable;
  UINT64     Value;

  //
  // If Attribute is TRUE, then write 0 to MSR_IA32_MISC_ENABLE[34].
  // Otherwise, write 1 to the bit.
  //
  Enable = (BOOLEAN *) Attribute;
  Value  = 1;
  if (*Enable) {
    Value = 0;
  }

  WriteRegisterTable (
    ProcessorNumber,
    Msr,
    MSR_IA32_MISC_ENABLE,
    N_MSR_XD_BIT_DISABLE,
    1,
    Value
    );
}
