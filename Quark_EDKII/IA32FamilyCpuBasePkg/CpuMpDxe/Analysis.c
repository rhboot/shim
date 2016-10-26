/** @file
  Code for Analysis phase.

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

  Module Name:  Analysis.c

**/

#include "Cpu.h"
#include "Feature.h"

/**
  Configures Processor Feature List for all processors.

  This function configures Processor Feature List for all processors.

**/
VOID
ConfigProcessorFeatureList (
  VOID
  )
{
  //
  // Configure Feature List for Max CPUID Value Limit
  //
  if (FeaturePcdGet (PcdCpuMaxCpuIDValueLimitFlag)) {
    MaxCpuidLimitConfigFeatureList ();
  }
  //
  // Configure Feature List for execute disable bit
  //
  if (FeaturePcdGet (PcdCpuExecuteDisableBitFlag)) {
    XdConfigFeatureList ();
  }
}

/**
  Produces Register Tables for all processors.

  This function produces Register Tables for all processors.

**/
VOID
ProduceRegisterTable (
  VOID
  )
{

  UINTN          ProcessorNumber;
  UINT8          Index;
  CPU_FEATURE_ID FeatureID;
  VOID           *Attribute;

  //
  // Parse Processor Feature Lists and translate to Register Tables for all processors.
  //
  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {
    
    //
    // Traverse Processor Feature List for this logical processor.
    //
    Index = 1;
    FeatureID = GetProcessorFeatureEntry (ProcessorNumber, Index, &Attribute);
    Index++;
    while (FeatureID != CpuFeatureMaximum) {

      switch (FeatureID) {

      case MaxCpuidValueLimit:
        if (FeaturePcdGet (PcdCpuMaxCpuIDValueLimitFlag)) {
          MaxCpuidLimitReg (ProcessorNumber, Attribute);
        }
        break;

      case ExecuteDisableBit:
        if (FeaturePcdGet (PcdCpuExecuteDisableBitFlag)) {
          XdReg (ProcessorNumber, Attribute);
        }
        break;

      default:
        break;
      }

      FeatureID = GetProcessorFeatureEntry (ProcessorNumber, Index, &Attribute);
      Index++;
    }
  }
}

/**
  Produces Pre-SMM-Init Register Tables for all processors.

  This function produces Pre-SMM-Init Register Tables for all processors.

**/
VOID
ProducePreSmmInitRegisterTable (
  VOID
  )
{
  UINTN  ProcessorNumber;
  UINT32 ApicId;

  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {
    ApicId = mCpuConfigConextBuffer.CollectedDataBuffer[ProcessorNumber].CpuMiscData.InitialApicID;
    mCpuConfigConextBuffer.PreSmmInitRegisterTable[ProcessorNumber].InitialApicId = ApicId;
  }
}

/**
  Produces register table according to output of Data Collection phase.
  
  This function produces register table according to output of Data Collection phase.

**/
VOID
AnalysisPhase (
  VOID
  )
{
  UINTN                     Index;
  UINTN                     ProcessorNumber;
  CPU_REGISTER_TABLE        *RegisterTable;
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntry;
  UINT8                     CallbackSignalValue;

  //
  // Set PcdCpuCallbackSignal to trigger callback function, and reads the value back.
  //
  CallbackSignalValue = SetAndReadCpuCallbackSignal (CPU_PROCESSOR_FEATURE_LIST_CONFIG_SIGNAL);
  //
  // Checks whether the callback function requests to bypass Processor Feature List configuration.
  //
  if (CallbackSignalValue != CPU_BYPASS_SIGNAL) {
    //
    // Configure Processor Feature List for all logical processors.
    //
    ConfigProcessorFeatureList ();
  }

  //
  // Set PcdCpuCallbackSignal to trigger callback function, and reads the value back.
  //
  CallbackSignalValue = SetAndReadCpuCallbackSignal (CPU_REGISTER_TABLE_TRANSLATION_SIGNAL);
  //
  // Checks whether the callback function requests to bypass Register Table translation.
  //
  if (CallbackSignalValue != CPU_BYPASS_SIGNAL) {
    //
    // Produce register tables for all logical processors.
    //
    ProduceRegisterTable ();
  }

  //
  // Debug information
  //
  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {

    RegisterTable = &mCpuConfigConextBuffer.RegisterTable[ProcessorNumber];
    for (Index = 0; Index < RegisterTable->TableLength; Index++) {
      RegisterTableEntry = &RegisterTable->RegisterTableEntry[Index];
      DEBUG ((EFI_D_INFO, "Processor: %d: MSR: %x, Bit Start: %d, Bit Length: %d, Value: %lx\r\n", ProcessorNumber, RegisterTableEntry->Index, RegisterTableEntry->ValidBitStart, RegisterTableEntry->ValidBitLength, RegisterTableEntry->Value));
    }
  }
  DEBUG ((EFI_D_INFO, "Capability: %8x\r\n", PcdGet32 (PcdCpuProcessorFeatureCapability)));
  DEBUG ((EFI_D_INFO, "Configuration: %8x\r\n", PcdGet32 (PcdCpuProcessorFeatureUserConfiguration)));
  DEBUG ((EFI_D_INFO, "Setting: %8x\r\n", PcdGet32 (PcdCpuProcessorFeatureSetting)));
}
