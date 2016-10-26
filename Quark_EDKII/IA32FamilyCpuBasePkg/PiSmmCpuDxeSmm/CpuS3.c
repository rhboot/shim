/** @file

  Code for Processor S3 restoration

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

  Module Name:  CpuS3.c

**/

#include "PiSmmCpuDxeSmm.h"

typedef struct {
  UINTN             Lock;
  VOID              *StackStart;
  UINTN             StackSize;
  VOID              *ApFunction;
  IA32_DESCRIPTOR   GdtrProfile;
  IA32_DESCRIPTOR   IdtrProfile;
  UINT32            BufferStart;
  UINT32            Cr3;
} MP_CPU_EXCHANGE_INFO;

typedef struct {
  UINT8 *RendezvousFunnelAddress;
  UINTN PModeEntryOffset;
  UINTN FlatJumpOffset;
  UINTN Size;
  UINTN LModeEntryOffset;
  UINTN LongJumpOffset;
} MP_ASSEMBLY_ADDRESS_MAP;

/**
  Get starting address and size of the rendezvous entry for APs.
  Information for fixing a jump instruction in the code is also returned.

  @param AddressMap  Output buffer for address map information.
**/
VOID *
EFIAPI
AsmGetAddressMap (
  MP_ASSEMBLY_ADDRESS_MAP                     *AddressMap
  );

#define LEGACY_REGION_SIZE    (2 * 0x1000)
#define LEGACY_REGION_BASE    (0xA0000 - LEGACY_REGION_SIZE)

ACPI_CPU_DATA                mAcpiCpuData;
UINT32                       mNumberToFinish;
MP_CPU_EXCHANGE_INFO         *mExchangeInfo;
BOOLEAN                      mRestoreSmmConfigurationInS3 = FALSE;
VOID                         *mGdtForAp = NULL;
VOID                         *mIdtForAp = NULL;
VOID                         *mMachineCheckHandlerForAp = NULL;

/**
  Synch up the MTRR values for all processors.

  @param MtrrTable  Table holding fixed/variable MTRR values to be loaded.
**/
VOID
EFIAPI
LoadMtrrData (
  EFI_PHYSICAL_ADDRESS       MtrrTable
  )
/*++

Routine Description:

  Synch up the MTRR values for all processors.

Arguments:

Returns:
    None

--*/
{
  MTRR_SETTINGS   *MtrrSettings;

  MtrrSettings = (MTRR_SETTINGS *) (UINTN) MtrrTable;
  MtrrSetAllMtrrs (MtrrSettings);
}

/**
  Programs registers for the calling processor.

  This function programs registers for the calling processor.

  @param  RegisterTable Pointer to register table of the running processor.

**/
VOID
SetProcessorRegister (
  CPU_REGISTER_TABLE        *RegisterTable
  )
{
  CPU_REGISTER_TABLE_ENTRY  *RegisterTableEntry;
  UINTN                     Index;
  UINTN                     Value;

  //
  // Traverse Register Table of this logical processor
  //
  RegisterTableEntry = (CPU_REGISTER_TABLE_ENTRY *) (UINTN) RegisterTable->RegisterTableEntry;
  for (Index = 0; Index < RegisterTable->TableLength; Index++, RegisterTableEntry++) {
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
                          (UINTN) RegisterTableEntry->Value
                          );
        AsmWriteCr0 (Value);
        break;
      case 2:
        Value = AsmReadCr2 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          (UINTN) RegisterTableEntry->Value
                          );
        AsmWriteCr2 (Value);
        break;
      case 3:
        Value = AsmReadCr3 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          (UINTN) RegisterTableEntry->Value
                          );
        AsmWriteCr3 (Value);
        break;
      case 4:
        Value = AsmReadCr4 ();
        Value = (UINTN) BitFieldWrite64 (
                          Value,
                          RegisterTableEntry->ValidBitStart,
                          RegisterTableEntry->ValidBitStart + RegisterTableEntry->ValidBitLength - 1,
                          (UINTN) RegisterTableEntry->Value
                          );
        AsmWriteCr4 (Value);
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
  AP initialization before SMBASE relocation in the S3 boot path.
