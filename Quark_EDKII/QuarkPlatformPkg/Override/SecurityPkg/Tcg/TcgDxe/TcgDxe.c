/** @file  
  This module implements TCG EFI Protocol.

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

#include <PiDxe.h>
#include <IndustryStandard/Tpm12.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/SmBios.h>

#include <Guid/GlobalVariable.h>
#include <Guid/SmBios.h>
#include <Guid/HobList.h>
#include <Guid/TcgEventHob.h>
#include <Guid/EventGroup.h>
#include <Guid/EventExitBootServiceFailed.h>
#include <Protocol/DevicePath.h>
#include <Protocol/TcgService.h>
#include <Protocol/AcpiTable.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/TpmCommLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/TimerLib.h>
#include <Library/ResetSystemLib.h>

#include "TpmComm.h"

#define  EFI_TCG_LOG_AREA_SIZE        0x10000

#pragma pack (1)

typedef struct _EFI_TCG_CLIENT_ACPI_TABLE {
  EFI_ACPI_DESCRIPTION_HEADER       Header;
  UINT16                            PlatformClass;
  UINT32                            Laml;
  EFI_PHYSICAL_ADDRESS              Lasa;
} EFI_TCG_CLIENT_ACPI_TABLE;

typedef struct _EFI_TCG_SERVER_ACPI_TABLE {
  EFI_ACPI_DESCRIPTION_HEADER             Header;
  UINT16                                  PlatformClass;
  UINT16                                  Reserved0;
  UINT64                                  Laml;
  EFI_PHYSICAL_ADDRESS                    Lasa;
  UINT16                                  SpecRev;
  UINT8                                   DeviceFlags;
  UINT8                                   InterruptFlags;
  UINT8                                   Gpe;
  UINT8                                   Reserved1[3];
  UINT32                                  GlobalSysInt;
  EFI_ACPI_3_0_GENERIC_ADDRESS_STRUCTURE  BaseAddress;
  UINT32                                  Reserved2;
  EFI_ACPI_3_0_GENERIC_ADDRESS_STRUCTURE  ConfigAddress;
  UINT8                                   PciSegNum;
  UINT8                                   PciBusNum;
  UINT8                                   PciDevNum;
  UINT8                                   PciFuncNum;
} EFI_TCG_SERVER_ACPI_TABLE;

#pragma pack ()

#define TCG_DXE_DATA_FROM_THIS(this)  \
  BASE_CR (this, TCG_DXE_DATA, TcgProtocol)

typedef struct _TCG_DXE_DATA {
  EFI_TCG_PROTOCOL                  TcgProtocol;
  TCG_EFI_BOOT_SERVICE_CAPABILITY   BsCap;
  EFI_TCG_CLIENT_ACPI_TABLE         *TcgClientAcpiTable;
  EFI_TCG_SERVER_ACPI_TABLE         *TcgServerAcpiTable;
  UINTN                             EventLogSize;
  UINT8                             *LastEvent;
  TIS_TPM_HANDLE                    TpmHandle;
} TCG_DXE_DATA;



EFI_TCG_CLIENT_ACPI_TABLE           mTcgClientAcpiTemplate = {
  {
    EFI_ACPI_3_0_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE,
    sizeof (mTcgClientAcpiTemplate),
    0x02                      //Revision
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in in production
    //
  },
  0,                          // 0 for PC Client Platform Class
  0,                          // Log Area Max Length
  (EFI_PHYSICAL_ADDRESS) (SIZE_4GB - 1)  // Log Area Start Address
};

//
// The following EFI_TCG_SERVER_ACPI_TABLE default setting is just one example,
// the TPM device connectes to LPC, and also defined the ACPI _UID as 0xFF,
// this _UID can be changed and should match with the _UID setting of the TPM 
// ACPI device object  
//
EFI_TCG_SERVER_ACPI_TABLE           mTcgServerAcpiTemplate = {
  {
    EFI_ACPI_3_0_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE,
    sizeof (mTcgServerAcpiTemplate),
    0x02                      //Revision
    //
    // Compiler initializes the remaining bytes to 0
    // These fields should be filled in in production
    //
  },
  1,                          // 1 for Server Platform Class
  0,                          // Reserved
  0,                          // Log Area Max Length
  (EFI_PHYSICAL_ADDRESS) (SIZE_4GB - 1), // Log Area Start Address
  0x0100,                     // TCG Specification revision 1.0
  2,                          // Device Flags
  0,                          // Interrupt Flags
  0,                          // GPE
  {0},                        // Reserved 3 bytes
  0,                          // Global System Interrupt
  {
    EFI_ACPI_3_0_SYSTEM_MEMORY,
    0,
    0,
    EFI_ACPI_3_0_BYTE,
    TPM_BASE_ADDRESS          // Base Address
  },
  0,                          // Reserved
  {0},                        // Configuration Address
  0xFF,                       // ACPI _UID value of the device, can be changed for different platforms
  0,                          // ACPI _UID value of the device, can be changed for different platforms
  0,                          // ACPI _UID value of the device, can be changed for different platforms
  0                           // ACPI _UID value of the device, can be changed for different platforms
};

UINTN  mBootAttempts  = 0;
CHAR16 mBootVarName[] = L"BootOrder";

/**
  This service provides EFI protocol capability information, state information 
  about the TPM, and Event Log state information.

  @param[in]  This               Indicates the calling context
  @param[out] ProtocolCapability The callee allocates memory for a TCG_BOOT_SERVICE_CAPABILITY 
                                 structure and fills in the fields with the EFI protocol 
                                 capability information and the current TPM state information.
  @param[out] TCGFeatureFlags    This is a pointer to the feature flags. No feature 
                                 flags are currently defined so this parameter 
                                 MUST be set to 0. However, in the future, 
                                 feature flags may be defined that, for example, 
                                 enable hash algorithm agility.
  @param[out] EventLogLocation   This is a pointer to the address of the event log in memory.
  @param[out] EventLogLastEntry  If the Event Log contains more than one entry, 
                                 this is a pointer to the address of the start of 
                                 the last entry in the event log in memory. 

  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_INVALID_PARAMETER  ProtocolCapability does not match TCG capability.
  
**/
EFI_STATUS
EFIAPI
TcgDxeStatusCheck (
  IN      EFI_TCG_PROTOCOL                 *This,
  OUT     TCG_EFI_BOOT_SERVICE_CAPABILITY  *ProtocolCapability,
  OUT     UINT32                           *TCGFeatureFlags,
  OUT     EFI_PHYSICAL_ADDRESS             *EventLogLocation,
  OUT     EFI_PHYSICAL_ADDRESS             *EventLogLastEntry
  )
{
  TCG_DXE_DATA                      *TcgData;

  TcgData = TCG_DXE_DATA_FROM_THIS (This);

  if (ProtocolCapability != NULL) {
    *ProtocolCapability = TcgData->BsCap;
  }

  if (TCGFeatureFlags != NULL) {
    *TCGFeatureFlags = 0;
  }

  if (EventLogLocation != NULL) {
    if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
      *EventLogLocation = TcgData->TcgClientAcpiTable->Lasa;
    } else {
      *EventLogLocation = TcgData->TcgServerAcpiTable->Lasa;
    }
  }

  if (EventLogLastEntry != NULL) {
    if (TcgData->BsCap.TPMDeactivatedFlag) {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)0;
    } else {
      *EventLogLastEntry = (EFI_PHYSICAL_ADDRESS)(UINTN)TcgData->LastEvent;
    }
  }

  return EFI_SUCCESS;
}

