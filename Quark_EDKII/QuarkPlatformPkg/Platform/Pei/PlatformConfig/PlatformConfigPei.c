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

Module Name:
  PlatformConfigPei.c

Abstract:
  Principle source module for Clanton Peak platform config PEIM driver.

--*/

#include "PlatformConfigPei.h"

//
// Global variables.
//

EFI_PEI_NOTIFY_DESCRIPTOR mPcdNotifyList[1] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gPcdPpiGuid,
    PlatformConfigPcdNotifyCallback
  }
};

//
// Private routines to this source module.
//

STATIC
VOID
EFIAPI
LegacySpiProtect (
  VOID
  )
{
  UINT32                            RegVal;

  RegVal = FixedPcdGet32 (PcdLegacyProtectedBIOSRange0Pei);
  if (RegVal != 0) {

    PlatformWriteFirstFreeSpiProtect (
      RegVal,
      0,
      0
      );

  }
  RegVal = FixedPcdGet32 (PcdLegacyProtectedBIOSRange1Pei);
  if (RegVal != 0) {
    PlatformWriteFirstFreeSpiProtect (
      RegVal,
      0,
      0
      );
  }
  RegVal = FixedPcdGet32 (PcdLegacyProtectedBIOSRange2Pei);
  if (RegVal != 0) {
    PlatformWriteFirstFreeSpiProtect (
      RegVal,
      0,
      0
      );
  }

  //
  // Make legacy SPI READ/WRITE enabled if not a secure build
  //
  if(FeaturePcdGet (PcdEnableSecureLock)) {
    LpcPciCfg32And (R_QNC_LPC_BIOS_CNTL, ~B_QNC_LPC_BIOS_CNTL_BIOSWE);
  } else {
    LpcPciCfg32Or (R_QNC_LPC_BIOS_CNTL, B_QNC_LPC_BIOS_CNTL_BIOSWE);
  }

}