**/
VOID
EarlyMPRendezvousProcedure (
  VOID
  )
{
  CPU_REGISTER_TABLE         *RegisterTableList;
  UINT32                     InitApicId;
  UINTN                      Index;

  LoadMtrrData (mAcpiCpuData.MtrrTable);

  //
  // Find processor number for this CPU.
  //
  RegisterTableList = (CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.PreSmmInitRegisterTable;
  InitApicId = GetInitialApicId ();
  for (Index = 0; Index < mAcpiCpuData.NumberOfCpus; Index++) {
    if (RegisterTableList[Index].InitialApicId == InitApicId) {
      SetProcessorRegister (&RegisterTableList[Index]);
      break;
    }
  } 

  //
  // Count down the number with lock mechanism.
  //
  InterlockedDecrement (&mNumberToFinish);
}

/**
  AP initialization after SMBASE relocation in the S3 boot path.
**/
VOID
MPRendezvousProcedure (
  VOID
  )
{
  CPU_REGISTER_TABLE         *RegisterTableList;
  UINT32                     InitApicId;
  UINTN                      Index;

  ProgramVirtualWireMode ();
  DisableLvtInterrupts ();

  RegisterTableList = (CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.RegisterTable;
  InitApicId = GetInitialApicId ();
  for (Index = 0; Index < mAcpiCpuData.NumberOfCpus; Index++) {
    if (RegisterTableList[Index].InitialApicId == InitApicId) {
      SetProcessorRegister (&RegisterTableList[Index]);
      break;
    }
  }

  //
  // Count down the number with lock mechanism.
  //
  InterlockedDecrement (&mNumberToFinish);
} 

/**
  Prepares startup vector for APs.

  This function prepares startup vector for APs.

  @param  WorkingBuffer  The address of the work buffer.
**/
VOID
PrepareAPStartupVector (
  EFI_PHYSICAL_ADDRESS  WorkingBuffer
  )
{
  EFI_PHYSICAL_ADDRESS                        StartupVector;
  MP_ASSEMBLY_ADDRESS_MAP                     AddressMap;

  //
  // Get the address map of startup code for AP,
  // including code size, and offset of long jump instructions to redirect.
  //
  ZeroMem (&AddressMap, sizeof (AddressMap));
  AsmGetAddressMap (&AddressMap);

  StartupVector = WorkingBuffer;
  
  //
  // Copy AP startup code to startup vector, and then redirect the long jump
  // instructions for mode switching.
  //
  CopyMem ((VOID *) (UINTN) StartupVector, AddressMap.RendezvousFunnelAddress, AddressMap.Size);
  *(UINT32 *) (UINTN) (StartupVector + AddressMap.FlatJumpOffset + 3) = (UINT32) (StartupVector + AddressMap.PModeEntryOffset);
  if (AddressMap.LongJumpOffset != 0) {
    *(UINT32 *) (UINTN) (StartupVector + AddressMap.LongJumpOffset + 2) = (UINT32) (StartupVector + AddressMap.LModeEntryOffset);
  }

  //
  // Get the start address of exchange data between BSP and AP.
  //
  mExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN) (StartupVector + AddressMap.Size);
  ZeroMem ((VOID *) mExchangeInfo, sizeof (MP_CPU_EXCHANGE_INFO));

  CopyMem ((VOID *) (UINTN) &mExchangeInfo->GdtrProfile, (VOID *) (UINTN) mAcpiCpuData.GdtrProfile, sizeof (IA32_DESCRIPTOR));
  CopyMem ((VOID *) (UINTN) &mExchangeInfo->IdtrProfile, (VOID *) (UINTN) mAcpiCpuData.IdtrProfile, sizeof (IA32_DESCRIPTOR));
  
  //
  // Copy AP's GDT, IDT and Machine Check handler from SMRAM to ACPI NVS memory
  //
  CopyMem ((VOID *) mExchangeInfo->GdtrProfile.Base, mGdtForAp, mExchangeInfo->GdtrProfile.Limit + 1);
  CopyMem ((VOID *) mExchangeInfo->IdtrProfile.Base, mIdtForAp, mExchangeInfo->IdtrProfile.Limit + 1);
  CopyMem ((VOID *)(UINTN) mAcpiCpuData.ApMachineCheckHandlerBase, mMachineCheckHandlerForAp, mAcpiCpuData.ApMachineCheckHandlerSize);
 
  mExchangeInfo->StackStart  = (VOID *) (UINTN) mAcpiCpuData.StackAddress;
  mExchangeInfo->StackSize   = mAcpiCpuData.StackSize;
  mExchangeInfo->BufferStart = (UINT32) StartupVector;
  mExchangeInfo->Cr3         = (UINT32) (AsmReadCr3 ());
}

/**
  The function is invoked before SMBASE relocation in S3 path to restors CPU status.

  The function is invoked before SMBASE relocation in S3 path. It does first time microcode load 
  and restores MTRRs for both BSP and APs.

**/
VOID
EarlyInitializeCpu (
  VOID
  )
{
  CPU_REGISTER_TABLE         *RegisterTableList;
  UINT32                     InitApicId;
  UINTN                      Index;

  LoadMtrrData (mAcpiCpuData.MtrrTable);

  //
  // Find processor number for this CPU.
  //
  RegisterTableList = (CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.PreSmmInitRegisterTable;
  InitApicId = GetInitialApicId ();
  for (Index = 0; Index < mAcpiCpuData.NumberOfCpus; Index++) {
    if (RegisterTableList[Index].InitialApicId == InitApicId) {
      SetProcessorRegister (&RegisterTableList[Index]);
      break;
    }
  } 

  ProgramVirtualWireMode ();

  PrepareAPStartupVector (mAcpiCpuData.StartupVector);

  mNumberToFinish = mAcpiCpuData.NumberOfCpus - 1;
  mExchangeInfo->ApFunction  = (VOID *) (UINTN) EarlyMPRendezvousProcedure;

  //
  // Send INIT IPI - SIPI to all APs
  //
  SendInitSipiSipiAllExcludingSelf ((UINT32)mAcpiCpuData.StartupVector);

  while (mNumberToFinish > 0) {
    CpuPause ();
  }
}

/**
  The function is invoked after SMBASE relocation in S3 path to restors CPU status.

  The function is invoked after SMBASE relocation in S3 path. It restores configuration according to 
  data saved by normal boot path for both BSP and APs.

**/
VOID
InitializeCpu (
  VOID
  )
{
  CPU_REGISTER_TABLE         *RegisterTableList;
  UINT32                     InitApicId;
  UINTN                      Index;

  RegisterTableList = (CPU_REGISTER_TABLE *) (UINTN) mAcpiCpuData.RegisterTable;
  InitApicId = GetInitialApicId ();
  for (Index = 0; Index < mAcpiCpuData.NumberOfCpus; Index++) {
    if (RegisterTableList[Index].InitialApicId == InitApicId) {
      SetProcessorRegister (&RegisterTableList[Index]);
      break;
    }
  } 

  mNumberToFinish = mAcpiCpuData.NumberOfCpus - 1;
  mExchangeInfo->ApFunction  = (VOID *) (UINTN) MPRendezvousProcedure;

  //
  // Send INIT IPI - SIPI to all APs
  //
  SendInitSipiSipiAllExcludingSelf ((UINT32)mAcpiCpuData.StartupVector);

  while (mNumberToFinish > 0) {
    CpuPause ();
  }
}

/**
  Restore SMM Configuration.
**/
VOID
RestoreSmmConfigurationInS3 (
  VOID
  )
{
  if (mRestoreSmmConfigurationInS3) {
    //
    // Configure SMM Code Access Check feature if available.
    //
    ConfigSmmCodeAccessCheck ();

    mRestoreSmmConfigurationInS3 = FALSE;
  }
}

