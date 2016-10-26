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

  Recovery.c

Abstract:

  Install Platform EFI_PEI_RECOVERY_MODULE_PPI and Implementation of
  EFI_PEI_LOAD_RECOVERY_CAPSULE service.

--*/

#include "CommonHeader.h"
#include "PlatformEarlyInit.h"

#include <Ppi/BlockIo.h>

//
// Capsule Types supported in this platform module
//
#include <Guid/CapsuleOnFatFloppyDisk.h>
#include <Guid/CapsuleOnFatIdeDisk.h>
#include <Guid/CapsuleOnFatUsbDisk.h>
#include <Guid/CapsuleOnDataCD.h>
#include <Guid/QuarkCapsuleGuid.h>
#include <Ppi/Cache.h>

#include <Ppi/RecoveryModule.h>
#include <Ppi/DeviceRecoveryModule.h>

#include <Library/PeiServicesLib.h>

//
// Required Service
//
EFI_STATUS
EFIAPI
PlatformRecoveryModule (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN EFI_PEI_RECOVERY_MODULE_PPI          *This
  );

VOID
AssertNoCapsulesError (
  IN EFI_PEI_SERVICES **PeiServices
  );

VOID
AssertMediaDeviceError (
  IN EFI_PEI_SERVICES **PeiServices
  );

VOID
ReportLoadCapsuleSuccess (
  IN EFI_PEI_SERVICES **PeiServices
  );

VOID
CheckIfMediaPresentOnBlockIoDevice (
  IN EFI_PEI_SERVICES   **PeiServices,
  IN OUT BOOLEAN        *MediaDeviceError,
  IN OUT BOOLEAN        *MediaPresent
  );

//
// Module globals
//
STATIC EFI_PEI_RECOVERY_MODULE_PPI  mRecoveryPpi = { PlatformRecoveryModule };

STATIC EFI_PEI_PPI_DESCRIPTOR         mRecoveryPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiRecoveryModulePpiGuid,
  &mRecoveryPpi
};

EFI_STATUS
EFIAPI
PeimInitializeRecovery (
  IN EFI_PEI_SERVICES     **PeiServices
  )
/*++

Routine Description:

  Provide the functionality of the Recovery Module.

Arguments:

  PeiServices  -  General purpose services available to every PEIM.
    
Returns:

  EFI_SUCCESS  -  If the interface could be successfully
                  installed.

--*/
{
  EFI_STATUS  Status;

  Status = PeiServicesInstallPpi (&mRecoveryPpiList);

  return Status;
}

EFI_STATUS
EFIAPI
PlatformRecoveryModule (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN EFI_PEI_RECOVERY_MODULE_PPI            *This
  )