/**
  This service abstracts the capability to do a hash operation on a data buffer.
  
  @param[in]      This             Indicates the calling context
  @param[in]      HashData         Pointer to the data buffer to be hashed
  @param[in]      HashDataLen      Length of the data buffer to be hashed
  @param[in]      AlgorithmId      Identification of the Algorithm to use for the hashing operation
  @param[in, out] HashedDataLen    Resultant length of the hashed data
  @param[in, out] HashedDataResult Resultant buffer of the hashed data  
  
  @retval EFI_SUCCESS              Operation completed successfully.
  @retval EFI_INVALID_PARAMETER    HashDataLen is NULL.
  @retval EFI_INVALID_PARAMETER    HashDataLenResult is NULL.
  @retval EFI_OUT_OF_RESOURCES     Cannot allocate buffer of size *HashedDataLen.
  @retval EFI_UNSUPPORTED          AlgorithmId not supported.
  @retval EFI_BUFFER_TOO_SMALL     *HashedDataLen < sizeof (TCG_DIGEST).
  
**/
EFI_STATUS
EFIAPI
TcgDxeHashAll (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      UINT8                     *HashData,
  IN      UINT64                    HashDataLen,
  IN      TCG_ALGORITHM_ID          AlgorithmId,
  IN OUT  UINT64                    *HashedDataLen,
  IN OUT  UINT8                     **HashedDataResult
  )
{
  if (HashedDataLen == NULL || HashedDataResult == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (AlgorithmId) {
    case TPM_ALG_SHA:
      if (*HashedDataLen == 0) {
        *HashedDataLen    = sizeof (TPM_DIGEST);
        *HashedDataResult = AllocatePool ((UINTN) *HashedDataLen);
        if (*HashedDataResult == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }
      }

      if (*HashedDataLen < sizeof (TPM_DIGEST)) {
        *HashedDataLen = sizeof (TPM_DIGEST);
        return EFI_BUFFER_TOO_SMALL;
      }
      *HashedDataLen = sizeof (TPM_DIGEST);

	  if (*HashedDataResult == NULL) {
	  	*HashedDataResult = AllocatePool ((UINTN) *HashedDataLen);
	  } 

      return TpmCommHashAll (
               HashData,
               (UINTN) HashDataLen,
               (TPM_DIGEST*)*HashedDataResult
               );
    default:
      return EFI_UNSUPPORTED;
  }
}

/**
  Add a new entry to the Event Log.

  @param[in] TcgData       TCG_DXE_DATA structure.
  @param[in] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.  
  @param[in] NewEventData  Pointer to the new event data.  
  
  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.

**/
EFI_STATUS
EFIAPI
TcgDxeLogEventI (
  IN      TCG_DXE_DATA              *TcgData,
  IN      TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
    TcgData->LastEvent = (UINT8*)(UINTN)TcgData->TcgClientAcpiTable->Lasa;
    return TpmCommLogEvent (
             &TcgData->LastEvent,
             &TcgData->EventLogSize,
             (UINTN)TcgData->TcgClientAcpiTable->Laml,
             NewEventHdr,
             NewEventData
             );
  } else {
    TcgData->LastEvent = (UINT8*)(UINTN)TcgData->TcgServerAcpiTable->Lasa;
    return TpmCommLogEvent (
             &TcgData->LastEvent,
             &TcgData->EventLogSize,
             (UINTN)TcgData->TcgServerAcpiTable->Laml,
             NewEventHdr,
             NewEventData
             );
  }
}

