/** @file
  Include file for SMM RAS Services protocol implementation.

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

**/
#ifndef _CPU_SERVICE_H_
#define _CPU_SERVICE_H_

typedef enum {
  SmmCpuNone,
  SmmCpuAdd,
  SmmCpuRemove,
  SmmCpuSwitchBsp
} SMM_CPU_OPERATION;


//
// SMM CPU Service Protocol function prototypes.
//

/**
  Gets processor information on the requested processor at the instant this call is made. 

  @param[in]  This                 A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in]  ProcessorNumber      The handle number of processor.
  @param[out] ProcessorInfoBuffer  A pointer to the buffer where information for
                                   the requested processor is deposited.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is invalid.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.

**/
EFI_STATUS
EFIAPI
SmmGetProcessorInfo ( 
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL *This, 
  IN       UINTN                        ProcessorNumber, 
  OUT      EFI_PROCESSOR_INFORMATION    *ProcessorInfoBuffer 
  );

/**
  This service switches the requested AP to be the BSP since the next SMI. 

  @param[in] This             A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in] ProcessorNumber  The handle number of AP that is to become the new BSP.

  @retval EFI_SUCCESS             BSP will be switched in next SMI.
  @retval EFI_UNSUPPORTED         Switching the BSP or a processor to be hot-removed is not supported.
  @retval EFI_NOT_FOUND           The processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is invalid.
**/
EFI_STATUS
EFIAPI
SmmSwitchBsp ( 
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL *This, 
  IN       UINTN                        ProcessorNumber 
  );

/**
  Notify that a processor was hot-added. 

  @param[in] This                A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in] ProcessorId         Local APIC ID of the hot-added processor.
  @param[out] ProcessorNumber    The handle number of the hot-added processor.

  @retval EFI_SUCCESS            The hot-addition of the specified processos was succesfully notified.
  @retval EFI_UNSUPPORTED        Hot addition of processor is not supported.
  @retval EFI_NOT_FOUND          The processor with the handle specified by ProcessorNumber does not exist.
  @retval EFI_INVALID_PARAMETER  ProcessorNumber is invalid.
  @retval EFI_ALREADY_STARTED    The processor is already online in the system.
**/
EFI_STATUS
EFIAPI
SmmAddProcessor ( 
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL  *This,
  IN       UINT64                        ProcessorId,
  OUT      UINTN                         *ProcessorNumber
  );

/**
  Notify that a processor was hot-removed. 

  @param[in] This                A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[in] ProcessorNumber     The handle number of the hot-added processor.

  @retval EFI_SUCCESS            The hot-removal of the specified processos was succesfully notified.
  @retval EFI_UNSUPPORTED        Hot removal of processor is not supported.
  @retval EFI_UNSUPPORTED        Hot removal of BSP is not supported.
  @retval EFI_UNSUPPORTED        Hot removal of a processor with pending hot-plug operation is not supported.
  @retval EFI_INVALID_PARAMETER  ProcessorNumber is invalid.
**/
EFI_STATUS
EFIAPI
SmmRemoveProcessor ( 
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL  *This, 
  IN       UINTN                         ProcessorNumber
  );

/**
  This return the handle number for the calling processor.

  @param[in] This                 A pointer to the EFI_SMM_CPU_SERVICE_PROTOCOL instance.
  @param[out] ProcessorNumber      The handle number of currently executing processor.

  @retval EFI_SUCCESS             The current processor handle number was returned 
                                  in ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.

**/
EFI_STATUS
EFIAPI
SmmWhoAmI ( 
  IN CONST EFI_SMM_CPU_SERVICE_PROTOCOL *This, 
  OUT      UINTN                        *ProcessorNumber 
  );

/**
  Register exception handler.

  @param  This                  A pointer to the SMM_CPU_SERVICE_PROTOCOL instance.
  @param  ExceptionType         Defines which interrupt or exception to hook. Type EFI_EXCEPTION_TYPE and 
                                the valid values for this parameter are defined in EFI_DEBUG_SUPPORT_PROTOCOL 
                                of the UEFI 2.0 specification. 
  @param  InterruptHandler      A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER 
                                that is called when a processor interrupt occurs. 
                                If this parameter is NULL, then the handler will be uninstalled. 

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.         
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was previously installed. 
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not previously installed. 
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS 
EFIAPI
SmmRegisterExceptionHandler (
  IN EFI_SMM_CPU_SERVICE_PROTOCOL  *This,
  IN EFI_EXCEPTION_TYPE            ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  );

//
// Internal function prototypes
//

/**
  Initializes the pointer to the external exception vector table.

  @param  VectorTable  Address of the external exception vector table.

**/
VOID
EFIAPI
InitializeSmmExternalVectorTablePtr (
  EFI_CPU_INTERRUPT_HANDLER  *VectorTable
  );

/**
  Update the SMM CPU list per the pending operation.

  This function is called after return from SMI handlers.
**/
VOID
SmmCpuUpdate (
  VOID
  );

/**
  Initialize SMM CPU Services.

  It installs EFI SMM CPU Services Protocol.

  @param ImageHandle The firmware allocated handle for the EFI image.

  @retval EFI_SUCCESS    EFI SMM CPU Services Protocol was installed successfully.
**/
EFI_STATUS
InitializeSmmCpuServices (
  IN EFI_HANDLE  Handle
  );

#endif