/*++

Routine Description:

  Provide the functionality of the Platform Recovery Module.

Arguments:

  PeiServices  -  General purpose services available to every PEIM.
  This         -  Pointer to EFI_PEI_RECOVERY_MODULE_PPI.

Returns:

  EFI_SUCCESS      -  If the interface could be successfully
                      installed.
  EFI_UNSUPPORTED  -  Not supported.

--*/
{
  EFI_STATUS                            Status;
  EFI_PEI_DEVICE_RECOVERY_MODULE_PPI    *DeviceRecoveryModule;
  UINTN                                 NumberOfImageProviders;
  BOOLEAN                               ProviderAvailable;
  UINTN                                 NumberRecoveryCapsules;
  UINTN                                 RecoveryCapsuleSize;
  EFI_GUID                              DeviceId;
  BOOLEAN                               ImageFound;
  EFI_PHYSICAL_ADDRESS                  Address;
  VOID                                  *Buffer;
  EFI_CAPSULE_HEADER                    *CapsuleHeader;
  EFI_PEI_HOB_POINTERS                  Hob;
  EFI_PEI_HOB_POINTERS                  HobOld;
  BOOLEAN                               HobUpdate;
  EFI_FIRMWARE_VOLUME_HEADER            *FvHeader;
  UINTN                                 Index;
  EFI_STATUS                            AuthStatus;
  VOID                                  *CapsuleSecurityHeader;
  EFI_GUID                              mEfiCapsuleHeaderGuid = QUARK_CAPSULE_GUID;

  Index = 0;

  Status                  = EFI_SUCCESS;
  AuthStatus              = EFI_SUCCESS;
  CapsuleSecurityHeader   = NULL;
  HobUpdate               = FALSE;

  ProviderAvailable       = TRUE;
  ImageFound              = FALSE;
  NumberOfImageProviders  = 0;

  DeviceRecoveryModule    = NULL;

  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Recovery Entry\n"));

  //
  // Search the platform for some recovery capsule if the DXE IPL
  // discovered a recovery condition and has requested a load.
  //
  while (ProviderAvailable) {

    Status = PeiServicesLocatePpi (
              &gEfiPeiDeviceRecoveryModulePpiGuid,
              Index,
              NULL,
              &DeviceRecoveryModule
              );

    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Device Recovery PPI located\n"));
      NumberOfImageProviders++;

      Status = DeviceRecoveryModule->GetNumberRecoveryCapsules (
                                      PeiServices,
                                      DeviceRecoveryModule,
                                      &NumberRecoveryCapsules
                                      );

      DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Number Of Recovery Capsules: %d\n", NumberRecoveryCapsules));

      if (NumberRecoveryCapsules == 0) {
        Index++;
      } else {
        break;
      }
    } else {
      ProviderAvailable = FALSE;
    }
  }
  //
  // The number of recovery capsules is 0.
  //
  if (!ProviderAvailable) {
    AssertNoCapsulesError (PeiServices);
  }
  //
  // If there is an image provider, get the capsule ID
  //
  if (ProviderAvailable) {
    RecoveryCapsuleSize = 0;
    if (FeaturePcdGet (PcdFrameworkCompatibilitySupport)) {
      Status = DeviceRecoveryModule->GetRecoveryCapsuleInfo (
                                      PeiServices,
                                      DeviceRecoveryModule,
                                      0,
                                      &RecoveryCapsuleSize,
                                      &DeviceId
                                      );
    } else {
      Status = DeviceRecoveryModule->GetRecoveryCapsuleInfo (
                  PeiServices,
                  DeviceRecoveryModule,
                  1,
                  &RecoveryCapsuleSize,
                  &DeviceId
                  );


  }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Recovery Capsule Size: %d\n", RecoveryCapsuleSize));

    //
    // Only support the 2 capsule types known
    // Future enhancement is to rank-order the selection
    //
    if ((!CompareGuid (&DeviceId, &gPeiCapsuleOnFatIdeDiskGuid)) &&
        (!CompareGuid (&DeviceId, &gPeiCapsuleOnDataCDGuid)) &&
       (!CompareGuid (&DeviceId, &gPeiCapsuleOnFatUsbDiskGuid))
        ) {
      return EFI_UNSUPPORTED;
    }

    Buffer  = NULL;
    Address = (UINTN) AllocatePages ((RecoveryCapsuleSize - 1) / 0x1000 + 1);
    ASSERT (Address);

    Buffer = (UINT8 *) (UINTN) Address;
    if (FeaturePcdGet (PcdFrameworkCompatibilitySupport)) {
      Status = DeviceRecoveryModule->LoadRecoveryCapsule (
                                      PeiServices,
                                      DeviceRecoveryModule,
                                      0,
                                      Buffer
                                      );
     } else {
       Status = DeviceRecoveryModule->LoadRecoveryCapsule (
                                        PeiServices,
                                        DeviceRecoveryModule,
                                        1,
                                        Buffer
                                        );

     }

    DEBUG ((EFI_D_INFO | EFI_D_LOAD, "LoadRecoveryCapsule Returns: %r\n", Status));

    if (Status == EFI_DEVICE_ERROR) {
      AssertMediaDeviceError (PeiServices);
    }

    if (EFI_ERROR (Status)) {
      return Status;
    } else {
      ReportLoadCapsuleSuccess (PeiServices);
    }

    //
    // Update FV Hob if found
    //
    Buffer  = (VOID *)((UINT8 *) Buffer + FixedPcdGet32 (PcdFvSecurityHeaderSize));
    Status      = PeiServicesGetHobList (&Hob.Raw);
    HobOld.Raw  = Hob.Raw;
    while (!END_OF_HOB_LIST (Hob)) {
      if (Hob.Header->HobType == EFI_HOB_TYPE_FV) {
        DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Hob FV Length: %x\n", Hob.FirmwareVolume->Length));

        if (Hob.FirmwareVolume->BaseAddress == (UINTN) PcdGet32 (PcdFlashFvMainBase)) {
          HobUpdate = TRUE;
          //
          // This looks like the Hob we are interested in
          //
          DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Hob Updated\n"));
          Hob.FirmwareVolume->BaseAddress = (UINTN) Buffer;
          Hob.FirmwareVolume->Length      = RecoveryCapsuleSize;
        }
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
    }
    //
    // Check if the top of the file is a firmware volume header
    //
    FvHeader      = (EFI_FIRMWARE_VOLUME_HEADER *) Buffer;
    CapsuleHeader = (EFI_CAPSULE_HEADER *) Buffer;
    if (FvHeader->Signature == EFI_FVH_SIGNATURE) {
      //
      // build FV Hob if it is not built before
      //
      if (!HobUpdate) {
        DEBUG ((EFI_D_INFO | EFI_D_LOAD, "FV Hob is not found, Build FV Hob then..\n"));
        BuildFvHob (
          (UINTN) Buffer,
          FvHeader->FvLength
          );

        DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Install FV Info PPI..\n"));

        PeiServicesInstallFvInfoPpi (
          NULL,
          Buffer,
          (UINT32) FvHeader->FvLength,
          NULL,
          NULL
        );
      }
      //
      // Point to the location immediately after the FV.
      //
      CapsuleHeader = (EFI_CAPSULE_HEADER *) ((UINT8 *) Buffer + FvHeader->FvLength + FixedPcdGet32 (PcdFvSecurityHeaderSize));
      CapsuleSecurityHeader = ((UINT8 *) Buffer + FvHeader->FvLength);
    }

    //
    // Check if pointer is still within the buffer
    //
    if ((UINTN) CapsuleHeader < (UINTN) ((UINT8 *) Buffer + RecoveryCapsuleSize)) {

      //
      // Check if it is a capsule
      //
      if (CompareGuid ((EFI_GUID *) CapsuleHeader, &mEfiCapsuleHeaderGuid)) {

        CapsuleSecurityHeader = BuildCapsuleSecurityHeaderHob (
                                  CapsuleSecurityHeader
                                  );
        DEBUG (
          (EFI_D_INFO,
          "Recovery:SecHdrHob: 0x%08x\n",
          (UINTN) CapsuleSecurityHeader
          ));

        //
        // Set bootmode to capsule update so the capsule hob gets the right bootmode in the hob header.
        //
        Status = (**PeiServices).SetBootMode (PeiServices, BOOT_ON_FLASH_UPDATE);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        //
        // Build capsule hob
        //
        BuildCvHob ((EFI_PHYSICAL_ADDRESS)CapsuleHeader, (UINT64)(EFI_CAPSULE_HEADER *)CapsuleHeader->CapsuleImageSize);
      }
    }
  }

  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Recovery Module Returning: %r\n", Status));
  return Status;
}

