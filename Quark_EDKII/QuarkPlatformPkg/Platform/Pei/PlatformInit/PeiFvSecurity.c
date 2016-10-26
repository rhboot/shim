/** @file
  EFI PEI Platform Security services
  
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

#include "PeiFvSecurity.h"
#include <Library/QuarkBootRomLib.h>

EFI_PEI_NOTIFY_DESCRIPTOR mNotifyOnFvInfoSecurityList = {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiFirmwareVolumeInfoPpiGuid,
    FirmwareVolmeInfoPpiNotifySecurityCallback 
};

/**
  Callback function to perform FV security checking on a FV Info PPI.

  @param PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param NotifyDescriptor  Address of the notification descriptor data structure.
  @param Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
FirmwareVolmeInfoPpiNotifySecurityCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  )
{
  EFI_STATUS  Status;
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI      *FvInfoPpi;
  EFI_PEI_FIRMWARE_VOLUME_PPI           *FvPpi;

  FvInfoPpi = (EFI_PEI_FIRMWARE_VOLUME_INFO_PPI *)Ppi;

  //
  // Locate the corresponding FV_PPI according to founded FV's format guid
  //
  Status = PeiServicesLocatePpi (
             &FvInfoPpi->FvFormat,
             0, 
             NULL,
             (VOID**)&FvPpi
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Don't authenticate FlashFvRecovery2 since already checked at the same time
  // as the main recovery stage1 FV.
  //
  if (((UINTN) FvInfoPpi->FvInfo) == ((UINTN) FixedPcdGet32 (PcdFlashFvRecovery2Base))) {
    return EFI_SUCCESS;
  }

  //
  // Only authenticate parent Firmware Volume (child firmware volumes are covered by the parent)
  //
  if ((VOID *)FvInfoPpi->ParentFvName == NULL && (VOID *)FvInfoPpi->ParentFileName == NULL) {
    Status = PeiSecurityVerifyFv ((EFI_FIRMWARE_VOLUME_HEADER*) FvInfoPpi->FvInfo);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  Authenticates the Firmware Volume

  @param CurrentFvAddress   Pointer to the current Firmware Volume under consideration

  @retval EFI_SUCCESS       Firmware Volume is legal

**/
EFI_STATUS
PeiSecurityVerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress
  )
{
  EFI_STATUS  Status;

  //
  // Call Security library to authenticate the Firmware Volume
  //
  DEBUG ((DEBUG_INFO, "PeiSecurityVerifyFv - CurrentFvAddress=0x%8x\n", (UINT32)CurrentFvAddress));
  Status = SecurityAuthenticateImage(
             (VOID *) CurrentFvAddress,
             NULL,
             TRUE,
             NULL,
             (QUARK_AUTH_ALLOC_POOL) AllocatePool,
             (QUARK_AUTH_FREE_POOL) FreePool
             );

  return Status;
}

/**

  Entry point for the PEI Security PEIM
  Sets up a notification to perform PEI security checking

  @param  FfsHeader    Not used.
  @param  PeiServices  General purpose services available to every PEIM.

  @return EFI_SUCCESS  PEI Security notification installed successfully.
          All others: PEI Security notification failed to install.

**/
EFI_STATUS
PeiInitializeFvSecurity (
  VOID
  )
{
  EFI_STATUS  Status;
  
  Status = PeiServicesNotifyPpi (&mNotifyOnFvInfoSecurityList);
  
  return Status;
}

