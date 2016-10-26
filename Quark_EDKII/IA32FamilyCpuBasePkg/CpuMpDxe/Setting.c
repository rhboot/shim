/** @file
  Code for Setting phase.

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

  Module Name:  Setting.c

**/

#include "Cpu.h"
#include "MpService.h"
#include "CpuOnlyReset.h"

BOOLEAN    mRestoreSettingAfterInit = FALSE;
BOOLEAN    mSetBeforeCpuOnlyReset;

/**
  Programs registers for the calling processor.

  This function programs registers for the calling processor.

  @param  PreSmmInit         Specify the target register table.
                             If TRUE, the target is the pre-SMM-init register table.
                             If FALSE, the target is the post-SMM-init register table.
  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
SetProcessorRegisterEx (
  IN BOOLEAN  PreSmmInit,
  IN UINTN    ProcessorNumber
  )
{
  CPU_REGISTER_TABLE        *RegisterTable;
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntry;
  UINTN                     Index;
  UINTN                     Value;
  UINTN                     StartIndex;
  UINTN                     EndIndex;

  if (PreSmmInit) {
    RegisterTable = &mCpuConfigConextBuffer.PreSmmInitRegisterTable[ProcessorNumber];
  } else {
    RegisterTable = &mCpuConfigConextBuffer.RegisterTable[ProcessorNumber];
  }
  
  //
  // If microcode patch has been applied, then the first register table entry
  // is for microcode upate, so it is skipped.  
  //
  StartIndex = 0;

  if (mSetBeforeCpuOnlyReset) {
    EndIndex    = StartIndex + RegisterTable->NumberBeforeReset;
  } else {
    StartIndex += RegisterTable->NumberBeforeReset;
    EndIndex    = RegisterTable->TableLength;
  }

  //
  // Traverse Register Table of this logical processor
  //
  for (Index = StartIndex; Index < EndIndex; Index++) {

    RegisterTableEntry = &RegisterTable->RegisterTableEntry[Index];
    
    //
    // Check the type of specified register
    //
    switch (RegisterTableEntry->RegisterType) {
    //
    // The specified register is Control Register
    //
    case ControlRegister:
      switch (RegisterTableEntry->Index) {
      case 0:
        Value = AsmReadCr0 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          RegisterTableEntry->Value
                          );
        AsmWriteCr0 (Value);
        break;
      case 2:
        Value = AsmReadCr2 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          RegisterTableEntry->Value
                          );
        AsmWriteCr2 (Value);
        break;
      case 3:
        Value = AsmReadCr3 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          RegisterTableEntry->Value
                          );
        AsmWriteCr3 (Value);
        break;
      case 4:
        Value = AsmReadCr4 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          RegisterTableEntry->Value
                          );
        AsmWriteCr4 (Value);
        break;
      case 8:
        //
        //  Do we need to support CR8?
        //
        break;
      default:
        break;
      }
      break;
    //
    // The specified register is Model Specific Register
    //
    case Msr:
      //
      // If this function is called to restore register setting after INIT signal,
      // there is no need to restore MSRs in register table.
      //
      if (!mRestoreSettingAfterInit) {
        if (RegisterTableEntry->ValidBitLength >= 64) {
          //
          // If length is not less than 64 bits, then directly write without reading
          //
          AsmWriteMsr64 (
            RegisterTableEntry->Index,
            RegisterTableEntry->Value
            );
        } else {
          //
          // Set the bit section according to bit start and length
          //
          AsmMsrBitFieldWrite64 (
            RegisterTableEntry->Index,
            RegisterTableEntry->ValidBitStart,
            RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
            RegisterTableEntry->Value
            );
        }
      }
      break;
    //
    // Enable or disable cache
    //
    case CacheControl:
      //
      // If value of the entry is 0, then disable cache.  Otherwise, enable cache.
      //
      if (RegisterTableEntry->Value == 0) {
        AsmDisableCache ();
      } else {
        AsmEnableCache ();
      }
      break;

    default:
      break;
    }
  }
}

/**
  Programs registers after SMM initialization for the calling processor.

  This function programs registers after SMM initialization for the calling processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
SetProcessorRegister (
  IN UINTN  ProcessorNumber
  )
{
  SetProcessorRegisterEx (FALSE, ProcessorNumber);
}

/**
  Programs registers before SMM initialization for the calling processor.

  This function programs registers before SMM initialization for the calling processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
SetPreSmmInitProcessorRegister (
  IN UINTN  ProcessorNumber
  )
{
  SetProcessorRegisterEx (TRUE, ProcessorNumber);
}

/**
  Triggers CPU-only reset and restores processor environment.

  This function triggers CPU-only reset and restores processor environment.

**/
VOID
CpuOnlyResetAndRestore (
  VOID
  )
{
  MTRR_SETTINGS  MtrrSetting;

  MtrrGetAllMtrrs (&MtrrSetting);

  InitiateCpuOnlyReset ();

  DispatchAPAndWait (
    TRUE,
    0,
    EarlyMpInit
    );

  EarlyMpInit (mCpuConfigConextBuffer.BspNumber);

  ProgramVirtualWireMode ();
}