/**
  This service abstracts the capability to add an entry to the Event Log.

  @param[in]      This           Indicates the calling context
  @param[in]      TCGLogData     Pointer to the start of the data buffer containing 
                                 the TCG_PCR_EVENT data structure. All fields in 
                                 this structure are properly filled by the caller.
  @param[in, out] EventNumber    The event number of the event just logged
  @param[in]      Flags          Indicate additional flags. Only one flag has been 
                                 defined at this time, which is 0x01 and means the 
                                 extend operation should not be performed. All 
                                 other bits are reserved. 
 
  @retval EFI_SUCCESS            Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory in the event log to complete this action.
  
**/
EFI_STATUS
EFIAPI
TcgDxeLogEvent (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      TCG_PCR_EVENT             *TCGLogData,
  IN OUT  UINT32                    *EventNumber,
  IN      UINT32                    Flags
  )
{
  TCG_DXE_DATA  *TcgData;

  TcgData = TCG_DXE_DATA_FROM_THIS (This);
  
  if (TcgData->BsCap.TPMDeactivatedFlag) {
    return EFI_DEVICE_ERROR;
  }
  return TcgDxeLogEventI (
           TcgData,
           (TCG_PCR_EVENT_HDR*)TCGLogData,
           TCGLogData->Event
           );
}

