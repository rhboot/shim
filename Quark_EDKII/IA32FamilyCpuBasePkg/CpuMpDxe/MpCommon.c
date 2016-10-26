/** @file

  Common functions for CPU DXE module.

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

  Module Name:  MpCommon.c

**/

#include "MpService.h"
#include "Cpu.h"
#include "ArchSpecificDef.h"

AP_PROCEDURE          mApFunction = NULL;
AP_PROCEDURE          SimpleAPProcedure;
UINT32                NumberToFinish;
UINTN                 mApCount = 0;
UINTN                 mStartupVectorSize;
EFI_PHYSICAL_ADDRESS  mApMachineCheckHandlerBase;
UINT32                mApMachineCheckHandlerSize;

/**
  Allocates startup vector for APs.

  This function allocates Startup vector for APs.

  @param  Size  The size of startup vector.

**/
VOID
AllocateStartupVector (
  UINTN   Size
  )
{
  EFI_STATUS                            Status;
  EFI_GENERIC_MEMORY_TEST_PROTOCOL      *GenMemoryTest;
  EFI_PHYSICAL_ADDRESS                  StartAddress;

  mStartupVectorSize = Size;

  //
  // In order to allocate startup vector for APs under 1M, try to locate
  // Generic Memory Test Protocol to test memory before allocation.
  //
  Status = gBS->LocateProtocol (
                  &gEfiGenericMemTestProtocolGuid,
                  NULL,
                  (VOID **) &GenMemoryTest
                  );
  if (EFI_ERROR (Status)) {
    GenMemoryTest = NULL;
  }

  //
  // Allocate wakeup buffer below 640K. Don't touch legacy region.
  // Leave some room immediately below 640K in for CSM module.
  // PcdEbdaReservedMemorySize has been required to be a multiple of 4KB.
  //
  StartAddress = 0xA0000 - PcdGet32 (PcdEbdaReservedMemorySize) - (EFI_SIZE_TO_PAGES (Size) * EFI_PAGE_SIZE);
  for (mStartupVector = StartAddress; mStartupVector >= 0x2000; mStartupVector -= (EFI_SIZE_TO_PAGES (Size) * EFI_PAGE_SIZE)) {
    //
    // If Generic Memory Test Protocol has been located successfully, call CompatibleRangeTest() to test
    // the range we are going to allocate. If the protocol has not been located, then the memory test would
    // be skipped.
    //
    if (GenMemoryTest != NULL) {
      Status = GenMemoryTest->CompatibleRangeTest (
                                GenMemoryTest,
                                mStartupVector,
                                EFI_SIZE_TO_PAGES (Size) * EFI_PAGE_SIZE
                                );
      if (EFI_ERROR (Status)) {
        continue;
      }
    }

    //
    // If finally no CSM in the platform, this wakeup buffer is to be used in S3 boot path.
    //
    Status = gBS->AllocatePages (
                    AllocateAddress,
                    EfiReservedMemoryType,
                    EFI_SIZE_TO_PAGES (Size),
                    &mStartupVector
                    );

    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  ASSERT_EFI_ERROR (Status);
}

 /**
  Count the number of APs that have been switched
  to E0000 or F0000 segments by ReAllocateMemoryForAP().

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
LegacyRegionAPCount (
  IN UINTN  ProcessorNumber
  )
{
  AcquireSpinLock (&mMPSystemData.APSerializeLock);

  mApCount++;

  ReleaseSpinLock (&mMPSystemData.APSerializeLock);
}


/**
  This AP function will place AP to the suitable state.
  
  If C-State is enable, try to place AP to the Mwait-Loop with deepest C-State, 
  otherwize place AP to  Hlt-Loop state.

  @param  Buffer  Pointer to MP Services.
**/
VOID
EFIAPI
ChangeApLoopMode (
  IN  VOID     *Buffer
  ) 
{
  UINTN                      ProcessorNumber;
  MONITOR_MWAIT_DATA         *MonitorAddr;

  WhoAmI (&mMpService, &ProcessorNumber);

  //
  // Set the ReadyToBootFlag for AP.
  //
  MonitorAddr = GetMonitorDataAddress (ProcessorNumber);
  MonitorAddr->ReadyToBootFlag = TRUE;
}

/**
  Protocol notification that will wake up and place AP to the suitable state
  before booting to OS.

  @param  Event                 The triggered event.
  @param  Context               Context for this event.
**/
VOID
EFIAPI
ChangeApLoopModeCallBack (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                 Status;

  while (ApRunning()) {
    CpuPause ();
  }
 
  Status = mMpService.StartupAllAPs (
                        &mMpService,
                        ChangeApLoopMode,
                        FALSE,
                        NULL,
                        0,
                        NULL,
                        NULL
                        );
  ASSERT_EFI_ERROR (Status);
}

/**
  Protocol notification that is fired when LegacyBios protocol is installed.

  Re-allocate a wakeup buffer from E/F segment because the previous wakeup buffer
  under 640K won't be preserved by the legacy OS.

  @param  Event                 The triggered event.
  @param  Context               Context for this event.
**/
VOID
EFIAPI
ReAllocateMemoryForAP (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_LEGACY_BIOS_PROTOCOL   *LegacyBios;
  VOID                       *LegacyRegion;
  EFI_STATUS                 Status;
  MP_CPU_EXCHANGE_INFO       *ExchangeInfo;
  MP_ASSEMBLY_ADDRESS_MAP    AddressMap;

  Status = gBS->LocateProtocol (&gEfiLegacyBiosProtocolGuid, NULL, (VOID **)&LegacyBios);
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Wait for all APs to finish their task before switching wakeup buffer
  //
  while (ApRunning()) {
    CpuPause ();
  }

  //
  // Allocate 4K aligned bytes from either 0xE0000 or 0xF0000.
  // Note some CSM16 does not satisfy alignment request, so allocate a buffer of 2*4K
  // and adjust the base address myself.
  //
  Status = LegacyBios->GetLegacyRegion (
                         LegacyBios,
                         0x2000,
                         0,
                         0x1000,
                         &LegacyRegion
                         );
  ASSERT_EFI_ERROR (Status);

  if ((((UINTN)LegacyRegion) & 0xfff) != 0) {
    LegacyRegion = (VOID *)(UINTN)((((UINTN)LegacyRegion) + 0xfff) & ~((UINTN)0xfff));
  }

  //
  // Get the address map of startup code for AP,
  // including code size, and offset of long jump instructions to redirect.
  //
  AsmGetAddressMap (&AddressMap);

  //
  // Update exchange info and copy AP startup code to new buffer in legacy region.
  //
  ExchangeInfo              = (MP_CPU_EXCHANGE_INFO *) (UINTN) (mStartupVector + AddressMap.Size);
  ExchangeInfo->BufferStart = (UINT32) (UINTN) LegacyRegion;
  RedirectFarJump (&AddressMap, (EFI_PHYSICAL_ADDRESS) (UINTN) LegacyRegion);

  //
  // Switch all APs to use new wakeup buffer
  //
  Status = LegacyBios->CopyLegacyRegion (
                         LegacyBios,
                         mStartupVectorSize,
                         LegacyRegion,
                         (VOID *)(UINTN)mStartupVector
                         );

  SendInitSipiSipiIpis (
    TRUE,
    0,
    LegacyRegionAPCount
    );

  //
  // Wait until all APs finish
  //
  while (mApCount < mAcpiCpuData->NumberOfCpus - 1) {
    CpuPause ();
  }

  //
  // Free original wakeup buffer
  //
  FreePages ((VOID *)(UINTN)mStartupVector, EFI_SIZE_TO_PAGES (mStartupVectorSize));

  mStartupVector = (EFI_PHYSICAL_ADDRESS)(UINTN)LegacyRegion;
  mAcpiCpuData->StartupVector = mStartupVector;
  mExchangeInfo = (MP_CPU_EXCHANGE_INFO *) (UINTN) (mStartupVector + AddressMap.Size);
}

/**
  Allocate aligned ACPI NVS memory below 4G.

  This function allocates aligned ACPI NVS memory below 4G.

  @param  Size       Size of memory region to allocate
  @param  Alignment  Alignment in bytes

  @return Base address of the allocated region

**/
VOID*
AllocateAlignedAcpiNvsMemory (
  IN  UINTN         Size,
  IN  UINTN         Alignment
  )
{
  UINTN       PointerValue;
  VOID        *Pointer;

  Pointer = AllocateAcpiNvsMemoryBelow4G (Size + Alignment - 1);

  PointerValue  = (UINTN) Pointer;
  PointerValue  = (PointerValue + Alignment - 1) / Alignment * Alignment;

  Pointer      = (VOID *) PointerValue;

  return Pointer;
}

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.

  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  Sends INIT-SIPI-SIPI to AP.

  This function sends INIT-SIPI-SIPI to AP, and assign procedure specified by ApFunction.

  @param  Broadcast   If TRUE, broadcase IPI to all APs; otherwise, send to specified AP.
  @param  ApicID      The Local APIC ID of the specified AP. If Broadcast is TRUE, it is ignored.
  @param  ApFunction  The procedure for AP to work on.

**/
VOID
SendInitSipiSipiIpis (
  IN BOOLEAN            Broadcast,
  IN UINT32             ApicID,
  IN AP_PROCEDURE       ApFunction
  )
{
  mApFunction = ApFunction;

  if (Broadcast) {
    SendInitSipiSipiAllExcludingSelf ((UINT32) mStartupVector);
  } else {
    SendInitSipiSipi (ApicID, (UINT32) mStartupVector);
  }
}

/**
  A simple wrapper function dispatched to AP.

  This function is a simple wrapper function dispatched to AP. It invokes task for AP, and count down
  the number.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
SimpleApProcWrapper (
  IN UINTN  ProcessorNumber
  )
{
  //
  // Program virtual wire mode and Local APIC timer for AP, since it will be lost after AP wake up
  //
  ProgramVirtualWireMode ();
  DisableLvtInterrupts ();
  InitializeApicTimer (mLocalApicTimerDivisor, mLocalApicTimerInitialCount, FALSE, 0);

  // Initialize Debug Agent to support source level debug on AP code.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_AP, NULL, NULL);

  //
  // If it is necessary to restore register setting after INIT signal,
  // call SetProcessorRegister().
  //
  if (mRestoreSettingAfterInit) {
    SetProcessorRegister (ProcessorNumber);
  }

  //
  // Invoke task for AP.
  //
  SimpleAPProcedure (ProcessorNumber);

  //
  // Count down the number with lock mechanism.
  //
  InterlockedDecrement (&NumberToFinish);
}

/**
  Dispatches task to AP.

  This function dispatches task to AP. The BSP waits until specified APs have finished.

  @param  Broadcast   If TRUE, send task to all APs; otherwise, send to specified AP.
  @param  ApicID      The Local APIC ID of the specified AP. If Broadcast is TRUE, it is ignored.
  @param  Procedure   The procedure for AP to work on.

**/
VOID
DispatchAPAndWait (
  IN BOOLEAN             Broadcast,
  IN UINT32              ApicID,
  IN AP_PROCEDURE        Procedure
  )
{
  //
  // Prepares the task for AP. It will invoked by SimpleApProcWrapper.
  //
  SimpleAPProcedure = Procedure;

  //
  // Checks whether the function is for broadcast.
  //
  if (Broadcast) {
    //
    // If in broadcast mode, the number to finish is the number of all APs
    //
    NumberToFinish = (UINT32) mCpuConfigConextBuffer.NumberOfProcessors - 1;
  } else {
    //
    // If in non-broadcast mode, the number to finish is 1
    //
    NumberToFinish = 1;
  }

  //
  // Wake up specified AP(s), and make them work on SimpleApProcWrapper, which
  // will in turn invoke Procedure.
  //
  SendInitSipiSipiIpis (
    Broadcast,
    ApicID,
    SimpleApProcWrapper
    );

  //
  // BSP waits until specified AP(s) have finished.
  //
  while (NumberToFinish > 0) {
    CpuPause ();
  }
}

/**
  Creates a copy of GDT and IDT for all APs.

  This function creates a copy of GDT and IDT for all APs.

  @param  Gdtr   Base and limit of GDT for AP
  @param  Idtr   Base and limit of IDT for AP

**/
VOID
PrepareGdtIdtForAP (
  OUT IA32_DESCRIPTOR          *Gdtr,
  OUT IA32_DESCRIPTOR          *Idtr
  )
{
  SEGMENT_DESCRIPTOR        *GdtForAP;
  INTERRUPT_GATE_DESCRIPTOR *IdtForAP;
  IA32_DESCRIPTOR           GdtrForBSP;
  IA32_DESCRIPTOR           IdtrForBSP;
  VOID                      *MachineCheckHandlerBuffer;

  //
  // Get the BSP's data of GDT and IDT
  //
  AsmReadGdtr ((IA32_DESCRIPTOR *) &GdtrForBSP);
  AsmReadIdtr ((IA32_DESCRIPTOR *) &IdtrForBSP);

  //
  // Allocate ACPI NVS memory for GDT, IDT, and machine check handler.
  // Combine allocation for ACPI NVS memory under 4G to save memory.
  //
  GdtForAP = AllocateAlignedAcpiNvsMemory (
               (GdtrForBSP.Limit + 1) + (IdtrForBSP.Limit + 1) + (UINTN) ApMachineCheckHandlerEnd - (UINTN) ApMachineCheckHandler,
               8
               );

  //
  // GDT base is 8-bype aligned, and its size is multiple of 8-bype, so IDT base here is
  // also 8-bype aligned.
  //
  IdtForAP = (INTERRUPT_GATE_DESCRIPTOR *) ((UINTN) GdtForAP + GdtrForBSP.Limit + 1);
  MachineCheckHandlerBuffer = (VOID *) ((UINTN) GdtForAP + (GdtrForBSP.Limit + 1) + (IdtrForBSP.Limit + 1));
  //
  // Make copy for APs' GDT & IDT
  //
  CopyMem (GdtForAP, (VOID *) GdtrForBSP.Base, GdtrForBSP.Limit + 1);
  CopyMem (IdtForAP, (VOID *) IdtrForBSP.Base, IdtrForBSP.Limit + 1);

  //
  // Copy code for AP's machine check handler to ACPI NVS memory, and register in IDT
  //
  CopyMem (
    MachineCheckHandlerBuffer,
    (VOID *) (UINTN) ApMachineCheckHandler,
    (UINTN) ApMachineCheckHandlerEnd - (UINTN) ApMachineCheckHandler
    );
  SetIdtEntry ((UINTN) MachineCheckHandlerBuffer, &IdtForAP[INTERRUPT_HANDLER_MACHINE_CHECK]);

  //
  // Set AP's profile for GDTR and IDTR
  //
  Gdtr->Base  = (UINTN) GdtForAP;
  Gdtr->Limit = GdtrForBSP.Limit;

  Idtr->Base  = (UINTN) IdtForAP;
  Idtr->Limit = IdtrForBSP.Limit;

  //
  // Save the AP's machine check handler information
  //
  mApMachineCheckHandlerBase = (EFI_PHYSICAL_ADDRESS) (UINTN) MachineCheckHandlerBuffer;
  mApMachineCheckHandlerSize = (UINT32) ((UINTN) ApMachineCheckHandlerEnd - (UINTN) ApMachineCheckHandler);
}

/**
  Get mointor data address for specified processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

  @return Pointer to monitor data.
**/
MONITOR_MWAIT_DATA *
EFIAPI
GetMonitorDataAddress (
  IN UINTN  ProcessorNumber
  )
{
  return (MONITOR_MWAIT_DATA *) ((UINT8*) mExchangeInfo->StackStart + (ProcessorNumber + 1) *  mExchangeInfo->StackSize - MONITOR_FILTER_SIZE);
}
