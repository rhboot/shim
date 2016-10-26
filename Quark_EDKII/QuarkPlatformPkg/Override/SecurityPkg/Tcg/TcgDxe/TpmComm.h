/** @file  
  Definitions and function prototypes used by TPM DXE driver.

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

#ifndef _TPM_COMM_H_
#define _TPM_COMM_H_

#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/UefiTcgPlatform.h>

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
  );

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
  );

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
     OUT  VOID                      *Buffer,
  IN      UINTN                     Size
  );

/**
  Send formatted command to TPM for execution and return formatted data from response.

  @param[in] TisReg    TPM Handle.  
  @param[in] Fmt       Format control string.  
  @param[in] ...       The variable argument list.
 
  @retval EFI_SUCCESS  Operation completed successfully.
  @retval EFI_TIMEOUT  The register can't run into the expected status in time.

**/
EFI_STATUS
EFIAPI
TisPcExecute (
  IN      TIS_TPM_HANDLE            TisReg,
  IN      CONST CHAR8               *Fmt,
  ...
  );

/**
  Send command to TPM for execution.

  @param[in] TisReg     TPM register space base address.
  @param[in] TpmBuffer  Buffer for TPM command data.
  @param[in] DataLength TPM command data length.

  @retval EFI_SUCCESS   Operation completed successfully.
  @retval EFI_TIMEOUT   The register can't run into the expected status in time.

**/
EFI_STATUS
TisPcSend (
  IN     TIS_PC_REGISTERS_PTR       TisReg,
  IN     UINT8                      *TpmBuffer,
  IN     UINT32                     DataLength
  );

/**
  Set Legacy GPIO Level

  @param[in]  LevelRegOffset        GPIO level register Offset from GPIO Base Address.
  @param[in]  GpioNum               GPIO bit to change.
  @param[in]  HighLevel             Set or clear GPIO bit.

**/
STATIC
VOID
LegacyGpioSetLevel (
  IN CONST UINT32              LevelRegOffset,
  IN CONST UINT32              GpioNum,
  IN CONST BOOLEAN             HighLevel
  );

/**
  Initialise the TPM (TPM_INIT).
  
  Pull TPM reset low for 80us (equivalent to cold reset, Table 39
  Infineon SLB9645 Databook), then high and wait for 150ms to give
  time for TPM to stabilise (Section 4.7 Infineon SLB9645 Databook).
  Infineon SLB9645 TPM on Crosshill board driven by GPIOSUS5.

**/
VOID
TpmCommInit (
  );

/**
  Send TPM_Startup command to TPM.

  @param[in] TpmHandle              TPM handle.
  @param[in] BootMode               Boot mode.
 
  @retval TPM_RESULT                TPM return code.

**/
TPM_RESULT
TpmCommStartup (
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      EFI_BOOT_MODE             BootMode
  );

/**
  Send TPM_ContinueSelfTest command to TPM.

  @param[in] TpmHandle              TPM handle.

  @retval TPM_RESULT                TPM return code.

**/
TPM_RESULT
TpmCommContinueSelfTest (
  IN      TIS_TPM_HANDLE            TpmHandle
  );

/**
  Send TPM_GetSelfTestResult command to TPM.

  @param[in] TpmHandle             TPM handle.
 
  @retval TPM_RESULT               TPM return code.

**/
TPM_RESULT
TpmCommGetSelfTestResult (
  IN      TIS_TPM_HANDLE           TpmHandle
  );

/**
  Issue TSC_PhysicalPresence command to TPM.

  Adapted from: SecurityPkg/Tcg/TcgPei/TpmComm.c

  @param[in] TpmHandle            TPM handle.
  @param[in] PhysicalPresence     The state to set the TPM's Physical Presence flags.
  
  @retval TPM_RESULT              TPM return code.

**/
TPM_RESULT
TpmCommPhysicalPresence (
  IN      TIS_TPM_HANDLE            TpmHandle,
  IN      TPM_PHYSICAL_PRESENCE     PhysicalPresence
  );

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
  );

/**
  Send following commands in order to ENABLE+ACTIVATE TPM
  
  (i)   TSC_PhysicalPresence(physicalPresenceCMDEnable) - flag that software going to set physical presence
  (ii)  TSC_PhysicalPresence(physicalPresencePresent) - set physical presence by default
  (iii) TPM_PhysicalEnable - enable
  (iv)  TPM_PhysicalSetDeactivated(FALSE), i.e. activate
  (v)   TSC_PhysicalPresence(physicalPresenceNotPresent) - unset physical presence

  @param[in] TpmHandle                TPM handle.

  @retval TPM_RESULT                  TPM return code.

**/
TPM_RESULT
TpmCommEnableActivate (
  IN      TIS_TPM_HANDLE            TpmHandle
  );

/**
  Writes single byte data to TPM specified by MMIO address.

  Write access to TPM is MMIO or I2C (based on platform type).

  This function is implemented in TpmCommLib (TpmAccess.c)

  @param  Address The register to write.
  @param  Data    The data to write to the register.

**/
VOID
TpmWriteByte (
  IN UINTN  Address,
  IN UINT8  Data
  );

/**
  Reads single byte data from TPM specified by MMIO address.

  Read access to TPM is via MMIO or I2C (based on platform type).

  This function is implemented in TpmCommLib (TpmAccess.c)

  @param  Address of the MMIO mapped register to read.

  @return The value read.

**/
UINT8
EFIAPI
TpmReadByte (
  IN  UINTN  Address
  );

#endif  // _TPM_COMM_H_
