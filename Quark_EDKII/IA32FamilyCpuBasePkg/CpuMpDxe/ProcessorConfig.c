/** @file
  Code for processor configuration.

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

  Module Name:  ProcessorConfig.c

**/

#include "MpService.h"
#include "Cpu.h"
#include "MpApic.h"

MP_SYSTEM_DATA                      mMPSystemData;
CPU_CONFIG_CONTEXT_BUFFER           mCpuConfigConextBuffer;
EFI_PHYSICAL_ADDRESS                mStartupVector;
UINT8                               mPlatformType;
ACPI_CPU_DATA                       *mAcpiCpuData;
EFI_HANDLE                          mHandle = NULL;
MTRR_SETTINGS                       *mMtrrSettings;
EFI_EVENT                           mSmmConfigurationNotificationEvent;
EFI_HANDLE                          mImageHandle;
EFI_TIMER_ARCH_PROTOCOL             *mTimer;
UINTN                               mLocalApicTimerDivisor;
UINT32                              mLocalApicTimerInitialCount;

/**
  Prepares memory region for processor configuration.
  
  This function prepares memory region for processor configuration.

**/
VOID
PrepareMemoryForConfiguration (
  VOID
  )
{
  UINTN                NumberOfProcessors;
  UINTN                Index;
  MONITOR_MWAIT_DATA   *MonitorData;

  //
  // Initialize Spin Locks for system
  //
  InitializeSpinLock (&mMPSystemData.APSerializeLock);
  for (Index = 0; Index < PcdGet32(PcdCpuMaxLogicalProcessorNumber); Index++) {
    InitializeSpinLock (&mMPSystemData.CpuData[Index].CpuDataLock);
  }

  //
  // Claim memory for AP stack.
  //
  mExchangeInfo->StackStart = AllocateAcpiNvsMemoryBelow4G (PcdGet32(PcdCpuMaxLogicalProcessorNumber) * PcdGet32 (PcdCpuApStackSize));
  mExchangeInfo->StackSize  = PcdGet32 (PcdCpuApStackSize);

  //
  // Initialize the Monitor Data structure in APs' stack
  //
  for (Index = 0; Index < PcdGet32(PcdCpuMaxLogicalProcessorNumber); Index++) {
    MonitorData = GetMonitorDataAddress (Index);
    MonitorData->ApLoopMode = ApInHltLoop;
  }

  //
  // Initialize data for CPU configuration context buffer
  //
  NumberOfProcessors = mCpuConfigConextBuffer.NumberOfProcessors;
  mCpuConfigConextBuffer.CollectedDataBuffer  = AllocateZeroPool (sizeof (CPU_COLLECTED_DATA) * NumberOfProcessors);
  mCpuConfigConextBuffer.FeatureLinkListEntry = AllocateZeroPool (sizeof (LIST_ENTRY) * NumberOfProcessors);

  //
  // Initialize Processor Feature List for all logical processors.
  //
  for (Index = 0; Index < NumberOfProcessors; Index++) {
    InitializeListHead (&mCpuConfigConextBuffer.FeatureLinkListEntry[Index]);
  }

  mCpuConfigConextBuffer.RegisterTable   = AllocateAcpiNvsMemoryBelow4G (
                                            (sizeof (CPU_REGISTER_TABLE) + sizeof (UINTN)) * NumberOfProcessors
                                            );
  mCpuConfigConextBuffer.PreSmmInitRegisterTable = AllocateAcpiNvsMemoryBelow4G (
                                                     (sizeof (CPU_REGISTER_TABLE) + sizeof (UINTN)) * NumberOfProcessors
                                                     );

  mCpuConfigConextBuffer.SettingSequence = (UINTN *) (((UINTN) mCpuConfigConextBuffer.RegisterTable) + (sizeof (CPU_REGISTER_TABLE) * NumberOfProcessors));
  for (Index = 0; Index < NumberOfProcessors; Index++) {
    mCpuConfigConextBuffer.SettingSequence[Index] = Index;
  }

  //
  // Set the value for PcdCpuConfigContextBuffer.
  //
  mCpuConfigLibConfigConextBuffer = &mCpuConfigConextBuffer;
  PcdSet64 (PcdCpuConfigContextBuffer, (UINT64) (UINTN) mCpuConfigLibConfigConextBuffer);

  //
  // Read the platform type from PCD
  //
  mPlatformType = PcdGet8 (PcdPlatformType);
}

