/** @file

  Include file for MP Services Protocol

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

  Module Name:  MpService.h

**/

#ifndef _MP_SERVICE_H_
#define _MP_SERVICE_H_

#include "MpCommon.h"

#define CPU_CHECK_AP_INTERVAL                 0x10  // microseconds
//
//  The MP data structure follows.
//
#define CPU_SWITCH_STATE_IDLE   0
#define CPU_SWITCH_STATE_STORED 1
#define CPU_SWITCH_STATE_LOADED 2

extern EFI_MP_SERVICES_PROTOCOL     mMpService;
extern EFI_TIMER_ARCH_PROTOCOL      *mTimer;

typedef struct {
  UINT8             State;        // offset 0
  UINTN             StackPointer; // offset 4 / 8
  IA32_DESCRIPTOR   Gdtr;         // offset 8 / 16
  IA32_DESCRIPTOR   Idtr;         // offset 14 / 26
} CPU_EXCHANGE_ROLE_INFO;

typedef enum {
  CpuStateIdle,
  CpuStateReady,
  CpuStateBusy,
  CpuStateFinished,
  CpuStateDisabled
} CPU_STATE;

//
// Define Individual Processor Data block.
//
typedef struct {
  EFI_AP_PROCEDURE volatile      Procedure;
  VOID* volatile                 Parameter;

  EFI_EVENT                      WaitEvent;
  BOOLEAN                        *Finished;
  UINT64                         ExpectedTime;
  UINT64                         CurrentTime;
  UINT64                         TotalTime;

  SPIN_LOCK                      CpuDataLock;
  CPU_STATE volatile             State;

} CPU_DATA_BLOCK;

//
// Define MP data block which consumes individual processor block.
//
typedef struct {
  SPIN_LOCK                   APSerializeLock;

  CPU_EXCHANGE_ROLE_INFO      BSPInfo;
  CPU_EXCHANGE_ROLE_INFO      APInfo;

  EFI_EVENT                   CheckAPsEvent;

  UINTN                       FinishCount;
  UINTN                       StartCount;

  BOOLEAN                     CpuList[FixedPcdGet32(PcdCpuMaxLogicalProcessorNumber)];

  EFI_AP_PROCEDURE            Procedure;
  VOID                        *ProcArguments;
  BOOLEAN                     SingleThread;
  EFI_EVENT                   WaitEvent;
  UINTN                       **FailedCpuList;
  UINT64                      ExpectedTime;
  UINT64                      CurrentTime;
  UINT64                      TotalTime;

  CPU_DATA_BLOCK              CpuData[FixedPcdGet32(PcdCpuMaxLogicalProcessorNumber)];
  EFI_CPU_STATE_CHANGE_CAUSE  DisableCause[FixedPcdGet32(PcdCpuMaxLogicalProcessorNumber)];
  BOOLEAN                     CpuHealthy[FixedPcdGet32(PcdCpuMaxLogicalProcessorNumber)];

} MP_SYSTEM_DATA;

typedef struct {
  ACPI_CPU_DATA              AcpiCpuData;
  IA32_DESCRIPTOR            GdtrProfile;
  IA32_DESCRIPTOR            IdtrProfile;
} MP_CPU_SAVED_DATA;

extern MP_SYSTEM_DATA                   mMPSystemData;

/**
  Implementation of GetNumberOfProcessors() service of MP Services Protocol.

  This service retrieves the number of logical processor in the platform
  and the number of those logical processors that are enabled on this boot.
  This service may only be called from the BSP.

  @param  This                       A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  NumberOfCPUs               Pointer to the total number of logical processors in the system,
                                     including the BSP and disabled APs.
  @param  NumberOfEnabledCPUs        Pointer to the number of enabled logical processors that exist
                                     in system, including the BSP.

  @retval EFI_SUCCESS	               Number of logical processors and enabled logical processors retrieved..
  @retval EFI_DEVICE_ERROR           Caller processor is AP.
  @retval EFI_INVALID_PARAMETER      NumberOfProcessors is NULL
  @retval EFI_INVALID_PARAMETER      NumberOfEnabledProcessors is NULL

**/
EFI_STATUS
EFIAPI
GetNumberOfProcessors (
  IN  EFI_MP_SERVICES_PROTOCOL            *This,
  OUT UINTN                               *NumberOfCPUs,
  OUT UINTN                               *NumberOfEnabledCPUs
  );

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
  );

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
  );

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
  );