/*
  AssertNoCapsulesError:
  There were no recovery capsules found. 
  Case 1: Report the error that no recovery block io device/media is readable and assert.  
  Case 2: Report the error that there is no media present on any recovery block io device and assert.
  Case 3: There is media present on some recovery block io device, 
          but there is no recovery capsule on it.  Report the error and assert.   
*/
VOID
AssertNoCapsulesError (
  IN EFI_PEI_SERVICES **PeiServices
  )
{
  BOOLEAN MediaDeviceError;
  BOOLEAN MediaPresent;

  MediaDeviceError  = TRUE;
  MediaPresent      = FALSE;

  CheckIfMediaPresentOnBlockIoDevice (PeiServices, &MediaDeviceError, &MediaPresent);
/*  if (MediaDeviceError) {
    ReportStatusCode (
      (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
      (EFI_PERIPHERAL_RECOVERY_MEDIA | EFI_P_EC_MEDIA_DEVICE_ERROR)
      );

  } else if (!MediaPresent) {
    ReportStatusCode (
      (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
      (EFI_PERIPHERAL_RECOVERY_MEDIA | EFI_P_EC_MEDIA_NOT_PRESENT)
      );

  } else {
    ReportStatusCode (
      (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
      (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_EC_NO_RECOVERY_CAPSULE)
      );
  }*/
  //
  // Hang.
  //
  CpuDeadLoop();
}

