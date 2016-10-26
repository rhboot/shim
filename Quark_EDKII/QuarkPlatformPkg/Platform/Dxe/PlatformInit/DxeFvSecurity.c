/** @file
  This driver produces security architectural protocol based on SecurityManagementLib.
 
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


#include "DxeFvSecurity.h"
#include <Library/QuarkBootRomLib.h>

//
// Protocol notify related globals
//
VOID          *gEfiFwVolBlockNotifySecurityReg;
EFI_EVENT     gEfiFwVolBlockSecurityEvent;

//
// Handle for the FvbSecurity protocol.
//
EFI_HANDLE    mFvbSecurityProtocolHandle = NULL;

STATIC
VOID
InstallFvbSecurityProtocol (
  VOID
  )
{
  EFI_STATUS                                Status;
  FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL   *FvbSecurityProtocol;

  //
  // Make sure the FvbSecurity Protocol is not already installed
  // in the system.
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gFirmwareVolumeBlockSecurityGuid);

  //
  // Setup protocol structure.
  //
  FvbSecurityProtocol = (FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL *) 
    AllocateZeroPool (sizeof (FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL));

  ASSERT (FvbSecurityProtocol != NULL);
  FvbSecurityProtocol->SecurityAuthenticateImage = DxeSecurityVerifyFv;

  //
  // Install the FvbSecurityProtocol onto a new handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mFvbSecurityProtocolHandle,
                  &gFirmwareVolumeBlockSecurityGuid,
                  FvbSecurityProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}

/**
  This notification function is invoked when an instance of the
  EFI_FW_VOLUME_BLOCK_PROTOCOL is produced.  It layers an instance of the
  EFI_FIRMWARE_VOLUME2_PROTOCOL on the same handle.  This is the function where
  the actual initialization of the EFI_FIRMWARE_VOLUME2_PROTOCOL is done.

  @param  Event                 The event that occured
  @param  Context               For EFI compatiblity.  Not used.

**/
VOID
EFIAPI
FirmwareVolmeBlockNotifySecurityCallback (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
{
  EFI_HANDLE                            Handle;
  EFI_STATUS                            Status;
  UINTN                                 BufferSize;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL    *Fvb;
  EFI_PHYSICAL_ADDRESS                  FvbBaseAddress;
  EFI_PEI_HOB_POINTERS                  HobPtr;
  EFI_BOOT_MODE                         BootMode;

  BootMode = GetBootModeHob ();

  //
  // Examine all new handles
  //
  for (;;) {
    //
    // Get the next handle
    //
    BufferSize = sizeof (Handle);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    NULL,
                    gEfiFwVolBlockNotifySecurityReg,
                    &BufferSize,
                    &Handle
                    );

    //
    // If not found, we're done
    //
    if (EFI_NOT_FOUND == Status) {
      break;
    }

    if (!EFI_ERROR (Status)) {
      //
      // Get the FirmwareVolumeBlock protocol on that handle
      //
      Status = gBS->HandleProtocol (Handle, &gEfiFirmwareVolumeBlockProtocolGuid, (VOID **)&Fvb);
      if (!(EFI_ERROR (Status)) && (Fvb != NULL)) {
        Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
        if (!EFI_ERROR (Status)) {
          //
          // (1) Only authenticate new Firmware Volumes (any FV that has a HOB was already authenticated in PEI)
          // (2) Don't authenticate the NV Storage Firmware Volume as it's contents change (can't sign)
          //

          //
          // Check if this FV already has a HOB
          //
          HobPtr.Raw = GetHobList ();
          while ((HobPtr.Raw = GetNextHob (EFI_HOB_TYPE_FV, HobPtr.Raw)) != NULL) {
            if ((HobPtr.FirmwareVolume->BaseAddress) == (UINT32)FvbBaseAddress) {
              break;
            }
            HobPtr.Raw = GET_NEXT_HOB (HobPtr);
          }

          //
          // If this FV has no HOB and it is not the NVRAM FV then authenticate it.
          //
          if ((HobPtr.Raw == NULL) && (FvbBaseAddress != PcdGet32 (PcdFlashNvStorageBase))) {

            //
            // No need to check volumes which are within firmware flash items.
            // These are checked in security boot rom or PEI stage.
            //
            if (!(MfhLibIsAddressWithinFwFlashItem((UINT32) FvbBaseAddress))) {
              Status = DxeSecurityVerifyFv ((EFI_FIRMWARE_VOLUME_HEADER*) ((UINTN) FvbBaseAddress));
            }
          }
        }
      }
    }
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
    }
  }

  return;
}

/**
  Calls BootRom services to verify the Firmware Volume

  @param CurrentFvAddress   Pointer to the current Firmware Volume under consideration

  @retval EFI_SUCCESS       Firmware Volume is legal

**/
EFI_STATUS
DxeSecurityVerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress
  )
{
  EFI_STATUS  Status;

  //
  // Authenticate the Firmware Volume image
  //
  DEBUG ((DEBUG_INFO, "DxeSecurityVerifyFv - CurrentFvAddress=0x%8x\n", (UINT32)CurrentFvAddress));
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
  Installs Firmware Volume Security.

  @retval EFI_SUCCESS   Installed Firmware Volume Security successfully.

**/
EFI_STATUS
EFIAPI
DxeInitializeFvSecurity (
  VOID
  )
{
  if (mFvbSecurityProtocolHandle == NULL) {
    InstallFvbSecurityProtocol ();
  }

  //
  // Create a notify event any time the gEfiFirmwareVolumeBlockProtocolGuid is installed
  //
  gEfiFwVolBlockSecurityEvent = EfiCreateProtocolNotifyEvent (
                          &gEfiFirmwareVolumeBlockProtocolGuid,
                          TPL_CALLBACK,
                          FirmwareVolmeBlockNotifySecurityCallback,
                          NULL,
                          &gEfiFwVolBlockNotifySecurityReg
                          );

  return EFI_SUCCESS;
}
