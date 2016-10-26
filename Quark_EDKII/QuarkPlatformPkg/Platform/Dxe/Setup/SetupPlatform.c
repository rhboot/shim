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

  SetupPlatform.c

Abstract:

  Platform Initialization Driver.

Revision History

--*/

#include "CommonHeader.h"

#include "SetupPlatform.h"
#include <Library/HobLib.h>

EFI_HANDLE            mImageHandle = NULL;

EFI_HII_DATABASE_PROTOCOL        *mHiiDataBase = NULL;
EFI_HII_CONFIG_ROUTING_PROTOCOL  *mHiiConfigRouting = NULL;

static UINT8                    mSmbusRsvdAddresses[PLATFORM_NUM_SMBUS_RSVD_ADDRESSES] = {
  SMBUS_ADDR_CH_A_1,
  SMBUS_ADDR_CK505,
  SMBUS_ADDR_THERMAL_SENSOR1,
  SMBUS_ADDR_THERMAL_SENSOR2
};

EFI_PLATFORM_POLICY_PROTOCOL    mPlatformPolicyData = {
  PLATFORM_NUM_SMBUS_RSVD_ADDRESSES,
  mSmbusRsvdAddresses
};

EFI_STATUS
DxePlatformDriverEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

  Routine Description:
    This is the standard EFI driver point for the D845GRgPlatform Driver. This
    driver is responsible for setting up any platform specific policy or
    initialization information.

  Arguments:
    ImageHandle     - Handle for the image of this driver
    SystemTable     - Pointer to the EFI System Table

  Returns:
    EFI_SUCCESS     - Policy decisions set

--*/
{
  EFI_STATUS                  Status;
  EFI_HANDLE                  Handle;
  
  S3BootScriptSaveInformationAsciiString (
    "SetupDxeEntryBegin"
    );

  mImageHandle = ImageHandle;

  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID**)&mHiiDataBase);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID**)&mHiiConfigRouting);
  ASSERT_EFI_ERROR (Status); 

  //
  // Initialize keyboard layout
  //
  Status = InitKeyboardLayout ();

  //
  // Initialize ICH registers
  //
  PlatformInitQNCRegs();

  ProducePlatformCpuData ();

  //
  // Install protocol to to allow access to this Policy.
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiPlatformPolicyProtocolGuid, &mPlatformPolicyData,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  S3BootScriptSaveInformationAsciiString (
    "SetupDxeEntryEnd"
    );
  
  return EFI_SUCCESS;
}

