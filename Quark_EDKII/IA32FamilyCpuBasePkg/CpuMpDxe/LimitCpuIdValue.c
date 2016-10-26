/** @file

  Code for Max CPUID Limit Feature

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

  Module Name:  LimitCpuIdValue.c

**/

#include "LimitCpuIdValue.h"

BOOLEAN    EnableLimitCpuIdValue[FixedPcdGet32(PcdCpuMaxLogicalProcessorNumber)];

/**
  Detect capability of Max CPUID Limit feature for specified processor.
  
  This function detects capability of Max CPUID Limit feature for specified processor.

  @param  ProcessorNumber   The handle number of specified processor.

**/
VOID
MaxCpuidLimitDetect (
  UINTN   ProcessorNumber
  )
{
  EFI_CPUID_REGISTER  *CpuidRegisters;
  UINT8               MaxCpuid;

  CpuidRegisters   = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_SIGNATURE);
  ASSERT (CpuidRegisters != NULL);

  MaxCpuid         = (UINT8) CpuidRegisters->RegEax;
  if (MaxCpuid > 3) {
    SetProcessorFeatureCapability (ProcessorNumber, MaxCpuidValueLimit, NULL);
  }
}

/**
  Configures Processor Feature Lists for Max CPUID Limit feature for all processors.
  
  This function configures Processor Feature Lists for Max CPUID Limit feature for all processors.

**/
VOID
MaxCpuidLimitConfigFeatureList (
  VOID
  )
{
  UINTN                 ProcessorNumber;
  BOOLEAN               UserConfigurationSet;

  UserConfigurationSet = FALSE;
  if ((PcdGet32 (PcdCpuProcessorFeatureUserConfiguration) & PCD_CPU_MAX_CPUID_VALUE_LIMIT_BIT) != 0) {
    UserConfigurationSet = TRUE;
  }

  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {
    //
    // Check whether this logical processor supports Max CPUID Value Limit
    //
    if (GetProcessorFeatureCapability (ProcessorNumber, MaxCpuidValueLimit, NULL)) {
      if (ProcessorNumber == mCpuConfigConextBuffer.BspNumber) {
        //
        // Set the bit of Max CPUID Value Limit capability according to BSP capability.
        //
        PcdSet32 (PcdCpuProcessorFeatureCapability, PcdGet32 (PcdCpuProcessorFeatureCapability) | PCD_CPU_MAX_CPUID_VALUE_LIMIT_BIT);
        //
        // Set the bit of Max CPUID Value Limit setting according to BSP setting.
        //
        if (UserConfigurationSet) {
          PcdSet32 (PcdCpuProcessorFeatureSetting, PcdGet32 (PcdCpuProcessorFeatureSetting) | PCD_CPU_MAX_CPUID_VALUE_LIMIT_BIT);
        }
      }

      //
      // If this logical processor supports Max CPUID Value Limit, then add feature into feature list for operation
      // on corresponding register.
      //
      EnableLimitCpuIdValue[ProcessorNumber] = UserConfigurationSet;
      AppendProcessorFeatureIntoList (ProcessorNumber, MaxCpuidValueLimit, &(EnableLimitCpuIdValue[ProcessorNumber]));
    }
  }
}

/**
  Produces entry of Max CPUID Limit feature in Register Table for specified processor.
  
  This function produces entry of Max CPUID Limit feature in Register Table for specified processor.

  @param  ProcessorNumber   The handle number of specified processor.
  @param  Attribute         Pointer to the attribute

**/
VOID
MaxCpuidLimitReg (
  UINTN      ProcessorNumber,
  VOID       *Attribute
  )
{
  BOOLEAN    *Enable;
  UINT64     Value;

  //
  // If Attribute is TRUE, then write 1 to MSR_IA32_MISC_ENABLE[22].
  // Otherwise, write 0 to the bit.
  //
  Enable = (BOOLEAN *) Attribute;
  Value  = 0;
  if (*Enable) {
    Value = 1;
  }

  WriteRegisterTable (
    ProcessorNumber,
    Msr,
    MSR_IA32_MISC_ENABLE,
    N_MSR_LIMIT_CPUID_MAXVAL,
    1,
    Value
    );
}