/**
  Event notification that is fired every time a gEfiSmmConfigurationProtocol installs.

  This function configures all logical processors with three-phase architecture.

  @param  Event                 The Event that is being processed, not used.
  @param  Context               Event Context, not used.

**/
VOID
EFIAPI
SmmConfigurationEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS    Status;
  VOID  *Registration;
  EFI_SMM_CONFIGURATION_PROTOCOL  *SmmConfiguration;

  //
  // Make sure this notification is for this handler
  //
  Status = gBS->LocateProtocol (&gEfiSmmConfigurationProtocolGuid, NULL, (VOID **)&SmmConfiguration);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Wakeup APs. Collect data of all processors. BSP polls to
  // wait for APs' completion.
  //
  DataCollectionPhase ();
  //
  // With collected data, BSP analyzes processors'configuration
  // according to user's policy.
  //
  AnalysisPhase ();

  //
  // Wakeup APs. Program registers of all processors, according to
  // the result of Analysis phase. BSP polls to wait for AP's completion.
  //
  SettingPhase ();

  //
  // Select least-feature procesosr as BSP
  //
  if (FeaturePcdGet (PcdCpuSelectLfpAsBspFlag)) {
    SelectLfpAsBsp ();
  }

  //
  // Add SMBIOS Processor Type and Cache Type tables for the CPU.
  //
  AddCpuSmbiosTables ();

  //
  // Save CPU S3 data
  //
  SaveCpuS3Data (mImageHandle);

  Status = gBS->SetTimer (
                  mMPSystemData.CheckAPsEvent,
                  TimerPeriodic,
                  10000 * MICROSECOND
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Setup notification on Legacy BIOS Protocol to reallocate AP wakeup
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiLegacyBiosProtocolGuid,
    TPL_CALLBACK,
    ReAllocateMemoryForAP,
    NULL,
    &Registration
    );
}

/**
  Early MP Initialization.
  
  This function does early MP initialization, including MTRR sync and first time microcode load.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
EarlyMpInit (
  IN UINTN  ProcessorNumber
  )
{
  MtrrSetAllMtrrs (mMtrrSettings);

  CollectBasicProcessorData (ProcessorNumber);

}

/**
  First phase MP initialization before SMM initialization.
  
  @retval EFI_SUCCESS      First phase MP initialization was done successfully.
  @retval EFI_UNSUPPORTED  There is legacy APIC ID conflict and can't be rsolved in xAPIC mode.

**/
EFI_STATUS
ProcessorConfiguration (
  VOID
  )
{
  EFI_STATUS    Status;

  //
  // Wakeup APs for the first time, BSP stalls for arbitrary
  // time for APs' completion. BSP then collects the number
  // and BIST information of APs.
  //
  WakeupAPAndCollectBist ();
  //
  // Sort APIC ID of all processors in asending order. Processor number
  // is assigned in this order to ease MP debug. SMBIOS logic also depends on it.
  //
  SortApicId ();

  //
  // Prepare data in memory for processor configuration
  //
  PrepareMemoryForConfiguration ();

  //
  // Early MP initialization
  //
  mMtrrSettings = (MTRR_SETTINGS *)(UINTN)PcdGet64 (PcdCpuMtrrTableAddress);
  MtrrGetAllMtrrs (mMtrrSettings);

  DispatchAPAndWait (
    TRUE,
    0,
    EarlyMpInit
    );

  EarlyMpInit (mCpuConfigConextBuffer.BspNumber);

  DEBUG_CODE (
    //
    // Verify that all processors have same APIC ID topology. New APIC IDs
    // were constructed based on this assumption.
    //
    UINTN Index;
    UINT8 PackageIdBitOffset;

    PackageIdBitOffset = mCpuConfigConextBuffer.CollectedDataBuffer[0].PackageIdBitOffset;
    for (Index = 1; Index < mCpuConfigConextBuffer.NumberOfProcessors; Index++) {
      if (PackageIdBitOffset != mCpuConfigConextBuffer.CollectedDataBuffer[Index].PackageIdBitOffset) {
        ASSERT (FALSE);
      }
    }
  );

  //
  // Check if there is legacy APIC ID conflict among all processors.
  //
  Status = CheckApicId ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Assign Package BSP for package scope programming later.
  //
  AssignPackageBsp ();

  //
  // Produce pre-SMM-init register table.
  //
  ProducePreSmmInitRegisterTable ();

  //
  // Early MP initialization step 2
  //
  DispatchAPAndWait (
    TRUE,
    0,
    SetPreSmmInitProcessorRegister
    );

  SetPreSmmInitProcessorRegister (mCpuConfigConextBuffer.BspNumber);

  //
  // Locate Timer Arch Protocol
  //
  Status = gBS->LocateProtocol (&gEfiTimerArchProtocolGuid, NULL, (VOID **) &mTimer);
  ASSERT_EFI_ERROR (Status);

  //
  // Install MP Services Protocol
  //
  InstallMpServicesProtocol ();

  return EFI_SUCCESS;
}