/**
  Implementation of SwitchBSP() service of MP Services Protocol.

  This service switches the requested AP to be the BSP from that point onward.
  This service may only be called from the current BSP.

  @param  This                  A pointer to the EFI_MP_SERVICES_PROTOCOL instance.
  @param  ProcessorNumber       The handle number of processor.
  @param  OldBSPState           Whether to enable or disable the original BSP.

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
  IN  BOOLEAN                             OldBSPState
  );

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
  );

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
  );

/**
  This function is called by both the BSP and the AP which is to become the BSP to 
  Exchange execution context including stack between them. After return from this
  function, the BSP becomes AP and the AP becomes the BSP.

  @param MyInfo      Pointer to buffer holding the exchanging information for the executing processor.
  @param OthersInfo  Pointer to buffer holding the exchanging information for the peer.
**/
VOID
EFIAPI
AsmExchangeRole (
  IN   CPU_EXCHANGE_ROLE_INFO    *MyInfo,
  IN   CPU_EXCHANGE_ROLE_INFO    *OthersInfo
  );

/**
  Worker function for SwitchBSP().

  Worker function for SwitchBSP(), assigned to the AP which is intended to become BSP.

  @param  Buffer  Not used.

**/
VOID
EFIAPI
FutureBSPProc (
  IN  VOID     *Buffer
  );

/**
  Worker function of EnableDisableAP ()

  Worker function of EnableDisableAP (). Changes state of specified processor.

  @param  CpuNumber       Processor number of specified AP.
  @param  NewState        Desired state of the specified AP.
  @param  Cause           The cause to change AP's state.

  @retval EFI_SUCCESS     AP's state successfully changed.

**/
EFI_STATUS
ChangeCpuState (
  IN     UINTN                      CpuNumber,
  IN     BOOLEAN                    NewState,
  IN     EFI_CPU_STATE_CHANGE_CAUSE Cause
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

 /**
  C function for AP execution.

  The AP startup code jumps to this C function after switching to flat32 model.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
EFIAPI
ApProcEntry (
  IN UINTN  ProcessorNumber
  );

/**
  Wrapper function for all procedures assigned to AP.

  Wrapper function for all procedures assigned to AP via MP service protocol.
  It controls states of AP and invokes assigned precedure.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
ApProcWrapper (
  IN UINTN  ProcessorNumber
  );

/**
  Function to wake up a specified AP and assign procedure to it.
  
  Function to wake up a specified AP and assign procedure to it.

  @param  ProcessorNumber  Handle number of the specified processor.
  @param  Procedure        Procedure to assign.
  @param  Parameter        Argument for Procedure.

**/
VOID
WakeUpAp (
  IN   UINTN                 ProcessorNumber,
  IN   EFI_AP_PROCEDURE      Procedure,
  IN   VOID                  *Parameter
  );

/**
  Reset an AP to Idle state.
  
  Any task being executed by the AP will be aborted and the AP 
  will be waiting for a new task in Wait-For-SIPI state.

  @param ProcessorNumber  The handle number of processor.
**/
VOID
ResetProcessorToIdleState (
  UINTN      ProcessorNumber
  );

/**
  Check whether any AP is running for assigned task.

  @retval TRUE           Some APs are running.
  @retval FALSE          No AP is running.
**/
BOOLEAN
ApRunning (
  VOID
  );

#endif