/**
  This service is a proxy for commands to the TPM.

  @param[in]  This                        Indicates the calling context
  @param[in]  TpmInputParameterBlockSize  Size of the TPM input parameter block
  @param[in]  TpmInputParameterBlock      Pointer to the TPM input parameter block
  @param[in]  TpmOutputParameterBlockSize Size of the TPM output parameter block
  @param[in]  TpmOutputParameterBlock     Pointer to the TPM output parameter block

  @retval     EFI_SUCCESS                 Operation completed successfully.
  @retval     EFI_INVALID_PARAMETER       Invalid ordinal.
  @retval     EFI_UNSUPPORTED             Current Task Priority Level  >= EFI_TPL_CALLBACK.
  @retval     EFI_TIMEOUT                 The TIS timed-out.
  
**/
EFI_STATUS
EFIAPI
TcgDxePassThroughToTpm (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      UINT32                    TpmInputParameterBlockSize,
  IN      UINT8                     *TpmInputParameterBlock,
  IN      UINT32                    TpmOutputParameterBlockSize,
  IN      UINT8                     *TpmOutputParameterBlock
  )
{
  TCG_DXE_DATA                      *TcgData;

  TcgData = TCG_DXE_DATA_FROM_THIS (This);

  return TisPcExecute (
           TcgData->TpmHandle,
           "%r%/%r",
           TpmInputParameterBlock,
           (UINTN) TpmInputParameterBlockSize,
           TpmOutputParameterBlock,
           (UINTN) TpmOutputParameterBlockSize
           );
}

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and add an entry to the Event Log.

  @param[in]      TcgData       TCG_DXE_DATA structure.
  @param[in]      HashData      Physical address of the start of the data buffer 
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData
  @param[in, out] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.  
  @param[in]      NewEventData  Pointer to the new event data.  

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
TcgDxeHashLogExtendEventI (
  IN      TCG_DXE_DATA              *TcgData,
  IN      UINT8                     *HashData,
  IN      UINT64                    HashDataLen,
  IN OUT  TCG_PCR_EVENT_HDR         *NewEventHdr,
  IN      UINT8                     *NewEventData
  )
{
  EFI_STATUS                        Status;

  if (HashDataLen > 0) {
    Status = TpmCommHashAll (
               HashData,
               (UINTN) HashDataLen,
               &NewEventHdr->Digest
               );
    ASSERT_EFI_ERROR (Status);
  }

  Status = TpmCommExtend (
             TcgData->TpmHandle,
             &NewEventHdr->Digest,
             NewEventHdr->PCRIndex,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    Status = TcgDxeLogEventI (TcgData, NewEventHdr, NewEventData);
  }

  return Status;
}