/**
  Installs MP Services Protocol and register related timer event.
  
  This function installs MP Services Protocol and register related timer event.

**/
VOID
InstallMpServicesProtocol (
  VOID
  )
{
  EFI_STATUS    Status;

  //
  // Create timer event to check AP state for non-blocking execution.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  CheckAPsStatus,
                  NULL,
                  &mMPSystemData.CheckAPsEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Now install the MP services protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gEfiMpServiceProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMpService
                  );
  ASSERT_EFI_ERROR (Status);
}

/**
  Callback function for idle events.
 
  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
IdleLoopEventCallback (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  CpuSleep ();
}

/**
  Entrypoint of CPU MP DXE module.
  
  This function is the entrypoint of CPU MP DXE module.
  It initializes Multi-processor configuration and installs MP Services Protocol.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The entrypoint always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MultiProcessorInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;
  EFI_EVENT   IdleLoopEvent;
  EFI_EVENT   ExitBootServiceEvent;
  EFI_EVENT   LegacyToBootEvent;

  mImageHandle = ImageHandle;
  //
  // Configure processors with three-phase architecture
  //
  Status = ProcessorConfiguration ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install notification callback on SMM Configuration Protocol
  //
  mSmmConfigurationNotificationEvent = EfiCreateProtocolNotifyEvent (
                                         &gEfiSmmConfigurationProtocolGuid,
                                         TPL_CALLBACK,
                                         SmmConfigurationEventNotify,
                                         NULL,
                                         &Registration
                                         );

  //
  // Create EXIT_BOOT_SERIVES Event to set AP to suitable status
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ChangeApLoopModeCallBack,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &ExitBootServiceEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //  
  // Create an event to be signalled when Legacy Boot occurs to set AP to suitable status  
  //
  Status = EfiCreateEventLegacyBootEx(
             TPL_NOTIFY, 
             ChangeApLoopModeCallBack, 
             NULL, 
             &LegacyToBootEvent
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Setup a callback for idle events
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IdleLoopEventCallback,
                  NULL,
                  &gIdleLoopEventGuid,
                  &IdleLoopEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Wakes up APs for the first time to count their number and collect BIST data.

  This function wakes up APs for the first time to count their number and collect BIST data.

**/
VOID
WakeupAPAndCollectBist (
  VOID
  )
{
  //
  // Save BSP's Local APIC Timer setting
  //
  GetApicTimerState (&mLocalApicTimerDivisor, NULL, NULL);
  mLocalApicTimerInitialCount = GetApicTimerInitCount ();

  //
  // Prepare code and data for APs' startup vector
  //
  PrepareAPStartupVector ();

  mCpuConfigConextBuffer.NumberOfProcessors = 1;
  mCpuConfigConextBuffer.BspNumber = 0;
  //
  // Item 0 of BistBuffer is for BSP.
  //
  mExchangeInfo->BistBuffer[0].ApicId = GetInitialApicId ();
  
  SendInitSipiSipiIpis (
    TRUE,
    0,
    NULL
    );

  //
  // Wait for task to complete and then exit.
  //
  MicroSecondDelay (PcdGet32 (PcdCpuApInitTimeOutInMicroSeconds));
  mExchangeInfo->InitFlag = 0;
}


