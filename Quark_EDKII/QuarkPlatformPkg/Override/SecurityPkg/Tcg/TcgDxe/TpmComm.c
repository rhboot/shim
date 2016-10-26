/** @file  
  Utility functions used by TPM Dxe driver.

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

#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/TpmCommLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include "TpmComm.h"

#ifndef TPM_PP_BIOS_FAILURE
#define TPM_PP_BIOS_FAILURE    ((TPM_RESULT)(-0x0f))
#endif

/**
  Extend a TPM PCR.

  @param[in]  TpmHandle       TPM handle.  
  @param[in]  DigestToExtend  The 160 bit value representing the event to be recorded.  
  @param[in]  PcrIndex        The PCR to be updated.
  @param[out] NewPcrValue     New PCR value after extend.  
  
  @retval EFI_SUCCESS         Operation completed successfully.
  @retval EFI_DEVICE_ERROR    The command was unsuccessful.

**/
EFI_STATUS
TpmCommExtend (
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      TPM_DIGEST                *DigestToExtend,
  IN      TPM_PCRINDEX              PcrIndex,
     OUT  TPM_DIGEST                *NewPcrValue
  )
{
  EFI_STATUS                        Status;
  TPM_DIGEST                        NewValue;
  TPM_RQU_COMMAND_HDR               CmdHdr;
  TPM_RSP_COMMAND_HDR               RspHdr;

  if (NewPcrValue == NULL) {
    NewPcrValue = &NewValue;
  }

  CmdHdr.tag = TPM_TAG_RQU_COMMAND;
  CmdHdr.paramSize =
    sizeof (CmdHdr) + sizeof (PcrIndex) + sizeof (*DigestToExtend);
  CmdHdr.ordinal = TPM_ORD_Extend;
  Status = TisPcExecute (
             TpmHandle,
             "%h%d%r%/%h%r",
             &CmdHdr,
             PcrIndex,
             DigestToExtend,
             (UINTN)sizeof (*DigestToExtend),
             &RspHdr,
             NewPcrValue,
             (UINTN)sizeof (*NewPcrValue)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (RspHdr.returnCode != 0) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

/**
  Get TPM capability flags.

  @param[in]  TpmHandle    TPM handle.  
  @param[in]  FlagSubcap   Flag subcap.  
  @param[out] FlagBuffer   Pointer to the buffer for returned flag structure.
  @param[in]  FlagSize     Size of the buffer.  
  
  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR The command was unsuccessful.

**/
EFI_STATUS
TpmCommGetFlags (
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      UINT32                    FlagSubcap,
     OUT  VOID                      *FlagBuffer,
  IN      UINTN                     FlagSize
  )
{
  EFI_STATUS                        Status;
  TPM_RQU_COMMAND_HDR               CmdHdr;
  TPM_RSP_COMMAND_HDR               RspHdr;
  UINT32                            Size;

  CmdHdr.tag = TPM_TAG_RQU_COMMAND;
  CmdHdr.paramSize = sizeof (CmdHdr) + sizeof (UINT32) * 3;
  CmdHdr.ordinal = TPM_ORD_GetCapability;

  Status = TisPcExecute (
             TpmHandle,
             "%h%d%d%d%/%h%d%r",
             &CmdHdr,
             TPM_CAP_FLAG,
             sizeof (FlagSubcap),
             FlagSubcap,
             &RspHdr,
             &Size,
             FlagBuffer,
             FlagSize
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (RspHdr.returnCode != 0) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

/**
  Add a new entry to the Event Log.

  @param[in, out] EventLogPtr   Pointer to the Event Log data.  
  @param[in, out] LogSize       Size of the Event Log.  
  @param[in]      MaxSize       Maximum size of the Event Log.
  @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.  
  @param[in]      NewEventData  Pointer to the new event data.  
  
  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
TpmCommLogEvent (
  IN OUT  UINT8                     **EventLogPtr,
  IN OUT  UINTN                     *LogSize,
  IN      UINTN                     MaxSize,
  IN      TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  UINT32                            NewLogSize;

  NewLogSize = sizeof (*NewEventHdr) + NewEventHdr->EventSize;
  if (NewLogSize + *LogSize > MaxSize) {
    return EFI_OUT_OF_RESOURCES;
  }

  *EventLogPtr += *LogSize;
  *LogSize += NewLogSize;
  CopyMem (*EventLogPtr, NewEventHdr, sizeof (*NewEventHdr));
  CopyMem (
    *EventLogPtr + sizeof (*NewEventHdr),
    NewEventData,
    NewEventHdr->EventSize
    );
  return EFI_SUCCESS;
}

/**
  Send TPM_Startup command to TPM.

  @param[in] TpmHandle    TPM handle.
  @param[in] BootMode     Boot mode.

  @retval TPM_RESULT      TPM return code.

**/
TPM_RESULT
TpmCommStartup (
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      EFI_BOOT_MODE             BootMode
  )
{
  TPM_STARTUP_TYPE                  TpmSt;
  TPM_RESULT                        TpmResponse;

  TpmSt = TPM_ST_CLEAR;
  if (BootMode == BOOT_ON_S3_RESUME) {
    TpmSt = TPM_ST_STATE;
  }

  //
  // send Tpm command TPM_ORD_Startup
  //
  TpmSt       = SwapBytes16 (TpmSt);
  TpmResponse = TpmCommandNoReturnData (TpmHandle, TPM_ORD_Startup, (UINTN)sizeof TpmSt, &TpmSt);

  return TpmResponse;
}

/**
  Send TPM_ContinueSelfTest command to TPM.

  @param[in] TpmHandle    TPM handle.
 
  @retval TPM_RESULT      TPM return code.

**/
TPM_RESULT
TpmCommContinueSelfTest (
  IN      TIS_TPM_HANDLE            TpmHandle
  )
{
  TPM_RESULT TpmResponse;

  //
  // send Tpm command TPM_ORD_ContinueSelfTest
  //
  TpmResponse = TpmCommandNoReturnData (TpmHandle, TPM_ORD_ContinueSelfTest, 0, NULL);

  return TpmResponse;
}

/**
  Send TPM_GetSelfTestResult command to TPM.

  @param[in] TpmHandle    TPM handle.
 
  @retval TPM_RESULT      TPM return code.

**/
TPM_RESULT
TpmCommGetSelfTestResult (
  IN      TIS_TPM_HANDLE            TpmHandle
  )
{
  TPM_RESULT TpmResponse;

  //
  // send Tpm command TPM_ORD_ContinueSelfTest
  //
  TpmResponse = TpmCommandNoReturnData (TpmHandle, TPM_ORD_GetTestResult, 0, NULL);

  return TpmResponse;
}

/**
  Issue TSC_PhysicalPresence command to TPM.

  Adapted from: SecurityPkg/Tcg/TcgPei/TpmComm.c

  @param[in] TpmHandle           TPM handle.  
  @param[in] PhysicalPresence    The state to set the TPM's Physical Presence flags.
  
  @retval TPM_RESULT             TPM return code.

**/
TPM_RESULT
TpmCommPhysicalPresence (
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      TPM_PHYSICAL_PRESENCE     PhysicalPresence
  )
{
  EFI_STATUS                        Status;
  TPM_RQU_COMMAND_HDR               *TpmRqu;
  TPM_PHYSICAL_PRESENCE             *TpmPp;
  TPM_RSP_COMMAND_HDR               TpmRsp;
  UINT8                             Buffer[sizeof (*TpmRqu) + sizeof (*TpmPp)];

  TpmRqu = (TPM_RQU_COMMAND_HDR*)Buffer;
  TpmPp  = (TPM_PHYSICAL_PRESENCE*)(TpmRqu + 1);

  TpmRqu->tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  TpmRqu->paramSize = SwapBytes32 (sizeof (Buffer));
  TpmRqu->ordinal   = SwapBytes32 (TSC_ORD_PhysicalPresence);
  WriteUnaligned16 (TpmPp, (TPM_PHYSICAL_PRESENCE) SwapBytes16 (PhysicalPresence));

  Status = TisPcExecute (
             TpmHandle,
             "%r%/%r",
             (UINT8*)TpmRqu,
             (UINTN) sizeof (Buffer),
             (UINT8*)&TpmRsp,
             (UINTN)sizeof (TpmRsp)
             );

  if (EFI_ERROR (Status) || (TpmRsp.tag != SwapBytes16 (TPM_TAG_RSP_COMMAND))) {
    return TPM_PP_BIOS_FAILURE;
  }

  return SwapBytes32 (TpmRsp.returnCode);
}


/**
  Issue a TPM command for which no additional output data will be returned.
  
  Adapted from: SecurityPkg/Library/DxeTcgPhysicalPresenceLib/DxeTcgPhysicalPresenceLib.c

  @param[in] TpmHandle                TPM handle. 
  @param[in] Ordinal                  TPM command code.
  @param[in] AdditionalParameterSize  Additional parameter size.
  @param[in] AdditionalParameters     Pointer to the Additional paramaters.

  @retval TPM_RESULT                  TPM return code.

**/
TPM_RESULT
TpmCommandNoReturnData (
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      TPM_COMMAND_CODE          Ordinal,
  IN      UINTN                     AdditionalParameterSize,
  IN      VOID                      *AdditionalParameters
  )
{
  EFI_STATUS                        Status;
  TPM_RQU_COMMAND_HDR               *TpmRqu;
  TPM_RSP_COMMAND_HDR               TpmRsp;
  UINT32                            Size;

  TpmRqu = (TPM_RQU_COMMAND_HDR*) AllocatePool (sizeof (*TpmRqu) + AdditionalParameterSize);
  if (TpmRqu == NULL) {
    return TPM_PP_BIOS_FAILURE;
  }

  TpmRqu->tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Size              = (UINT32)(sizeof (*TpmRqu) + AdditionalParameterSize);
  TpmRqu->paramSize = SwapBytes32 (Size);
  TpmRqu->ordinal   = SwapBytes32 (Ordinal);
  CopyMem (TpmRqu + 1, AdditionalParameters, AdditionalParameterSize);

  Status = TisPcExecute (
             TpmHandle,
             "%r%/%r",
             (UINT8*)TpmRqu,
             (UINTN) Size,
             (UINT8*)&TpmRsp,
             (UINTN)sizeof (TpmRsp)
             );

  FreePool (TpmRqu);
  if (EFI_ERROR (Status) || (TpmRsp.tag != SwapBytes16 (TPM_TAG_RSP_COMMAND))) {
    return TPM_PP_BIOS_FAILURE;
  }

  return SwapBytes32 (TpmRsp.returnCode);
}


/**
  Send following commands in order to ENABLE+ACTIVATE TPM
  
  (i)   TSC_PhysicalPresence(physicalPresenceCMDEnable) - flag that software going to set physical presence
  (ii)  TSC_PhysicalPresence(physicalPresencePresent) - set physical presence by default
  (iii) TPM_PhysicalEnable - enable
  (iv)  TPM_PhysicalSetDeactivated(FALSE), i.e. activate
  (v)   TSC_PhysicalPresence(physicalPresenceNotPresent) - unset physical presence

  @param[in] TpmHandle   TPM handle.

  @retval TPM_RESULT     TPM return code.

**/
TPM_RESULT
TpmCommEnableActivate (
  IN      TIS_TPM_HANDLE            TpmHandle
  )
{
  TPM_RESULT  TpmResponse;
  BOOLEAN     DeactivateTPM;

  //
  // Set flag that software going to set physical presence.
  //
  TpmResponse = TpmCommPhysicalPresence (TpmHandle, TPM_PHYSICAL_PRESENCE_CMD_ENABLE);
  if (TpmResponse != TPM_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "TpmCommEnableActivate(): TPM_PHYSICAL_PRESENCE_CMD_ENABLE failed\n"));
    return TpmResponse;
  }

  //
  // Set physical presence
  //
  TpmResponse = TpmCommPhysicalPresence (TpmHandle, TPM_PHYSICAL_PRESENCE_PRESENT);
  if (TpmResponse != TPM_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "TpmCommEnableActivate(): TPM_PHYSICAL_PRESENCE_PRESENT failed\n"));
    return TpmResponse;
  }

  //
  // Enable the TPM
  //
  TpmResponse = TpmCommandNoReturnData (TpmHandle, TPM_ORD_PhysicalEnable, 0, NULL);
  if (TpmResponse != TPM_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "TpmCommEnableActivate(): TPM_ORD_PhysicalEnable failed\n"));
    return TpmResponse;
  }

  //
  // Activate the TPM
  //
  DeactivateTPM = FALSE;
  TpmResponse = TpmCommandNoReturnData (TpmHandle, TPM_ORD_PhysicalSetDeactivated, sizeof (DeactivateTPM), &DeactivateTPM);
  if (TpmResponse != TPM_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "TpmCommEnableActivate(): TPM_ORD_PhysicalSetDeactivated(false) failed\n"));
    return TpmResponse;
  }

  //
  // Unset Physical Presence
  //
  TpmResponse = TpmCommPhysicalPresence (TpmHandle, TPM_PHYSICAL_PRESENCE_NOTPRESENT);
  if (TpmResponse != TPM_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "TpmCommEnableActivate(): TPM_PHYSICAL_PRESENCE_NOTPRESENT failed\n"));
  }

  return TpmResponse;
}
