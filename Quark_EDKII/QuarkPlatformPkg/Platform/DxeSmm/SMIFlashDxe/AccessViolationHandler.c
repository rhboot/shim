/*++

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


Module Name:

  AccessViolationHandler.c

Abstract:

  SPI BIOS Flash access violation handler routines.

--*/

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/BaseLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/SmmVariable.h>
#include <Guid/GlobalVariable.h>
#include <QNCAccessLib.h>
#include <Library/IntelQNCLib.h>
#include "AccessViolationHandler.h"

//
// Variables private to this source module.
//
STATIC EFI_SMM_VARIABLE_PROTOCOL           *mSmmVariable;
STATIC EFI_HANDLE                          mSmiHandle;
//
// Routines private to this source module.
//

STATIC
EFI_STATUS
IncrementAccessViolationCount (
  VOID
  )
{
  EFI_STATUS                        Status;
  UINT32                            AccessCount;
  UINT32                            Attr;
  UINT32                            DataSize;

  if (mSmmVariable == NULL) {
    return EFI_SUCCESS;
  }
  DataSize = sizeof (AccessCount);
  Status = mSmmVariable->SmmGetVariable (
                           SPI_ACCESS_VIOLATION_COUNT_VAR_NAME,
                           &gEfiGlobalVariableGuid,
                           &Attr,
                           &DataSize,
                           (VOID *) &AccessCount
                           );

  if (EFI_ERROR (Status)) {
    AccessCount = 0;
    Attr = SPI_ACCESS_VIOLATION_COUNT_VAR_ATTR;
    DataSize = sizeof (AccessCount);
  } else {
    AccessCount++;
  }

  Status = mSmmVariable->SmmSetVariable (
                           SPI_ACCESS_VIOLATION_COUNT_VAR_NAME,
                           &gEfiGlobalVariableGuid,
                           Attr,
                           DataSize,
                           (VOID *) &AccessCount
                           );

  return Status;
}

STATIC
EFI_STATUS
VariableNotificationFunction (
  IN CONST EFI_GUID                       *Protocol,
  IN VOID                                 *Interface,
  IN EFI_HANDLE                           Handle
  )
{
  EFI_STATUS                        Status;
  Status = gSmst->SmmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, &mSmmVariable);
  return EFI_SUCCESS;
}

//
// Routines exported by this source module.
//


/**
  SMI Handler to trap SPI Flash access violations.

  @param  DispatchHandle   The unique handle assigned to this handler by SmiHandlerRegister()
  @param  Context          Points to an optional handler context which was specified when the
                           handler was registered..
  @param  CommBuffer       A pointer to a collection of data in memory that will
                           be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize   The size of the CommBuffer.

  @retval EFI_SUCCESS           Calculate the total size for all FV images stored in Capsule image.

**/
EFI_STATUS
EFIAPI
AccessViolationHandlerCallback (
  IN        EFI_HANDLE  DispatchHandle,
  IN CONST  VOID        *Context,
  IN OUT    VOID        *CommBuffer,
  IN OUT    UINTN       *CommBufferSize
  )
{
  EFI_STATUS                        Status;
  UINT32                            BcValue;

  Status = EFI_WARN_INTERRUPT_SOURCE_PENDING; 
  BcValue = LpcPciCfg32 (R_QNC_LPC_BIOS_CNTL);

  //
  // B_QNC_LPC_BIOS_CNTL_BLE and B_QNC_LPC_BIOS_CNTL_BIOSWE
  // must be set to indicates condiitions met.
  //
  if ((BcValue & B_QNC_LPC_BIOS_CNTL_BLE) != 0) {
    if ((BcValue & B_QNC_LPC_BIOS_CNTL_BIOSWE) != 0) {

      IncrementAccessViolationCount ();

      //
      // Clear B_QNC_LPC_BIOS_CNTL_BIOSWE to trap
      // next edge.
      //
      LpcPciCfg32 (R_QNC_LPC_BIOS_CNTL) =
        BcValue & (~B_QNC_LPC_BIOS_CNTL_BIOSWE);
      Status = EFI_SUCCESS;
    }
  }
  return Status;
}

/**
  Installs SMI handler to trap Flash Access Violations.

  @retval EFI_SUCCESS           Function success.

**/
EFI_STATUS
EFIAPI
AccessViolationHandlerInit (
  VOID
  )
{
  EFI_STATUS                        Status;
  VOID                              *RegistrationVar;
  UINT32                            Attr;
  UINT32                            DataSize;
  UINT32                            AccessCount;

  //
  // Register protocol notify for SMM variable services.
  //

  Status = gSmst->SmmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, &mSmmVariable);
  if (EFI_ERROR(Status)) {
    mSmmVariable = NULL;
    RegistrationVar = NULL;
    Status = gSmst->SmmRegisterProtocolNotify (
                      &gEfiSmmVariableProtocolGuid,
                      VariableNotificationFunction,
                      &RegistrationVar
                      );
  }

  //
  // Smm register SMI Access violation handler.
  //

  Status = gSmst->SmiHandlerRegister (AccessViolationHandlerCallback, NULL, &mSmiHandle);
  ASSERT_EFI_ERROR (Status);

  //
  // Check if there was a SPI Fash write on previous boot
  //
  Attr = SPI_ACCESS_VIOLATION_COUNT_VAR_ATTR;
  DataSize = sizeof(UINT32);
  Status = mSmmVariable->SmmGetVariable (
    SPI_ACCESS_VIOLATION_COUNT_VAR_NAME,
    &gEfiGlobalVariableGuid,
    &Attr,
    &DataSize,
    (VOID *) &AccessCount
    );
  //    
  // Report error and clear count for previous boot, Initialize if not set 
  //
  if ((EFI_ERROR(Status) || AccessCount != 0)) {
    if(!EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "Access Violation Count: %x\n", AccessCount));
    }
    AccessCount = 0;
    Status = mSmmVariable->SmmSetVariable (
            SPI_ACCESS_VIOLATION_COUNT_VAR_NAME,
            &gEfiGlobalVariableGuid,
            Attr,
            DataSize,
            (VOID *) &AccessCount
            );
  }

  return Status;
}