/**
  Programs processor registers according to register tables.

  This function programs processor registers according to register tables.

**/
VOID
SettingPhase (
  VOID
  )
{
  UINT8                     CallbackSignalValue;
  UINTN                     Index;
  UINTN                     ProcessorNumber;
  CPU_REGISTER_TABLE        *RegisterTable;
  BOOLEAN                   NeedCpuOnlyReset;

  //
  // Set PcdCpuCallbackSignal to trigger callback function, and reads the value back.
  //
  CallbackSignalValue = SetAndReadCpuCallbackSignal (CPU_PROCESSOR_SETTING_SIGNAL);
  //
  // Checks whether the callback function requests to bypass Setting phase.
  //
  if (CallbackSignalValue == CPU_BYPASS_SIGNAL) {
    return;
  }

  //
  // Check if CPU-only reset is needed
  //
  NeedCpuOnlyReset = FALSE;
  for (Index = 0; Index < mCpuConfigConextBuffer.NumberOfProcessors; Index++) {
    RegisterTable = &mCpuConfigConextBuffer.RegisterTable[Index];
    if (RegisterTable->NumberBeforeReset > 0) {
      NeedCpuOnlyReset = TRUE;
      break;
    }
  }

  //
  // if CPU-only reset is needed, then program corresponding registers, and
  // trigger CPU-only reset.
  //
  if (NeedCpuOnlyReset) {
    mSetBeforeCpuOnlyReset = TRUE;
    DispatchAPAndWait (
      TRUE,
      0,
      SetProcessorRegister
      );

    SetProcessorRegister (mCpuConfigConextBuffer.BspNumber);

    CpuOnlyResetAndRestore ();
  }

  //
  // Program processors' registers in sequential mode.
  //
  mSetBeforeCpuOnlyReset = FALSE;
  for (Index = 0; Index < mCpuConfigConextBuffer.NumberOfProcessors; Index++) {
    
    ProcessorNumber = mCpuConfigConextBuffer.SettingSequence[Index];

    if (ProcessorNumber == mCpuConfigConextBuffer.BspNumber) {
      SetProcessorRegister (ProcessorNumber);
    } else {

      DispatchAPAndWait (
        FALSE,
        GET_CPU_MISC_DATA (ProcessorNumber, ApicID),
        SetProcessorRegister
        );
    }

    RegisterTable = &mCpuConfigConextBuffer.RegisterTable[ProcessorNumber];
    RegisterTable->InitialApicId = GetInitialLocalApicId (ProcessorNumber);
  }
  //
  // Set PcdCpuCallbackSignal to trigger callback function
  //
  PcdSet8 (PcdCpuCallbackSignal, CPU_PROCESSOR_SETTING_END_SIGNAL);

  //
  // From now on, SetProcessorRegister() will be called only by SimpleApProcWrapper()
  // and ApProcWrapper to restore register settings after INIT signal, so switch
  // this flag from FALSE to TRUE.
  //
  mRestoreSettingAfterInit = TRUE;
}