/**
  This service abstracts the capability to do a hash operation on a data buffer,
  extend a specific TPM PCR with the hash result, and add an entry to the Event Log

  @param[in]      This               Indicates the calling context
  @param[in]      HashData           Physical address of the start of the data buffer 
                                     to be hashed, extended, and logged.
  @param[in]      HashDataLen        The length, in bytes, of the buffer referenced by HashData
  @param[in]      AlgorithmId        Identification of the Algorithm to use for the hashing operation
  @param[in, out] TCGLogData         The physical address of the start of the data 
                                     buffer containing the TCG_PCR_EVENT data structure.
  @param[in, out] EventNumber        The event number of the event just logged.
  @param[out]     EventLogLastEntry  Physical address of the first byte of the entry 
                                     just placed in the Event Log. If the Event Log was 
                                     empty when this function was called then this physical 
                                     address will be the same as the physical address of 
                                     the start of the Event Log.

  @retval EFI_SUCCESS                Operation completed successfully.
  @retval EFI_UNSUPPORTED            AlgorithmId != TPM_ALG_SHA.
  @retval EFI_UNSUPPORTED            Current TPL >= EFI_TPL_CALLBACK.
  @retval EFI_DEVICE_ERROR           The command was unsuccessful.
  
**/
EFI_STATUS
EFIAPI
TcgDxeHashLogExtendEvent (
  IN      EFI_TCG_PROTOCOL          *This,
  IN      EFI_PHYSICAL_ADDRESS      HashData,
  IN      UINT64                    HashDataLen,
  IN      TPM_ALGORITHM_ID          AlgorithmId,
  IN OUT  TCG_PCR_EVENT             *TCGLogData,
  IN OUT  UINT32                    *EventNumber,
     OUT  EFI_PHYSICAL_ADDRESS      *EventLogLastEntry
  )
{
  TCG_DXE_DATA  *TcgData;

  TcgData = TCG_DXE_DATA_FROM_THIS (This);
  
  if (TcgData->BsCap.TPMDeactivatedFlag) {
    return EFI_DEVICE_ERROR;
  }
    
  if (AlgorithmId != TPM_ALG_SHA) {
    return EFI_UNSUPPORTED;
  }

  return TcgDxeHashLogExtendEventI (
           TcgData,
           (UINT8 *) (UINTN) HashData,
           HashDataLen,
           (TCG_PCR_EVENT_HDR*)TCGLogData,
           TCGLogData->Event
           );
}

TCG_DXE_DATA                 mTcgDxeData = {
  {
    TcgDxeStatusCheck,
    TcgDxeHashAll,
    TcgDxeLogEvent,
    TcgDxePassThroughToTpm,
    TcgDxeHashLogExtendEvent
  },
  {
    sizeof (mTcgDxeData.BsCap),
    { 1, 2, 0, 0 },
    { 1, 2, 0, 0 },
    1,
    TRUE,
    FALSE
  },
  &mTcgClientAcpiTemplate,
  &mTcgServerAcpiTemplate,
  0,
  NULL,
  NULL
};

/**
  Initialize the Event Log and log events passed from the PEI phase.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.

**/
EFI_STATUS
EFIAPI
SetupEventLog (
  VOID
  )
{
  EFI_STATUS            Status;
  TCG_PCR_EVENT         *TcgEvent;
  EFI_PEI_HOB_POINTERS  GuidHob;
  EFI_PHYSICAL_ADDRESS  Lasa;
  
  if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
    Lasa = mTcgClientAcpiTemplate.Lasa;
  
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    EFI_SIZE_TO_PAGES (EFI_TCG_LOG_AREA_SIZE),
                    &Lasa
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    mTcgClientAcpiTemplate.Lasa = Lasa;
    //
    // To initialize them as 0xFF is recommended 
    // because the OS can know the last entry for that.
    //
    SetMem ((VOID *)(UINTN)mTcgClientAcpiTemplate.Lasa, EFI_TCG_LOG_AREA_SIZE, 0xFF);
    mTcgClientAcpiTemplate.Laml = EFI_TCG_LOG_AREA_SIZE;
  
  } else {
    Lasa = mTcgServerAcpiTemplate.Lasa;
  
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiACPIMemoryNVS,
                    EFI_SIZE_TO_PAGES (EFI_TCG_LOG_AREA_SIZE),
                    &Lasa
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    mTcgServerAcpiTemplate.Lasa = Lasa;
    //
    // To initialize them as 0xFF is recommended 
    // because the OS can know the last entry for that.
    //
    SetMem ((VOID *)(UINTN)mTcgServerAcpiTemplate.Lasa, EFI_TCG_LOG_AREA_SIZE, 0xFF);
    mTcgServerAcpiTemplate.Laml = EFI_TCG_LOG_AREA_SIZE;
  }

  GuidHob.Raw = GetHobList ();
  while (!EFI_ERROR (Status) && 
         (GuidHob.Raw = GetNextGuidHob (&gTcgEventEntryHobGuid, GuidHob.Raw)) != NULL) {
    TcgEvent    = GET_GUID_HOB_DATA (GuidHob.Guid);
    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
    Status = TcgDxeLogEventI (
               &mTcgDxeData,
               (TCG_PCR_EVENT_HDR*)TcgEvent,
               TcgEvent->Event
               );
  }

  return Status;
}

