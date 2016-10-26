/** @file

  Implementation of MP Services Protocol

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

  Module Name:  MpService.c

**/

#include "MpService.h"
#include "Cpu.h"

EFI_MP_SERVICES_PROTOCOL     mMpService = {
  GetNumberOfProcessors,
  GetProcessorInfo,
  StartupAllAPs,
  StartupThisAP,
  SwitchBSP,
  EnableDisableAP,
  WhoAmI
};

BOOLEAN            mStopCheckAPsStatus  = FALSE;
BOOLEAN            mBspSwitched         = FALSE;
UINTN              mNewProcessorNumber  = 0;
MONITOR_MWAIT_DATA *mMonitorDataAddress = NULL;

/**
  Implementation of GetNumberOfProcessors() service of MP Services Protocol.

  This service retrieves the number of logical processor in the platform
  and the number of those logical processors that are enabled on this boot.
  This service may only be called from the BSP.

  @param  This                       A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  NumberOfProcessors         Pointer to the total number of logical processors in the system,
                                     including the BSP and disabled APs.
  @param  NumberOfEnabledProcessors  Pointer to the number of enabled logical processors that exist
                                     in system, including the BSP.

  @retval EFI_SUCCESS	               Number of logical processors and enabled logical processors retrieved..
  @retval EFI_DEVICE_ERROR           Caller processor is AP.
  @retval EFI_INVALID_PARAMETER      NumberOfProcessors is NULL
  @retval EFI_INVALID_PARAMETER      NumberOfEnabledProcessors is NULL

**/
EFI_STATUS
EFIAPI
GetNumberOfProcessors (
  IN  EFI_MP_SERVICES_PROTOCOL     *This,
  OUT UINTN                        *NumberOfProcessors,
  OUT UINTN                        *NumberOfEnabledProcessors
  )
{
  UINTN           Index;
  CPU_DATA_BLOCK  *CpuData;
  UINT32          ThreadNumber;
  UINTN           CallerNumber;

  //
  // Check whether caller processor is BSP
  //
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != mCpuConfigConextBuffer.BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check parameter NumberOfProcessors
  //
  if (NumberOfProcessors == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check parameter NumberOfEnabledProcessors
  //
  if (NumberOfEnabledProcessors == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *NumberOfProcessors = mCpuConfigConextBuffer.NumberOfProcessors;

  //
  // Count the number of enabled processors
  //
  *NumberOfEnabledProcessors = 0;
  for (Index = 0; Index < mCpuConfigConextBuffer.NumberOfProcessors; Index++) {

    GetProcessorLocation (
      Index,
      NULL,
      NULL,
      &ThreadNumber
      );

    CpuData = &mMPSystemData.CpuData[Index];
    if ((PcdGet32 (PcdCpuProcessorFeatureUserConfiguration) & PCD_CPU_HT_BIT) != 0) {
      if (CpuData->State != CpuStateDisabled) {
        (*NumberOfEnabledProcessors)++;
      }
    } else {
      if (CpuData->State != CpuStateDisabled && ThreadNumber == 0) {
        (*NumberOfEnabledProcessors)++;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Implementation of GetNumberOfProcessors() service of MP Services Protocol.

  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       The handle number of processor.
  @param  ProcessorInfoBuffer   A pointer to the buffer where information for the requested processor is deposited.

  @retval EFI_SUCCESS           Processor information successfully returned.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_INVALID_PARAMETER ProcessorInfoBuffer is NULL
  @retval EFI_NOT_FOUND         Processor with the handle specified by ProcessorNumber does not exist.

**/
EFI_STATUS
EFIAPI
GetProcessorInfo (
  IN       EFI_MP_SERVICES_PROTOCOL     *This,
  IN       UINTN                        ProcessorNumber,
  OUT      EFI_PROCESSOR_INFORMATION    *ProcessorInfoBuffer
  )
{
  CPU_DATA_BLOCK            *CpuData;
  UINTN                     CallerNumber;

  //
  // Check whether caller processor is BSP
  //
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != mCpuConfigConextBuffer.BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check parameter ProcessorInfoBuffer
  //
  if (ProcessorInfoBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether processor with the handle specified by ProcessorNumber exists
  //
  if (ProcessorNumber >= mCpuConfigConextBuffer.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  //
  // Get Local APIC ID of specified processor
  //
  ProcessorInfoBuffer->ProcessorId = GET_CPU_MISC_DATA (ProcessorNumber, ApicID);

  //
  // Get physical location of specified processor
  //
  GetProcessorLocation (
    ProcessorNumber,
    &(ProcessorInfoBuffer->Location.Package),
    &(ProcessorInfoBuffer->Location.Core),
    &(ProcessorInfoBuffer->Location.Thread)
    );

  //
  // Get Status Flag of specified processor
  //
  ProcessorInfoBuffer->StatusFlag = 0;

  ProcessorInfoBuffer->StatusFlag |= PROCESSOR_ENABLED_BIT;
  if ((PcdGet32 (PcdCpuProcessorFeatureUserConfiguration) & PCD_CPU_HT_BIT) == 0) {
    if (ProcessorInfoBuffer->Location.Thread > 0) {
      ProcessorInfoBuffer->StatusFlag &= ~PROCESSOR_ENABLED_BIT;
    }
  }

  if (CpuData->State == CpuStateDisabled) {
    ProcessorInfoBuffer->StatusFlag &= ~PROCESSOR_ENABLED_BIT;
  }

  if (ProcessorNumber == mCpuConfigConextBuffer.BspNumber) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_AS_BSP_BIT;
  }

  if (mMPSystemData.CpuHealthy[ProcessorNumber]) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_HEALTH_STATUS_BIT;
  }

  return EFI_SUCCESS;
}

/**
  Implementation of StartupAllAPs() service of MP Services Protocol.

  This service lets the caller get all enabled APs to execute a caller-provided function.
  This service may only be called from the BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  Procedure             A pointer to the function to be run on enabled APs of the system.
  @param  SingleThread          Indicates whether to execute the function simultaneously or one by one..
  @param  WaitEvent             The event created by the caller.
                                If it is NULL, then execute in blocking mode.
                                If it is not NULL, then execute in non-blocking mode.
  @param  TimeoutInMicroSeconds The time limit in microseconds for this AP to finish the function.
                                Zero means infinity.
  @param  ProcedureArgument     Pointer to the optional parameter of the assigned function.
  @param  FailedCpuList         The list of processor numbers that fail to finish the function before
                                TimeoutInMicrosecsond expires.

  @retval EFI_SUCCESS           In blocking mode, all APs have finished before the timeout expired.
  @retval EFI_SUCCESS           In non-blocking mode, function has been dispatched to all enabled APs.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_NOT_STARTED       No enabled AP exists in the system.
  @retval EFI_NOT_READY         Any enabled AP is busy.
  @retval EFI_TIMEOUT           In blocking mode, The timeout expired before all enabled APs have finished.
  @retval EFI_INVALID_PARAMETER Procedure is NULL.

**/
EFI_STATUS
EFIAPI
StartupAllAPs (
  IN  EFI_MP_SERVICES_PROTOCOL   *This,
  IN  EFI_AP_PROCEDURE           Procedure,
  IN  BOOLEAN                    SingleThread,
  IN  EFI_EVENT                  WaitEvent             OPTIONAL,
  IN  UINTN                      TimeoutInMicroSeconds,
  IN  VOID                       *ProcedureArgument    OPTIONAL,
  OUT UINTN                      **FailedCpuList       OPTIONAL
  )
{
  EFI_STATUS      Status;
  UINTN           ProcessorNumber;
  CPU_DATA_BLOCK  *CpuData;
  BOOLEAN         Blocking;

  if (FailedCpuList != NULL) {
    *FailedCpuList = NULL;
  }

  //
  // Check whether caller processor is BSP
  //
  WhoAmI (This, &ProcessorNumber);
  if (ProcessorNumber != mCpuConfigConextBuffer.BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check parameter Procedure
  //
  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Temporarily suppress CheckAPsStatus()
  //
  mStopCheckAPsStatus = TRUE;

  //
  // Check whether all enabled APs are idle.
  // If any enabled AP is not idle, return EFI_NOT_READY.
  //
  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {

    CpuData = &mMPSystemData.CpuData[ProcessorNumber];

    mMPSystemData.CpuList[ProcessorNumber] = FALSE;
    if (ProcessorNumber != mCpuConfigConextBuffer.BspNumber) {
      if (CpuData->State != CpuStateDisabled) {
        if (CpuData->State != CpuStateIdle) {
          mStopCheckAPsStatus = FALSE;
          return EFI_NOT_READY;
        } else {
          //
          // Mark this processor as responsible for current calling.
          //
          mMPSystemData.CpuList[ProcessorNumber] = TRUE;
        }
      }
    }
  }

  mMPSystemData.FinishCount = 0;
  mMPSystemData.StartCount  = 0;
  Blocking                  = FALSE;
  //
  // Go through all enabled APs to wakeup them for Procedure.
  // If in Single Thread mode, then only one AP is woken up, and others are waiting.
  //
  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {

    CpuData = &mMPSystemData.CpuData[ProcessorNumber];
    //
    // Check whether this processor is responsible for current calling.
    //
    if (mMPSystemData.CpuList[ProcessorNumber]) {

      mMPSystemData.StartCount++;

      AcquireSpinLock (&CpuData->CpuDataLock);
      CpuData->State = CpuStateReady;
      ReleaseSpinLock (&CpuData->CpuDataLock);

      if (!Blocking) {
        WakeUpAp (
          ProcessorNumber,
          Procedure,
          ProcedureArgument
          );
      }

      if (SingleThread) {
        Blocking = TRUE;
      }
    }
  }

  //
  // If only BSP exists then return EFI_SUCCESS as no APs to start, else return EFI_NOT_STARTED.
  //
  if (mMPSystemData.StartCount == 0) {
    mStopCheckAPsStatus = FALSE;
    if (mCpuConfigConextBuffer.NumberOfProcessors == 1)	{
      return EFI_SUCCESS;
	}
    return EFI_NOT_STARTED;
  }

  //
  // If WaitEvent is not NULL, execute in non-blocking mode.
  // BSP saves data for CheckAPsStatus(), and returns EFI_SUCCESS.
  // CheckAPsStatus() will check completion and timeout periodically.
  //
  mMPSystemData.Procedure      = Procedure;
  mMPSystemData.ProcArguments  = ProcedureArgument;
  mMPSystemData.SingleThread   = SingleThread;
  mMPSystemData.FailedCpuList  = FailedCpuList;
  mMPSystemData.ExpectedTime   = CalculateTimeout (TimeoutInMicroSeconds, &mMPSystemData.CurrentTime);
  mMPSystemData.WaitEvent      = WaitEvent;

  //
  // Allow CheckAPsStatus()
  //
  mStopCheckAPsStatus = FALSE;

  if (WaitEvent != NULL) {
    return EFI_SUCCESS;
  }

  //
  // If WaitEvent is NULL, execute in blocking mode.
  // BSP checks APs'state until all APs finish or TimeoutInMicrosecsond expires.
  //
  do {
    Status = CheckAllAPs ();
  } while (Status == EFI_NOT_READY);

  return Status;
}

/**
  Implementation of StartupThisAP() service of MP Services Protocol.

  This service lets the caller get one enabled AP to execute a caller-provided function.
  This service may only be called from the BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  Procedure             A pointer to the function to be run on the designated AP.
  @param  ProcessorNumber       The handle number of AP..
  @param  WaitEvent             The event created by the caller.
                                If it is NULL, then execute in blocking mode.
                                If it is not NULL, then execute in non-blocking mode.
  @param  TimeoutInMicroseconds The time limit in microseconds for this AP to finish the function.
                                Zero means infinity.
  @param  ProcedureArgument     Pointer to the optional parameter of the assigned function.
  @param  Finished              Indicates whether AP has finished assigned function.
                                In blocking mode, it is ignored.

  @retval EFI_SUCCESS           In blocking mode, specified AP has finished before the timeout expires.
  @retval EFI_SUCCESS           In non-blocking mode, function has been dispatched to specified AP.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_TIMEOUT           In blocking mode, the timeout expires before specified AP has finished.
  @retval EFI_NOT_READY         Specified AP is busy.
  @retval EFI_NOT_FOUND         Processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_INVALID_PARAMETER Procedure is NULL.

**/
EFI_STATUS
EFIAPI
StartupThisAP (
  IN  EFI_MP_SERVICES_PROTOCOL   *This,
  IN  EFI_AP_PROCEDURE           Procedure,
  IN  UINTN                      ProcessorNumber,
  IN  EFI_EVENT                  WaitEvent OPTIONAL,
  IN  UINTN                      TimeoutInMicroseconds,
  IN  VOID                       *ProcedureArgument OPTIONAL,
  OUT BOOLEAN                    *Finished OPTIONAL
  )
{
  CPU_DATA_BLOCK  *CpuData;
  UINTN           CallerNumber;
  EFI_STATUS      Status;

  if (Finished != NULL) {
    *Finished = TRUE;
  }

  //
  // Check whether caller processor is BSP
  //
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != mCpuConfigConextBuffer.BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check whether processor with the handle specified by ProcessorNumber exists
  //
  if (ProcessorNumber >= mCpuConfigConextBuffer.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether specified processor is BSP
  //
  if (ProcessorNumber == mCpuConfigConextBuffer.BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check parameter Procedure
  //
  if (Procedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  //
  // Temporarily suppress CheckAPsStatus()
  //
  mStopCheckAPsStatus = TRUE;

  //
  // Check whether specified AP is disabled
  //
  if (CpuData->State == CpuStateDisabled) {
    mStopCheckAPsStatus = FALSE;
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether specified AP is busy
  //
  if (CpuData->State != CpuStateIdle) {
    mStopCheckAPsStatus = FALSE;
    return EFI_NOT_READY;
  }

  //
  // Wakeup specified AP for Procedure.
  //
  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->State = CpuStateReady;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  WakeUpAp (
    ProcessorNumber,
    Procedure,
    ProcedureArgument
    );

  //
  // If WaitEvent is not NULL, execute in non-blocking mode.
  // BSP saves data for CheckAPsStatus(), and returns EFI_SUCCESS.
  // CheckAPsStatus() will check completion and timeout periodically.
  //
  CpuData->WaitEvent      = WaitEvent;
  CpuData->Finished       = Finished;
  CpuData->ExpectedTime   = CalculateTimeout (TimeoutInMicroseconds, &CpuData->CurrentTime);

  //
  // Allow CheckAPsStatus()
  //
  mStopCheckAPsStatus = FALSE;

  if (WaitEvent != NULL) {
    return EFI_SUCCESS;
  }

  //
  // If WaitEvent is NULL, execute in blocking mode.
  // BSP checks AP's state until it finishes or TimeoutInMicrosecsond expires.
  //
  do {
    Status = CheckThisAP (ProcessorNumber);
  } while (Status == EFI_NOT_READY);

  return Status;
}

/**
  Implementation of SwitchBSP() service of MP Services Protocol.

  This service switches the requested AP to be the BSP from that point onward.
  This service may only be called from the current BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       The handle number of processor.
  @param  EnableOldBSP          Whether to enable or disable the original BSP.

  @retval EFI_SUCCESS           BSP successfully switched.
  @retval EFI_DEVICE_ERROR      Caller processor is AP.
  @retval EFI_NOT_FOUND         Processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER ProcessorNumber specifies the BSP or disabled AP.
  @retval EFI_NOT_READY         Specified AP is busy.

**/
EFI_STATUS
EFIAPI
SwitchBSP (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  IN  UINTN                               ProcessorNumber,
  IN  BOOLEAN                             EnableOldBSP
  )
{
  EFI_STATUS            Status;
  BOOLEAN               OldInterruptState;
  CPU_DATA_BLOCK        *CpuData;
  CPU_STATE             CpuState;
  UINTN                 CallerNumber;
  UINT32                CurrentTimerCount;
  UINTN                 DivideValue;
  UINT8                 Vector;
  BOOLEAN               PeriodicMode;
  UINT64                CurrentTscValue;
  UINT64                TimerPeriod;

  //
  // Check whether caller processor is BSP
  //
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != mCpuConfigConextBuffer.BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check whether processor with the handle specified by ProcessorNumber exists
  //
  if (ProcessorNumber >= mCpuConfigConextBuffer.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether specified processor is BSP
  //
  if (ProcessorNumber == mCpuConfigConextBuffer.BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  //
  // Check whether specified AP is disabled
  //
  if (CpuData->State == CpuStateDisabled) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether specified AP is busy
  //
  if (CpuData->State != CpuStateIdle) {
    return EFI_NOT_READY;
  }

  //
  // Save current rate of DXE Timer
  //
  mTimer->GetTimerPeriod (mTimer, &TimerPeriod);

  //
  // Disable DXE Timer and drain pending interrupts
  //
  mTimer->SetTimerPeriod (mTimer, 0);

  //
  // Record the current local APIC timer setting of BSP
  //
  GetApicTimerState (&DivideValue, &PeriodicMode, &Vector);
  CurrentTimerCount = GetApicTimerCurrentCount ();
  DisableApicTimerInterrupt ();

  //
  // Record the current TSC value
  //
  CurrentTscValue = AsmReadTsc ();

  //
  // Before send both BSP and AP to a procedure to exchange their roles,
  // interrupt must be disabled. This is because during the exchange role
  // process, 2 CPU may use 1 stack. If interrupt happens, the stack will
  // be corrputed, since interrupt return address will be pushed to stack
  // by hardware.
  //
  OldInterruptState = SaveAndDisableInterrupts ();
  //
  // Mask LINT0 & LINT1 for the old BSP
  //
  DisableLvtInterrupts ();
  //
  // Clear the BSP bit of MSR_IA32_APIC_BASE
  //
  AsmMsrBitFieldWrite64 (EFI_MSR_IA32_APIC_BASE, N_MSR_BSP_FLAG, N_MSR_BSP_FLAG, 0);

  mMPSystemData.BSPInfo.State  = CPU_SWITCH_STATE_IDLE;
  mMPSystemData.APInfo.State   = CPU_SWITCH_STATE_IDLE;

  //
  // Need to wakeUp AP (future BSP).
  //
  WakeUpAp (
    ProcessorNumber,
    FutureBSPProc,
    NULL
    );

  AsmExchangeRole (&mMPSystemData.BSPInfo, &mMPSystemData.APInfo);

  //
  // The new BSP has come out. Since it carries the register value of the AP, need
  // to pay attention to variable which are stored in registers (due to optimization)
  //
  AsmMsrBitFieldWrite64 (EFI_MSR_IA32_APIC_BASE, N_MSR_BSP_FLAG, N_MSR_BSP_FLAG, 1);
  ProgramVirtualWireMode ();

  //
  // Restore local APIC timer setting to new BSP
  //
  InitializeApicTimer (DivideValue, CurrentTimerCount, PeriodicMode, Vector);

  //
  // Restore interrupt state.
  //
  SetInterruptState (OldInterruptState);

  //
  // Enable and restore rate of DXE Timer
  //
  mTimer->SetTimerPeriod (mTimer, TimerPeriod);

  CpuData = &mMPSystemData.CpuData[mCpuConfigConextBuffer.BspNumber];
  while (TRUE) {
    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuState = CpuData->State;
    ReleaseSpinLock (&CpuData->CpuDataLock);

    if (CpuState == CpuStateFinished) {
      break;
    }
  }

  Status              = ChangeCpuState (mCpuConfigConextBuffer.BspNumber, EnableOldBSP, EFI_CPU_CAUSE_USER_SELECTION);
  mCpuConfigConextBuffer.BspNumber  = ProcessorNumber;

  return Status;
}

/**
  Implementation of EnableDisableAP() service of MP Services Protocol.

  This service lets the caller enable or disable an AP.
  This service may only be called from the BSP.

  @param  This                   A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber        The handle number of processor.
  @param  NewAPState             Indicates whether the newstate of the AP is enabled or disabled.
  @param  HealthFlag             Indicates new health state of the AP..

  @retval EFI_SUCCESS            AP successfully enabled or disabled.
  @retval EFI_DEVICE_ERROR       Caller processor is AP.
  @retval EFI_NOT_FOUND          Processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETERS ProcessorNumber specifies the BSP.

**/
EFI_STATUS
EFIAPI
EnableDisableAP (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  IN  UINTN                               ProcessorNumber,
  IN  BOOLEAN                             NewAPState,
  IN  UINT32                              *HealthFlag OPTIONAL
  )
{
  EFI_STATUS      Status;
  UINTN           CallerNumber;

  //
  // Check whether caller processor is BSP
  //
  WhoAmI (This, &CallerNumber);
  if (CallerNumber != mCpuConfigConextBuffer.BspNumber) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Check whether processor with the handle specified by ProcessorNumber exists
  //
  if (ProcessorNumber >= mCpuConfigConextBuffer.NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  //
  // Check whether specified processor is BSP
  //
  if (ProcessorNumber == mCpuConfigConextBuffer.BspNumber) {
    return EFI_INVALID_PARAMETER;
  }

  Status  = ChangeCpuState (ProcessorNumber, NewAPState, EFI_CPU_CAUSE_USER_SELECTION);

  if (HealthFlag != NULL) {
    mMPSystemData.CpuHealthy[ProcessorNumber] = (BOOLEAN) ((*HealthFlag & PROCESSOR_HEALTH_STATUS_BIT) != 0);
  }

  return Status;
}

/**
  Implementation of WhoAmI() service of MP Services Protocol.

  This service lets the caller processor get its handle number.
  This service may be called from the BSP and APs.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       Pointer to the handle number of AP.

  @retval EFI_SUCCESS           Processor number successfully returned.
  @retval EFI_INVALID_PARAMETER ProcessorNumber is NULL

**/
EFI_STATUS
EFIAPI
WhoAmI (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  OUT UINTN                               *ProcessorNumber
  )
{
  UINTN   Index;
  UINT32  ApicID;

  //
  // Check parameter ProcessorNumber
  //
  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ApicID = GetApicId ();

  for (Index = 0; Index < mCpuConfigConextBuffer.NumberOfProcessors; Index++) {
    if (ApicID == GET_CPU_MISC_DATA (Index, ApicID)) {
      break;
    }
  }

  *ProcessorNumber = Index;
  return EFI_SUCCESS;
}

/**
  Checks APs' status periodically.

  This function is triggerred by timer perodically to check the
  state of APs for StartupAllAPs() and StartupThisAP() executed
  in non-blocking mode.

  @param  Event    Event triggered.
  @param  Context  Parameter passed with the event.

**/
VOID
EFIAPI
CheckAPsStatus (
  IN  EFI_EVENT                           Event,
  IN  VOID                                *Context
  )
{
  UINTN           ProcessorNumber;
  CPU_DATA_BLOCK  *CpuData;
  EFI_STATUS      Status;

  //
  // If CheckAPsStatus() is stopped, then return immediately.
  //
  if (mStopCheckAPsStatus) {
    return;
  }

  //
  // First, check whether pending StartupAllAPs() exists.
  //
  if (mMPSystemData.WaitEvent != NULL) {

    Status = CheckAllAPs ();
    //
    // If all APs finish for StartupAllAPs(), signal the WaitEvent for it..
    //
    if (Status != EFI_NOT_READY) {
      Status = gBS->SignalEvent (mMPSystemData.WaitEvent);
      mMPSystemData.WaitEvent = NULL;
    }
  }

  //
  // Second, check whether pending StartupThisAPs() callings exist.
  //
  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {

    CpuData = &mMPSystemData.CpuData[ProcessorNumber];

    if (CpuData->WaitEvent == NULL) {
      continue;
    }

    Status = CheckThisAP (ProcessorNumber);

    if (Status != EFI_NOT_READY) {
      gBS->SignalEvent (CpuData->WaitEvent);
      CpuData->WaitEvent = NULL;
    }
  }
  return ;
}

/**
  Checks status of all APs.

  This function checks whether all APs have finished task assigned by StartupAllAPs(),
  and whether timeout expires.

  @retval EFI_SUCCESS           All APs have finished task assigned by StartupAllAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         APs have not finished task and timeout has not expired.

**/
EFI_STATUS
CheckAllAPs (
  VOID
  )
{
  UINTN           ProcessorNumber;
  UINTN           NextProcessorNumber;
  UINTN           ListIndex;
  EFI_STATUS      Status;
  CPU_STATE       CpuState;
  CPU_DATA_BLOCK  *CpuData;

  NextProcessorNumber = 0;

  //
  // Go through all APs that are responsible for the StartupAllAPs().
  //
  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {
    if (!mMPSystemData.CpuList[ProcessorNumber]) {
      continue;
    }

    CpuData = &mMPSystemData.CpuData[ProcessorNumber];

    //
    //  Check the CPU state of AP. If it is CPU_STATE_FINISHED, then the AP has finished its task.
    //  Only BSP and corresponding AP access this unit of CPU Data. This means the AP will not modify the
    //  value of state after setting the it to CPU_STATE_FINISHED, so BSP can safely make use of its value.
    //
    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuState = CpuData->State;
    ReleaseSpinLock (&CpuData->CpuDataLock);

    if (CpuState == CpuStateFinished) {
      mMPSystemData.FinishCount++;
      mMPSystemData.CpuList[ProcessorNumber] = FALSE;

      AcquireSpinLock (&CpuData->CpuDataLock);
      CpuData->State = CpuStateIdle;
      ReleaseSpinLock (&CpuData->CpuDataLock);

      //
      // If in Single Thread mode, then search for the next waiting AP for execution.
      //
      if (mMPSystemData.SingleThread) {
        Status = GetNextWaitingProcessorNumber (&NextProcessorNumber);

        if (!EFI_ERROR (Status)) {
          WakeUpAp (
            NextProcessorNumber,
            mMPSystemData.Procedure,
            mMPSystemData.ProcArguments
            );
        }
      }
    }
  }

  //
  // If all APs finish, return EFI_SUCCESS.
  //
  if (mMPSystemData.FinishCount == mMPSystemData.StartCount) {
    return EFI_SUCCESS;
  }

  //
  // If timeout expires, report timeout.
  //
  if (CheckTimeout (&mMPSystemData.CurrentTime, &mMPSystemData.TotalTime, mMPSystemData.ExpectedTime)) {
    //
    // If FailedCpuList is not NULL, record all failed APs in it.
    //
    if (mMPSystemData.FailedCpuList != NULL) {
      *mMPSystemData.FailedCpuList = AllocatePool ((mMPSystemData.StartCount - mMPSystemData.FinishCount + 1) * sizeof(UINTN));
      ASSERT (*mMPSystemData.FailedCpuList != NULL);
    }
    ListIndex = 0;

    for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {
      //
      // Check whether this processor is responsible for StartupAllAPs().
      //
      if (mMPSystemData.CpuList[ProcessorNumber]) {
        //
        // Reset failed APs to idle state
        //
        ResetProcessorToIdleState (ProcessorNumber);
        mMPSystemData.CpuList[ProcessorNumber] = FALSE;
        if (mMPSystemData.FailedCpuList != NULL) {
          (*mMPSystemData.FailedCpuList)[ListIndex++] = ProcessorNumber;
        }
      }
    }
    if (mMPSystemData.FailedCpuList != NULL) {
      (*mMPSystemData.FailedCpuList)[ListIndex] = END_OF_CPU_LIST;
    }
    return EFI_TIMEOUT;
  }
  return EFI_NOT_READY;
}

/**
  Checks status of specified AP.

  This function checks whether specified AP has finished task assigned by StartupThisAP(),
  and whether timeout expires.

  @param  ProcessorNumber       The handle number of processor.

  @retval EFI_SUCCESS           Specified AP has finished task assigned by StartupThisAPs().
  @retval EFI_TIMEOUT           The timeout expires.
  @retval EFI_NOT_READY         Specified AP has not finished task and timeout has not expired.

**/
EFI_STATUS
CheckThisAP (
  UINTN  ProcessorNumber
  )
{
  CPU_DATA_BLOCK  *CpuData;
  CPU_STATE       CpuState;

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  //
  //  Check the CPU state of AP. If it is CPU_STATE_FINISHED, then the AP has finished its task.
  //  Only BSP and corresponding AP access this unit of CPU Data. This means the AP will not modify the
  //  value of state after setting the it to CPU_STATE_FINISHED, so BSP can safely make use of its value.
  //
  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuState = CpuData->State;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  //
  // If the APs finishes for StartupThisAP(), return EFI_SUCCESS.
  //
  if (CpuState == CpuStateFinished) {

    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuData->State = CpuStateIdle;
    ReleaseSpinLock (&CpuData->CpuDataLock);

    if (CpuData->Finished != NULL) {
      *(CpuData->Finished) = TRUE;
    }
    return EFI_SUCCESS;
  } else {
    //
    // If timeout expires for StartupThisAP(), report timeout.
    //
    if (CheckTimeout (&CpuData->CurrentTime, &CpuData->TotalTime, CpuData->ExpectedTime)) {

      if (CpuData->Finished != NULL) {
        *(CpuData->Finished) = FALSE;
      }
      //
      // Reset failed AP to idle state
      //
      ResetProcessorToIdleState (ProcessorNumber);

      return EFI_TIMEOUT;
    }
  }
  return EFI_NOT_READY;
}

/**
  Calculate timeout value and return the current performance counter value.

  Calculate the number of performance counter ticks required for a timeout.
  If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
  as infinity.

  @param  TimeoutInMicroseconds   Timeout value in microseconds.
  @param  CurrentTime             Returns the current value of the performance counter.

  @return Expected timestamp counter for timeout.
          If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
          as infinity.

**/
UINT64
CalculateTimeout (
  IN  UINTN   TimeoutInMicroseconds,
  OUT UINT64  *CurrentTime
  )
{
  //
  // Read the current value of the performance counter
  //
  *CurrentTime = GetPerformanceCounter ();

  //
  // If TimeoutInMicroseconds is 0, return value is also 0, which is recognized
  // as infinity.
  //
  if (TimeoutInMicroseconds == 0) {
    return 0;
  }

  //
  // GetPerformanceCounterProperties () returns the timestamp counter's frequency
  // in Hz. So multiply the return value with TimeoutInMicroseconds and then divide
  // it by 1,000,000, to get the number of ticks for the timeout value.
  //
  return DivU64x32 (
           MultU64x64 (
             GetPerformanceCounterProperties (NULL, NULL),
             TimeoutInMicroseconds
             ),
           1000000
           );
}

/**
  Checks whether timeout expires.

  Check whether the number of ellapsed performance counter ticks required for a timeout condition
  has been reached.  If Timeout is zero, which means infinity, return value is always FALSE.

  @param  PreviousTime         On input, the value of the performance counter when it was last read.
                               On output, the current value of the performance counter
  @param  TotalTime            The total amount of ellapsed time in performance counter ticks.
  @param  Timeout              The number of performance counter ticks required to reach a timeout condition.

  @retval TRUE                 A timeout condition has been reached.
  @retval FALSE                A timeout condition has not been reached.

**/
BOOLEAN
CheckTimeout (
  IN OUT UINT64  *PreviousTime,
  IN     UINT64  *TotalTime,
  IN     UINT64  Timeout
  )
{
  UINT64  Start;
  UINT64  End;
  UINT64  CurrentTime;
  INT64   Delta;
  INT64   Cycle;

  if (Timeout == 0) {
    return FALSE;
  }
  GetPerformanceCounterProperties (&Start, &End);
  Cycle = End - Start;
  if (Cycle < 0) {
    Cycle = -Cycle;
  }
  Cycle++;
  CurrentTime = GetPerformanceCounter();
  Delta = (INT64) (CurrentTime - *PreviousTime);
  if (Start > End) {
    Delta = -Delta;
  }
  if (Delta < 0) {
    Delta += Cycle;
  }
  *TotalTime += Delta;
  *PreviousTime = CurrentTime;
  if (*TotalTime > Timeout) {
    return TRUE;
  }
  return FALSE;
}

/**
  Searches for the next waiting AP.

  Search for the next AP that is put in waiting state by single-threaded StartupAllAPs().

  @param  NextProcessorNumber  Pointer to the processor number of the next waiting AP.

  @retval EFI_SUCCESS          The next waiting AP has been found.
  @retval EFI_NOT_FOUND        No waiting AP exists.

**/
EFI_STATUS
GetNextWaitingProcessorNumber (
  OUT UINTN                               *NextProcessorNumber
  )
{
  UINTN           ProcessorNumber;

  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {

    if (mMPSystemData.CpuList[ProcessorNumber]) {
      *NextProcessorNumber = ProcessorNumber;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Worker function for SwitchBSP().

  Worker function for SwitchBSP(), assigned to the AP which is intended to become BSP.

  @param  Buffer  Not used.

**/
VOID
EFIAPI
FutureBSPProc (
  IN  VOID     *Buffer
  )
{
  AsmExchangeRole (&mMPSystemData.APInfo, &mMPSystemData.BSPInfo);
  return ;
}

/**
  Worker function of EnableDisableAP ()

  Worker function of EnableDisableAP (). Changes state of specified processor.

  @param  ProcessorNumber Processor number of specified AP.
  @param  NewState        Desired state of the specified AP.
  @param  Cause           The cause to change AP's state.

  @retval EFI_SUCCESS     AP's state successfully changed.

**/
EFI_STATUS
ChangeCpuState (
  IN     UINTN                      ProcessorNumber,
  IN     BOOLEAN                    NewState,
  IN     EFI_CPU_STATE_CHANGE_CAUSE Cause
  )
{
  CPU_DATA_BLOCK                              *CpuData;
  EFI_COMPUTING_UNIT_CPU_DISABLED_ERROR_DATA  ErrorData;

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  mMPSystemData.DisableCause[ProcessorNumber] = Cause;

  if (!NewState) {
    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuData->State = CpuStateDisabled;
    ReleaseSpinLock (&CpuData->CpuDataLock);

    ErrorData.Cause             = Cause;
    ErrorData.SoftwareDisabled  = TRUE;

    REPORT_STATUS_CODE_EX (
      EFI_ERROR_MINOR | EFI_ERROR_CODE,
      EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_EC_DISABLED,
      (UINT32) ProcessorNumber,
      NULL,
      NULL,
      (UINT8 *) &ErrorData + sizeof (EFI_STATUS_CODE_DATA),
      sizeof (EFI_COMPUTING_UNIT_CPU_DISABLED_ERROR_DATA) - sizeof (EFI_STATUS_CODE_DATA)
      );
  } else {
    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuData->State = CpuStateIdle;
    ReleaseSpinLock (&CpuData->CpuDataLock);
  }

  return EFI_SUCCESS;
}

 /**
  C function for AP execution.

  The AP startup code jumps to this C function after switching to flat32 model.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
EFIAPI
ApProcEntry (
  IN UINTN  ProcessorNumber
  )
{
  if (mApFunction != NULL) {
    (*mApFunction) (ProcessorNumber);
  }
}

/**
  Wrapper function for all procedures assigned to AP.

  Wrapper function for all procedures assigned to AP via MP service protocol.
  It controls states of AP and invokes assigned precedure.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
ApProcWrapper (
  IN UINTN  ProcessorNumber
  )
{
  EFI_AP_PROCEDURE         Procedure;
  VOID                     *Parameter;
  CPU_DATA_BLOCK           *CpuData;
  MONITOR_MWAIT_DATA       *MonitorAddr;

  //
  // Program virtual wire mode and Local APIC timer for AP, since it will be lost after AP wake up
  //
  ProgramVirtualWireMode ();
  DisableLvtInterrupts ();
  InitializeApicTimer (mLocalApicTimerDivisor, mLocalApicTimerInitialCount, FALSE, 0);

  //
  // Initialize Debug Agent to support source level debug on AP code.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_AP, NULL, NULL);

  //
  // Call SetProcessorRegister() to restore register setting,
  // for some registers are cleared by INIT signal.
  //
  SetProcessorRegister (ProcessorNumber);

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->State = CpuStateBusy;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  //
  // Now let us check it out.
  //
  AcquireSpinLock (&CpuData->CpuDataLock);
  Procedure = CpuData->Procedure;
  Parameter = CpuData->Parameter;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  if (Procedure != NULL) {

    Procedure (Parameter);

    AcquireSpinLock (&CpuData->CpuDataLock);
    CpuData->Procedure = NULL;
    ReleaseSpinLock (&CpuData->CpuDataLock);
  }

  //
  // Update signal once finishing AP task
  //
  MonitorAddr = GetMonitorDataAddress (ProcessorNumber);
  MonitorAddr->StartupApSignal = 0;

  //
  // if BSP is switched to AP, it continue execute from here, but it carries register state
  // of the old AP, so need to reload CpuData (might be stored in a register after compiler
  // optimization) to make sure it points to the right data
  //
  if (Procedure == FutureBSPProc) {
    mBspSwitched = TRUE;
    WhoAmI (&mMpService, &mNewProcessorNumber);
    CpuData = &mMPSystemData.CpuData[mNewProcessorNumber];
    mMonitorDataAddress = GetMonitorDataAddress (mNewProcessorNumber);
    //
    // Copy the monitor data structure of the old AP into the AP stack of the new AP.
    //
    CopyMem (mMonitorDataAddress, MonitorAddr, sizeof (MONITOR_MWAIT_DATA));
  }

  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->State = CpuStateFinished;
  ReleaseSpinLock (&CpuData->CpuDataLock);
}

/**
  Function to wake up a specified AP and assign procedure to it.

  Function to wake up a specified AP and assign procedure to it.

  @param  ProcessorNumber  Handle number of the specified processor.
  @param  Procedure        Procedure to assign.
  @param  ProcArguments    Argument for Procedure.

**/
VOID
WakeUpAp (
  IN   UINTN                 ProcessorNumber,
  IN   EFI_AP_PROCEDURE      Procedure,
  IN   VOID                  *ProcArguments
  )
{
  CPU_DATA_BLOCK           *CpuData;
  MONITOR_MWAIT_DATA       *MonitorAddr;

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->Parameter  = ProcArguments;
  CpuData->Procedure  = Procedure;
  ReleaseSpinLock (&CpuData->CpuDataLock);

  //
  // Check AP wakeup manner, update signal to wake up AP 
  //
  MonitorAddr = GetMonitorDataAddress (ProcessorNumber);

  //
  // Check AP Loop mode
  //
  if (MonitorAddr->ApLoopMode == ApInHltLoop) {
    //
    // Save new the AP Loop mode
    //
    MonitorAddr->ApLoopMode = mExchangeInfo->ApLoopMode;
    SendInitSipiSipiIpis (
      FALSE,
      GET_CPU_MISC_DATA (ProcessorNumber, ApicID),
      ApProcWrapper
      );

  } else if (MonitorAddr->ApLoopMode == ApInMwaitLoop ||
             MonitorAddr->ApLoopMode == ApInRunLoop) {
    //
    // Save new the AP Loop mode
    //
    MonitorAddr->ApLoopMode = mExchangeInfo->ApLoopMode;
    mApFunction = ApProcWrapper;
    //
    // AP is in Mwait C1 state
    //
    MonitorAddr->MwaitTargetCstate = 0;  
    MonitorAddr->StartupApSignal = STARTUP_AP_SIGNAL | (UINT16) ProcessorNumber;

  } else {

    ASSERT (FALSE);
  }

}

/**
  Reset an AP to Idle state.
  
  Any task being executed by the AP will be aborted and the AP 
  will be waiting for a new task in Wait-For-SIPI state.

  @param ProcessorNumber  The handle number of processor.
**/
VOID
ResetProcessorToIdleState (
  UINTN      ProcessorNumber
  )
{
  CPU_DATA_BLOCK  *CpuData;

  SendInitSipiSipiIpis (
    FALSE,
    GET_CPU_MISC_DATA (ProcessorNumber, ApicID),
    NULL
    );

  CpuData = &mMPSystemData.CpuData[ProcessorNumber];

  AcquireSpinLock (&CpuData->CpuDataLock);
  CpuData->State = CpuStateIdle;
  ReleaseSpinLock (&CpuData->CpuDataLock);
}

/**
  Check whether any AP is running for assigned task.

  @retval TRUE           Some APs are running.
  @retval FALSE          No AP is running.
**/
BOOLEAN
ApRunning (
  VOID
  )
{
  CPU_DATA_BLOCK  *CpuData;
  UINTN           ProcessorNumber;

  for (ProcessorNumber = 0; ProcessorNumber < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorNumber++) {
    CpuData = &mMPSystemData.CpuData[ProcessorNumber];
    if (ProcessorNumber != mCpuConfigConextBuffer.BspNumber) {
      if (CpuData->State != CpuStateIdle && CpuData->State != CpuStateDisabled) {
        return TRUE;
      }
    }
  }
  return FALSE;
}