/** Set Dynamic PCD values.

**/
STATIC
VOID
EFIAPI
UpdateDynamicPcds (
  VOID
  )
{
  MFH_LIB_FINDCONTEXT               FindContext;
  MFH_FLASH_ITEM                    *FlashItem;
  UINT32                            SecHdrSize;
  UINT32                            Temp32;
  UINT32                            SramImageIndex;
  QUARK_EDKII_STAGE1_HEADER         *SramEdk2ImageHeader;
  QUARK_EDKII_STAGE1_HEADER         *FlashEntryEdk2ImageHeader;
  MFH_LIB_FINDCONTEXT               MfhFindContext;
  UINT32                            Stage1Base;
  UINT32                            Stage1Len;

  SecHdrSize = FixedPcdGet32 (PcdFvSecurityHeaderSize);

  //
  // Init stage1 locals with fixed recovery image constants.
  //
  Stage1Base = FixedPcdGet32 (PcdFlashFvFixedStage1AreaBase);
  Stage1Len = FixedPcdGet32 (PcdFlashFvFixedStage1AreaSize);

  //
  // Setup PCDs determined from MFH if not running in recovery.
  //
  if (!PlatformIsBootWithRecoveryStage1()) {
    //
    // If found in SPI MFH override Stage1Base & Len with MFH values.
    //
    SramEdk2ImageHeader = (QUARK_EDKII_STAGE1_HEADER *) (FixedPcdGet32 (PcdEsramStage1Base) + SecHdrSize);
    SramImageIndex = (UINT32)  SramEdk2ImageHeader->ImageIndex;
    FlashItem = MfhLibFindFirstWithFilter (
                  MFH_FIND_ALL_STAGE1_FILTER,
                  FALSE,
                  &MfhFindContext
                  );
    while (FlashItem != NULL) {
      FlashEntryEdk2ImageHeader = (QUARK_EDKII_STAGE1_HEADER *) (FlashItem->FlashAddress + SecHdrSize);
      if (SramImageIndex == FlashEntryEdk2ImageHeader->ImageIndex) {
        Stage1Base = FlashItem->FlashAddress;
        Stage1Len = FlashItem->LengthBytes;
        break;
      }
      FlashItem = MfhLibFindNextWithFilter (
                    MFH_FIND_ALL_STAGE1_FILTER,
                    &MfhFindContext
                    );
    }

    Temp32 = PcdSet32 (PcdFlashFvRecoveryBase, (Stage1Base + SecHdrSize));
    ASSERT (Temp32 == (Stage1Base + SecHdrSize));

    Temp32 = PcdSet32 (PcdFlashFvRecoverySize, (Stage1Len - SecHdrSize));
    ASSERT (Temp32 == (Stage1Len - SecHdrSize));

    //
    // Set FvMain base and length PCDs from SPI MFH database.
    //
    FlashItem = MfhLibFindFirstWithFilter (
                  MFH_FIND_ALL_STAGE2_FILTER,
                  FALSE,
                  &FindContext
                  );

    if (FlashItem != NULL) {
      Temp32 = PcdSet32 (PcdFlashFvMainBase, (FlashItem->FlashAddress + SecHdrSize));
      ASSERT (Temp32 == (FlashItem->FlashAddress + SecHdrSize));

      Temp32 = PcdSet32 (PcdFlashFvMainSize, (FlashItem->LengthBytes - SecHdrSize));
      ASSERT (Temp32 == (FlashItem->LengthBytes - SecHdrSize));
    }

    //
    // Set Payload base and length PCDs from SPI MFH database.
    //
    FlashItem = MfhLibFindFirstWithFilter (
                  MFH_FIND_ALL_BOOTLOADER_FILTER,
                  FALSE,
                  &FindContext
                  );

    if (FlashItem != NULL) {
      Temp32 = PcdSet32 (PcdFlashFvPayloadBase, (FlashItem->FlashAddress + SecHdrSize));
      ASSERT (Temp32 == (FlashItem->FlashAddress + SecHdrSize));

      Temp32 = PcdSet32 (PcdFlashFvPayloadSize, FlashItem->LengthBytes);
      ASSERT (Temp32 == FlashItem->LengthBytes);
    }
  }
}

//
// Public routines exported by this source module.
//

/** Callback to indicate Pcd services available.

  Config operations to do when PCD services available.
    1) Call PeiQNCPreMemInit library function.
    2) Protect Legacy SPI Flash regions.
    3) Update Dynamic PCDs.

  @param[in]       PeiServices       General purpose services available to every PEIM.
  @param[in]       NotifyDescriptor  Information about the notify event.
  @param[in]       Ppi               The notify context.

  @retval EFI_SUCCESS                Platform config success.
*/
EFI_STATUS
EFIAPI
PlatformConfigPcdNotifyCallback (
  IN EFI_PEI_SERVICES                     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR            *NotifyDescriptor,
  IN VOID                                 *Ppi
  )
{
  //
  // Do SOC Init Pre memory init.
  //
  PeiQNCPreMemInit ();

  //
  // Protect areas specified by PCDs.
  //
  LegacySpiProtect ();

  //
  // Early update of Dynamic PCDs given run time info.
  //
  UpdateDynamicPcds ();
  return EFI_SUCCESS;
}

/** PlatformConfigPei driver entry point.

  Platform config in PEI stage.

  @param[in]       FfsHeader    Pointer to Firmware File System file header.
  @param[in]       PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS           Platform config success.
*/
EFI_STATUS
EFIAPI
PlatformConfigPeiInit (
  IN       EFI_PEI_FILE_HANDLE            FileHandle,
  IN CONST EFI_PEI_SERVICES               **PeiServices
  )
{
  EFI_STATUS                        Status;

  //
  // Do config tasks when PCD PEI services available.
  //
  Status = PeiServicesNotifyPpi (&mPcdNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