/**
  Install TCG ACPI Table when ACPI Table Protocol is available.

  A system's firmware uses an ACPI table to identify the system's TCG capabilities 
  to the Post-Boot environment. The information in this ACPI table is not guaranteed 
  to be valid until the Host Platform transitions from pre-boot state to post-boot state.  

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
InstallAcpiTable (
  IN EFI_EVENT                      Event,
  IN VOID*                          Context
  )
{
  UINTN                             TableKey;
  EFI_STATUS                        Status;
  EFI_ACPI_TABLE_PROTOCOL           *AcpiTable;
  UINT8                             Checksum;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  if (EFI_ERROR (Status)) {
    return;
  }

  if (PcdGet8 (PcdTpmPlatformClass) == TCG_PLATFORM_TYPE_CLIENT) {
 
    //
    // The ACPI table must be checksumed before calling the InstallAcpiTable() 
    // service of the ACPI table protocol to install it.
    //
    Checksum = CalculateCheckSum8 ((UINT8 *)&mTcgClientAcpiTemplate, sizeof (mTcgClientAcpiTemplate));
    mTcgClientAcpiTemplate.Header.Checksum = Checksum;

    Status = AcpiTable->InstallAcpiTable (
                            AcpiTable,
                            &mTcgClientAcpiTemplate,
                            sizeof (mTcgClientAcpiTemplate),
                            &TableKey
                            );
  } else {

    //
    // The ACPI table must be checksumed before calling the InstallAcpiTable() 
    // service of the ACPI table protocol to install it.
    //
    Checksum = CalculateCheckSum8 ((UINT8 *)&mTcgServerAcpiTemplate, sizeof (mTcgServerAcpiTemplate));
    mTcgServerAcpiTemplate.Header.Checksum = Checksum;

    Status = AcpiTable->InstallAcpiTable (
                            AcpiTable,
                            &mTcgServerAcpiTemplate,
                            sizeof (mTcgServerAcpiTemplate),
                            &TableKey
                            );
  }
  ASSERT_EFI_ERROR (Status);
}


/**
  Get TPM Deactivated state.

  @param[out] TPMDeactivatedFlag   Returns TPM Deactivated state.  

  @retval EFI_SUCCESS              Operation completed successfully.
  @retval EFI_DEVICE_ERROR         The operation was unsuccessful.

**/
EFI_STATUS
GetTpmStatus (
     OUT  BOOLEAN                   *TPMDeactivatedFlag
  )
{
  EFI_STATUS                        Status;
  TPM_STCLEAR_FLAGS                 VFlags;

  Status = TpmCommGetFlags (
             mTcgDxeData.TpmHandle,
             TPM_CAP_FLAG_VOLATILE,
             &VFlags,
             sizeof (VFlags)
             );
  if (!EFI_ERROR (Status)) {
    *TPMDeactivatedFlag = VFlags.deactivated;
  }

  return Status;
}

