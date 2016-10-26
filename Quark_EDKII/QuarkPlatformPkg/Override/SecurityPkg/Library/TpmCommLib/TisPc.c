/** @file
  Basic TIS (TPM Interface Specification) functions.

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

#include "CommonHeader.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GlobalNvsArea.h>
#include "TpmAccess.h"

/**
  Check whether TPM chip exist and update ACPI memory region.

  @param[in] TisReg  Pointer to TIS register.

  @retval    TRUE    TPM chip exists.
  @retval    FALSE   TPM chip is not found.
**/
BOOLEAN
TisPcPresenceCheck (
  IN      TIS_PC_REGISTERS_PTR      TisReg
  )
{
  UINT8                             RegRead;
  EFI_STATUS                        Status;
  EFI_GLOBAL_NVS_AREA_PROTOCOL      *GlobalNvsAreaProtocol;
  EFI_GLOBAL_NVS_AREA               *GlobalNvsAreaPtr = NULL;

  //
  // Get Global NVS Area Protocol
  //
  Status = gBS->LocateProtocol (&gEfiGlobalNvsAreaProtocolGuid, NULL, (VOID **)&GlobalNvsAreaProtocol);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "TisPcPresenceCheck(): Could not locate GlobalNvsAreaProtocol\n"));
  }
  ASSERT_EFI_ERROR (Status);
  GlobalNvsAreaPtr = GlobalNvsAreaProtocol->Area; 

  RegRead = TpmReadByte ((UINTN)&TisReg->Access);

  if ((RegRead != (UINT8)-1)) {
    GlobalNvsAreaPtr->TpmPresent = TRUE;
  } else {
    GlobalNvsAreaPtr->TpmPresent = FALSE;
  }

  return (BOOLEAN)(RegRead != (UINT8)-1);
}

/**
  Check whether the value of a TPM chip register satisfies the input BIT setting.

  @param[in]  Register     Address port of register to be checked.
  @param[in]  BitSet       Check these data bits are set.
  @param[in]  BitClear     Check these data bits are clear.
  @param[in]  TimeOut      The max wait time (unit MicroSecond) when checking register.

  @retval     EFI_SUCCESS  The register satisfies the check bit.
  @retval     EFI_TIMEOUT  The register can't run into the expected status in time.
**/
EFI_STATUS
EFIAPI
TisPcWaitRegisterBits (
  IN      UINT8                     *Register,
  IN      UINT8                     BitSet,
  IN      UINT8                     BitClear,
  IN      UINT32                    TimeOut
  )
{
  UINT8                             RegRead;
  UINT32                            WaitTime;

  for (WaitTime = 0; WaitTime < TimeOut; WaitTime += 30){
    RegRead = TpmReadByte ((UINTN)Register);
    if ((RegRead & BitSet) == BitSet && (RegRead & BitClear) == 0)
      return EFI_SUCCESS;
    MicroSecondDelay (30);
  }
  return EFI_TIMEOUT;
}

/**
  Get BurstCount by reading the burstCount field of a TIS regiger 
  in the time of default TIS_TIMEOUT_D.

  @param[in]  TisReg                Pointer to TIS register.
  @param[out] BurstCount            Pointer to a buffer to store the got BurstConut.

  @retval     EFI_SUCCESS           Get BurstCount.
  @retval     EFI_INVALID_PARAMETER TisReg is NULL or BurstCount is NULL.
  @retval     EFI_TIMEOUT           BurstCount can't be got in time.
**/
EFI_STATUS
EFIAPI
TisPcReadBurstCount (
  IN      TIS_PC_REGISTERS_PTR      TisReg,
     OUT  UINT16                    *BurstCount
  )
{
  UINT32                            WaitTime;
  UINT8                             DataByte0;
  UINT8                             DataByte1;

  if (BurstCount == NULL || TisReg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  WaitTime = 0;
  do {
    //
    // TIS_PC_REGISTERS_PTR->burstCount is UINT16, but it is not 2bytes aligned,
    // so it needs to use TpmReadByte to read two times
    //
    DataByte0   = TpmReadByte ((UINTN)&TisReg->BurstCount);
    DataByte1   = TpmReadByte ((UINTN)&TisReg->BurstCount + 1);
    *BurstCount = (UINT16)((DataByte1 << 8) + DataByte0);
    if (*BurstCount != 0) {
      return EFI_SUCCESS;
    }
    MicroSecondDelay (30);
    WaitTime += 30;
  } while (WaitTime < TIS_TIMEOUT_D);

  return EFI_TIMEOUT;
}

/**
  Set TPM chip to ready state by sending ready command TIS_PC_STS_READY 
  to Status Register in time.

  @param[in] TisReg                Pointer to TIS register.

  @retval    EFI_SUCCESS           TPM chip enters into ready state.
  @retval    EFI_INVALID_PARAMETER TisReg is NULL.
  @retval    EFI_TIMEOUT           TPM chip can't be set to ready state in time.
**/
EFI_STATUS
EFIAPI
TisPcPrepareCommand (
  IN      TIS_PC_REGISTERS_PTR      TisReg
  )
{
  EFI_STATUS                        Status;

  if (TisReg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TpmWriteByte ((UINTN)&TisReg->Status, TIS_PC_STS_READY);
  Status = TisPcWaitRegisterBits (
             &TisReg->Status,
             TIS_PC_STS_READY,
             0,
             TIS_TIMEOUT_B
             );
  return Status;
}

/**
  Get the control of TPM chip by sending requestUse command TIS_PC_ACC_RQUUSE 
  to ACCESS Register in the time of default TIS_TIMEOUT_D.

  @param[in] TisReg                Pointer to TIS register.

  @retval    EFI_SUCCESS           Get the control of TPM chip.
  @retval    EFI_INVALID_PARAMETER TisReg is NULL.
  @retval    EFI_NOT_FOUND         TPM chip doesn't exit.
  @retval    EFI_TIMEOUT           Can't get the TPM control in time.
**/
EFI_STATUS
EFIAPI
TisPcRequestUseTpm (
  IN      TIS_PC_REGISTERS_PTR      TisReg
  )
{
  EFI_STATUS                        Status;
  
  if (TisReg == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (!TisPcPresenceCheck (TisReg)) {
    return EFI_NOT_FOUND;
  }

  TpmWriteByte ((UINTN)&TisReg->Access, TIS_PC_ACC_RQUUSE);
  Status = TisPcWaitRegisterBits (
             &TisReg->Access,
             (UINT8)(TIS_PC_ACC_ACTIVE |TIS_PC_VALID),
             0,
             TIS_TIMEOUT_D
             );
  return Status;
}
