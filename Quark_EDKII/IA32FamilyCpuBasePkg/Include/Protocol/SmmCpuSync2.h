/** @file


  Smm CPU Sync2 protocol definition.

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

#ifndef _SMM_CPU_SYNC2_PROTOCOL_H_
#define _SMM_CPU_SYNC2_PROTOCOL_H_

#include "SmmCpuSync.h"

#define SMM_CPU_SYNC2_PROTOCOL_GUID \
  { \
    0x9db72e22, 0x9262, 0x4a18, { 0x8f, 0xe0, 0x85, 0xe0, 0x3d, 0xfa, 0x96, 0x73 } \
  }

typedef struct _SMM_CPU_SYNC2_PROTOCOL SMM_CPU_SYNC2_PROTOCOL;

typedef enum {
  SmmCpuSync2StateNormal,
  SmmCpuSync2StateDelayed,
  SmmCpuSync2StateBlocked,
  SmmCpuSync2StateDisabled,
  SmmCpuSync2StateMax
} SMM_CPU_SYNC2_CPU_STATE;

//
// Protocol functions
//

/**
  Given timeout constraint, wait for all APs to arrive, and insure if this function returns EFI_SUCCESS, then
  no AP will execute normal mode code before entering SMM, so it is safe to access shared hardware resource 
  between SMM and normal mode. Note if there are SMI disabled APs, this function will return EFI_NOT_READY.


  @param  This                  A pointer to the SMM_CPU_SYNC_PROTOCOL instance.

  @retval EFI_SUCCESS           No AP will execute normal mode code before entering SMM, so it is safe to access 
                                shared hardware resource between SMM and normal mode
  @retval EFI_NOT_READY         One or more CPUs may still execute normal mode code
  @retval EFI_DEVICE_ERROR      Error occured in obtaining CPU status.

**/
typedef
EFI_STATUS
(EFIAPI *SMM_CPU_SYNC2_CHECK_AP_ARRIVAL)(
  IN    SMM_CPU_SYNC2_PROTOCOL            *This
  );


/**
  Set SMM synchronization mode starting from the next SMI run.

  @param  This                  A pointer to the SMM_CPU_SYNC_PROTOCOL instance.
  @param  SyncMode              The SMM synchronization mode to be set and effective from the next SMI run.

  @retval EFI_SUCCESS           The function completes successfully.
  @retval EFI_INVALID_PARAMETER SynMode is not valid.

**/
typedef
EFI_STATUS
(EFIAPI *SMM_CPU_SYNC2_SET_MODE) (
  IN    SMM_CPU_SYNC2_PROTOCOL           *This,
  IN    SMM_CPU_SYNC_MODE                SyncMode
  );


/**
  Get current effective SMM synchronization mode.

  @param  This                  A pointer to the SMM_CPU_SYNC_PROTOCOL instance.
  @param  SyncMode              Output parameter. The current effective SMM synchronization mode.

  @retval EFI_SUCCESS           The SMM synchronization mode has been retrieved successfully.
  @retval EFI_INVALID_PARAMETER SyncMode is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *SMM_CPU_SYNC2_GET_MODE) (
  IN    SMM_CPU_SYNC2_PROTOCOL           *This,
  OUT   SMM_CPU_SYNC_MODE                *SyncMode
  );


/**
  Checks CPU SMM synchronization state

  @param  This                  A pointer to the SMM_CPU_SYNC_PROTOCOL instance.

  @param  CpuIndex              Index of the CPU for which the state is to be retrieved.

  @param  CpuState              The state of the CPU

  @retval EFI_SUCCESS           Returns EFI_SUCCESS if the CPU's state is retrieved.

  @retval EFI_INVALID_PARAMETER Returns EFI_INVALID_PARAMETER if CpuIndex is invalid or CpuState is NULL

  @retval EFI_DEVICE_ERROR      Error occured in obtaining CPU status.
**/
typedef
EFI_STATUS
(EFIAPI *SMM_CPU_SYNC2_CHECK_CPU_STATE)(
  IN    SMM_CPU_SYNC2_PROTOCOL            *This,
  IN    UINTN                             CpuIndex,
  OUT   SMM_CPU_SYNC2_CPU_STATE           *CpuState               
  );


/**
  Change CPU's SMM enabling state.

  @param  This                  A pointer to the SMM_CPU_SYNC_PROTOCOL instance.

  @param  CpuIndex              Index of the CPU to enable / disable SMM

  @param  Enable                If TRUE, enable SMM; if FALSE disable SMM

  @retval EFI_SUCCESS           The CPU's SMM enabling state is changed.

  @retval EFI_INVALID_PARAMETER Returns EFI_INVALID_PARAMETER if CpuIndex is invalid

  @retval EFI_UNSUPPORTED       Returns EFI_UNSUPPORTED the CPU does not support dynamically enabling / disabling SMI

  @retval EFI_DEVICE_ERROR      Error occured in changing SMM enabling state.
**/
typedef
EFI_STATUS
(EFIAPI *SMM_CPU_SYNC2_CHANGE_SMM_ENABLING)(
  IN    SMM_CPU_SYNC2_PROTOCOL           *This,
  IN    UINTN                            CpuIndex,
  IN    BOOLEAN                          Enable
  );


/**
  Send SMI IPI to a specific CPU

  @param  This                  A pointer to the SMM_CPU_SYNC_PROTOCOL instance.

  @param  CpuIndex              Index of the CPU to send SMI to. "-1" means all-exclude-self.

  @retval EFI_SUCCESS           The SMI IPI is sent successfully.

  @retval EFI_INVALID_PARAMETER Returns EFI_INVALID_PARAMETER if CpuIndex is invalid

  @retval EFI_DEVICE_ERROR      Error occured in sending SMI IPI.
**/
typedef
EFI_STATUS
(EFIAPI *SMM_CPU_SYNC2_SEND_SMI)(
  IN    SMM_CPU_SYNC2_PROTOCOL           *This,
  IN    UINTN                            CpuIndex
  );


/**
  Send SMI IPI to all CPUs excluding self

  @param  This                  A pointer to the SMM_CPU_SYNC_PROTOCOL instance.

  @retval EFI_SUCCESS           The SMI IPI is sent successfully.

  @retval EFI_INVALID_PARAMETER Returns EFI_INVALID_PARAMETER if CpuIndex is invalid

  @retval EFI_DEVICE_ERROR      Error occured in sending SMI IPI.
**/
typedef
EFI_STATUS
(EFIAPI *SMM_CPU_SYNC2_SEND_SMI_ALL_EXCLUDING_SELF)(
  IN    SMM_CPU_SYNC2_PROTOCOL           *This
  );

/**
  This protocol provides services on SMM CPU syncrhonization related status and controls
**/
struct _SMM_CPU_SYNC2_PROTOCOL {
  SMM_CPU_SYNC2_CHECK_AP_ARRIVAL            CheckApArrival;
  SMM_CPU_SYNC2_SET_MODE                    SetMode;
  SMM_CPU_SYNC2_GET_MODE                    GetMode;
  SMM_CPU_SYNC2_CHECK_CPU_STATE             CheckCpuState;
  SMM_CPU_SYNC2_CHANGE_SMM_ENABLING         ChangeSmmEnabling;
  SMM_CPU_SYNC2_SEND_SMI                    SendSmi;
  SMM_CPU_SYNC2_SEND_SMI_ALL_EXCLUDING_SELF SendSmiAllExcludingSelf;
};

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gSmmCpuSync2ProtocolGuid;


#endif