/**
  The driver's entry point.

  If TPM is present and available, TPM is Enabled+Activated (if not
  activated already) by default (without any user confirmation).
  A reset is performed once after successful activation of TPM to ensure
  TPM state change takes effect. Also EFI TCG Protocols are published.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
DriverEntry (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS                        Status;
  EFI_BOOT_MODE                     BootMode;
  VOID                              *Registration;
  TPM_RESULT                        TpmResponse;

  Status = EFI_SUCCESS;

  //
  // If TPM not supported by default in build then return.
  //
  if (PcdGetBool (PcdHideTpmSupport) && PcdGetBool (PcdHideTpm)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check if TPM is present and issue request to use TPM.
  //
  mTcgDxeData.TpmHandle = (TIS_TPM_HANDLE)(UINTN)TPM_BASE_ADDRESS;
  Status = TisPcRequestUseTpm (mTcgDxeData.TpmHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_WARN, "TPM not detected!\n"));
    return Status;
  }

  //
  // Check current boot mode.
  //
  BootMode = GetBootModeHob();

  //
  // Issue Startup command to TPM.
  //
  TpmResponse = TpmCommStartup (mTcgDxeData.TpmHandle, BootMode);
  if (TpmResponse != TPM_SUCCESS) {
    DEBUG ((EFI_D_ERROR, "Could not start the TPM!, TPM Response = %0x (hex)\n", TpmResponse));
    Status = EFI_DEVICE_ERROR;
    return Status;
  }

  //
  // Issue SelfTest command to TPM (selftest is optional on S3 path, skip it to save S3 time).
  //
  if (BootMode != BOOT_ON_S3_RESUME) {
    TpmResponse = TpmCommContinueSelfTest (mTcgDxeData.TpmHandle);
    if (TpmResponse != TPM_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "TPM Continue Selftests command failed, TPM Response = %0x (hex)\n", TpmResponse));
      Status = EFI_DEVICE_ERROR;
      return Status;
    }

    //
    // Check that selftests have completed successfully.
    // Wait 2ms to ensure selftests finished (Infineon SLB9645 spec. states ~1.2ms max).
    //
    MicroSecondDelay (2000);
    TpmResponse = TpmCommGetSelfTestResult (mTcgDxeData.TpmHandle);
    if (TpmResponse != TPM_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "TPM Selftests failed, TPM Response = %0x (hex)\n", TpmResponse));
      Status = EFI_DEVICE_ERROR;
      return Status;
    }
  }

  //
  // Get status of TPM Deactivated Flag.
  //
  Status = GetTpmStatus (&mTcgDxeData.BsCap.TPMDeactivatedFlag);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      EFI_D_ERROR,
      "Line %d in file " __FILE__ ":\n    "
      "DriverEntry: TPM not working properly\n",
      __LINE__
      ));
    return Status;
  }

  //
  // If TPM not activated already, enable+activate TPM by default.
  //
  if (mTcgDxeData.BsCap.TPMDeactivatedFlag) {
    TpmResponse = TpmCommEnableActivate (mTcgDxeData.TpmHandle);
    if (TpmResponse != TPM_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "TPM Enable+Activate command failed, TPM Response = %0x\n", TpmResponse));
      Status = EFI_DEVICE_ERROR;
      return Status;
    } else {
      DEBUG ((EFI_D_INFO, "Rebooting system due to TPM state change: Enabled+Activated\n"));
      ResetCold();
    }
  }

  //
  // Install TCG DXE protocols
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiTcgProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mTcgDxeData.TcgProtocol
                  );

  //
  // Install ACPI Table
  //
  EfiCreateProtocolNotifyEvent (&gEfiAcpiTableProtocolGuid, TPL_CALLBACK, InstallAcpiTable, NULL, &Registration);
    
  if (!EFI_ERROR (Status) && !mTcgDxeData.BsCap.TPMDeactivatedFlag) {
    //
    // Setup the log area and copy event log from hob list to it
    //
    Status = SetupEventLog ();
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR, "TPM setup the event log failed, Status = %r\n", Status));
    }
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