#define MAX_BLOCK_IO_PPI  32

/*
  CheckIfMediaPresentOnBlockIoDevice: 
  Checks to see whether there was a media device error or to see if there is media present.  
*/
VOID
CheckIfMediaPresentOnBlockIoDevice (
  IN EFI_PEI_SERVICES   **PeiServices,
  IN OUT BOOLEAN        *MediaDeviceError,
  IN OUT BOOLEAN        *MediaPresent
  )
{
  EFI_STATUS                      Status;
  UINTN                           BlockIoPpiInstance;
  EFI_PEI_RECOVERY_BLOCK_IO_PPI *BlockIoPpi;
  UINTN                           NumberBlockDevices;
  EFI_PEI_BLOCK_IO_MEDIA          Media;

  *MediaDeviceError = TRUE;
  *MediaPresent     = FALSE;

  for (BlockIoPpiInstance = 0; BlockIoPpiInstance < MAX_BLOCK_IO_PPI; BlockIoPpiInstance++) {
    Status = PeiServicesLocatePpi (
              &gEfiPeiVirtualBlockIoPpiGuid,
              BlockIoPpiInstance,
              NULL,
              &BlockIoPpi
              );
    if (EFI_ERROR (Status)) {
      //
      // Done with all Block Io Ppis
      //
      break;
    }

    Status = BlockIoPpi->GetNumberOfBlockDevices (
                          PeiServices,
                          BlockIoPpi,
                          &NumberBlockDevices
                          );
    if (EFI_ERROR (Status) || (NumberBlockDevices == 0)) {
      continue;
    }
    //
    // Just retrieve the first block
    //
    Status = BlockIoPpi->GetBlockDeviceMediaInfo (
                          PeiServices,
                          BlockIoPpi,
                          0,
                          &Media
                          );
    if (!EFI_ERROR (Status)) {
      *MediaDeviceError = FALSE;
      if (Media.MediaPresent) {
        *MediaPresent = TRUE;
        break;
      }
    }
  }
}

VOID
AssertMediaDeviceError (
  IN EFI_PEI_SERVICES **PeiServices
  )
{
/*  ReportStatusCode (
    (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
    (EFI_PERIPHERAL_RECOVERY_MEDIA | EFI_P_EC_MEDIA_DEVICE_ERROR)
    );
*/
  CpuDeadLoop ();
}

VOID
ReportLoadCapsuleSuccess (
  IN EFI_PEI_SERVICES **PeiServices
  )
{
  //
  // EFI_SW_PEI_PC_CAPSULE_START: (from the status code spec):
  // Loaded the recovery capsule.  About to hand off control to the capsule.
  //
/*  ReportStatusCode (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEIM_PC_CAPSULE_LOAD_SUCCESS)
    );*/
}