/**
  Prepare ACPI NVS memory below 4G memory for use of S3 resume.
  
  This function allocates ACPI NVS memory below 4G memory for use of S3 resume,
  and saves data into the memory region.

  @param  Context   The Context save the info.

**/
VOID
SaveCpuS3Data (
  VOID    *Context
  )
{
  MP_CPU_SAVED_DATA       *MpCpuSavedData;

  //
  // Allocate ACPI NVS memory below 4G memory for use of S3 resume.
  //
  MpCpuSavedData = AllocateAcpiNvsMemoryBelow4G (sizeof (MP_CPU_SAVED_DATA));

  //
  // Set the value for CPU data
  //
  mAcpiCpuData                 = &(MpCpuSavedData->AcpiCpuData);
  mAcpiCpuData->GdtrProfile    = (EFI_PHYSICAL_ADDRESS) (UINTN) &(MpCpuSavedData->GdtrProfile);
  mAcpiCpuData->IdtrProfile    = (EFI_PHYSICAL_ADDRESS) (UINTN) &(MpCpuSavedData->IdtrProfile);
  mAcpiCpuData->StackAddress   = (EFI_PHYSICAL_ADDRESS) (UINTN) mExchangeInfo->StackStart;
  mAcpiCpuData->StackSize      = PcdGet32 (PcdCpuApStackSize);
  mAcpiCpuData->MtrrTable      = (EFI_PHYSICAL_ADDRESS) (UINTN) PcdGet64 (PcdCpuMtrrTableAddress);
  mAcpiCpuData->RegisterTable  = (EFI_PHYSICAL_ADDRESS) (UINTN) mCpuConfigConextBuffer.RegisterTable;

  mAcpiCpuData->PreSmmInitRegisterTable   = (EFI_PHYSICAL_ADDRESS) (UINTN) mCpuConfigConextBuffer.PreSmmInitRegisterTable;
  mAcpiCpuData->ApMachineCheckHandlerBase = mApMachineCheckHandlerBase;
  mAcpiCpuData->ApMachineCheckHandlerSize = mApMachineCheckHandlerSize;

  //
  // Check user's policy for HT enable.
  //
  mAcpiCpuData->APState        = FALSE;
  if ((PcdGet32 (PcdCpuProcessorFeatureUserConfiguration) & PCD_CPU_HT_BIT) != 0) {
    mAcpiCpuData->APState = TRUE;
  }

  //
  // Copy GDTR and IDTR profiles
  //
  CopyMem ((VOID *) (UINTN) mAcpiCpuData->GdtrProfile, (VOID *) (UINTN) &mExchangeInfo->GdtrProfile, sizeof (IA32_DESCRIPTOR));
  CopyMem ((VOID *) (UINTN) mAcpiCpuData->IdtrProfile, (VOID *) (UINTN) &mExchangeInfo->IdtrProfile, sizeof (IA32_DESCRIPTOR));

  mAcpiCpuData->NumberOfCpus  = (UINT32) mCpuConfigConextBuffer.NumberOfProcessors;
  
  //
  // Set the base address of CPU S3 data to PcdCpuS3DataAddress
  //
  PcdSet64 (PcdCpuS3DataAddress, (UINT64)(UINTN)mAcpiCpuData); 
}
