/*++

  This is QNC Smm Power Management driver

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


--*/

#include "SmmPowerManagement.h"

//
// Global variables
//
EFI_SMM_CPU_PROTOCOL                    *mSmmCpu = NULL;
EFI_GLOBAL_NVS_AREA                     *mGlobalNvsAreaPtr = NULL;
EFI_MP_SERVICES_PROTOCOL                *mMpService = NULL;
EFI_ACPI_SDT_PROTOCOL                   *mAcpiSdt = NULL;
EFI_ACPI_TABLE_PROTOCOL                 *mAcpiTable = NULL;

EFI_GUID    mS3CpuRegisterTableGuid = S3_CPU_REGISTER_TABLE_GUID;

EFI_STATUS
EFIAPI
InitializePowerManagement (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  Initializes the SMM Handler Driver

Arguments:

  ImageHandle -

  SystemTable -

Returns:

  None

--*/
{
  EFI_STATUS                                Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL             *SwDispatch;
  EFI_GLOBAL_NVS_AREA_PROTOCOL              *GlobalNvsAreaProtocol;

  //
  // Get SMM CPU protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmCpuProtocolGuid, 
                    NULL, 
                    (VOID **)&mSmmCpu
                    );
  ASSERT_EFI_ERROR (Status);
  
  //
  //  Get the Sw dispatch protocol
  //
  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID**)&SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);  
  
  //
  // Get Global NVS Area Protocol
  //
  Status = gBS->LocateProtocol (&gEfiGlobalNvsAreaProtocolGuid, NULL, (VOID **)&GlobalNvsAreaProtocol);
  ASSERT_EFI_ERROR (Status);
  mGlobalNvsAreaPtr = GlobalNvsAreaProtocol->Area; 
  
  //
  // Locate and cache PI AcpiSdt Protocol.
  //  
  Status = gBS->LocateProtocol (
                  &gEfiAcpiSdtProtocolGuid, 
                  NULL, 
                  (VOID **) &mAcpiSdt
                  );
  ASSERT_EFI_ERROR (Status);
  
  
  //
  // Locate and cache PI AcpiSdt Protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid, 
                  NULL, 
                  (VOID **) &mAcpiTable
                  );
  ASSERT_EFI_ERROR (Status);  

  
  //
  // Get MpService protocol
  //
  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&mMpService);
  ASSERT_EFI_ERROR (Status);
  //
  // Initialize power management features on processors
  //
  PpmInit();

  return Status;
}
