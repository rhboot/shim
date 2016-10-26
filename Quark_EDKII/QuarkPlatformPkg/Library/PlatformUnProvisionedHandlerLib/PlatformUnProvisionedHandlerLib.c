/** @file

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

  PlatformUnProvisionedHandlerLib.c

Abstract:

  Library to handle Security Access Protocol SAP image loading
  when secure boot is un provisioned.

--*/

#include <PiDxe.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/SecureBootHelper.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PeCoffLib.h>
#include <Library/SecurityManagementLib.h>

/** Get the file image from mask.

  If This or File is NULL, then ASSERT().
  If sanity checks on This pointer fail then assert.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       File  This is a pointer to the device path of the file
                         to get IMAGE_FROM_XXX mask for.

  @return UINT32 Mask which is one of IMAGE_FROM_XXX defs in protocol header file.

**/
STATIC
UINT32
EFIAPI
GetImageFromMask (
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *File
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL          *TempDevicePath;
  EFI_BLOCK_IO_PROTOCOL             *BlockIo;

  //
  // First check to see if File is from a Firmware Volume
  //
  DeviceHandle      = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiFirmwareVolume2ProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      return IMAGE_FROM_FV;
    }
  }

  //
  // Next check to see if File is from a Block I/O device
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiBlockIoProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    BlockIo = NULL;
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status) && BlockIo != NULL) {
      if (BlockIo->Media != NULL) {
        if (BlockIo->Media->RemovableMedia) {
          //
          // Block I/O is present and specifies the media is removable
          //
          return IMAGE_FROM_REMOVABLE_MEDIA;
        } else {
          //
          // Block I/O is present and specifies the media is not removable
          //
          return IMAGE_FROM_FIXED_MEDIA;
        }
      }
    }
  }

  //
  // File is not in a Firmware Volume or on a Block I/O device, so check to see if
  // the device path supports the Simple File System Protocol.
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Simple File System is present without Block I/O, so assume media is fixed.
    //
    return IMAGE_FROM_FIXED_MEDIA;
  }

  //
  // File is not from an FV, Block I/O or Simple File System, so the only options
  // left are a PCI Option ROM and a Load File Protocol such as a PXE Boot from a NIC.
  //
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  while (!IsDevicePathEndType (TempDevicePath)) {
    switch (DevicePathType (TempDevicePath)) {

    case MEDIA_DEVICE_PATH:
      if (DevicePathSubType (TempDevicePath) == MEDIA_RELATIVE_OFFSET_RANGE_DP) {
        return IMAGE_FROM_OPTION_ROM;
      }
      break;

    case MESSAGING_DEVICE_PATH:
      if (DevicePathSubType(TempDevicePath) == MSG_MAC_ADDR_DP) {
        return IMAGE_FROM_PXE;
      }
      break;

    default:
      break;
    }
    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }
  return IMAGE_UNKNOWN;
}

/** Trace a Device Path.

  @param[in]       DevPath  Device Path to trace.

**/
STATIC
VOID
EFIAPI
TraceDevPath (
  IN CHAR16                               *Context,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *DevPath
  )
{
  EFI_STATUS                        Status;
  CHAR16                            *AsText;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *DevPathToText;

  EFI_DEVICE_PATH_PROTOCOL          *DupPath;
  DupPath = DuplicateDevicePath (DevPath);
  if (DupPath == NULL) {
    DEBUG ((EFI_D_INFO, "%s DuplicateDevicePath FAIL\n", Context));
    return;
  }

  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **) &DevPathToText
                  );
  if (!EFI_ERROR (Status)) {
      AsText = DevPathToText->ConvertDevicePathToText (
                                DupPath,
                                FALSE,
                                TRUE
                                );
      DEBUG ((EFI_D_INFO, "%s %s\n", Context, (AsText) ? AsText : L"ConvertDevicePathToText FAIL"));
  } else {
    DEBUG ((EFI_D_INFO, "%s Locate gEfiDevicePathToTextProtocolGuid FAIL\n", Context));
  }

  FreePool (DupPath);
}

/**
  The security handler is used to abstract platform-specific policy
  from the DXE core response to an attempt to use a file that returns a
  given status for the authentication check from the section extraction protocol.

  The possible responses in a given SAP implementation may include locking 
  flash upon failure to authenticate, attestation logging for all signed drivers,
  and other exception operations.  The File parameter allows for possible logging
  within the SAP of the driver.

  If File is NULL, then EFI_INVALID_PARAMETER is returned.

  If the file specified by File with an authentication status specified by 
  AuthenticationStatus is safe for the DXE Core to use, then EFI_SUCCESS is returned.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is not safe for the DXE Core to use under any circumstances,
  then EFI_ACCESS_DENIED is returned.

  If the file specified by File with an authentication status specified by 
  AuthenticationStatus is not safe for the DXE Core to use right now, but it
  might be possible to use it at a future time, then EFI_SECURITY_VIOLATION is
  returned.

  @param[in]    AuthenticationStatus 
                           This is the authentication status returned from the security
                           measurement services for the input file.
  @param[in]    File       This is a pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.
  @param[in]    FileBuffer File buffer matches the input file device path.
  @param[in]    FileSize   Size of File buffer matches the input file device path.

  @retval EFI_SUCCESS            The file specified by File did authenticate, and the
                                 platform policy dictates that the DXE Core may use File.
  @retval EFI_INVALID_PARAMETER  File is NULL.
  @retval EFI_SECURITY_VIOLATION The file specified by File did not authenticate, and
                                 the platform policy dictates that File should be placed
                                 in the untrusted state. A file may be promoted from
                                 the untrusted to the trusted state at a future time
                                 with a call to the Trust() DXE Service.
  @retval EFI_ACCESS_DENIED      The file specified by File did not authenticate, and
                                 the platform policy dictates that File should not be
                                 used for any purpose.

**/
EFI_STATUS
EFIAPI
UnProvisionedHandler (
  IN  OUT   UINT32                        AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL      *File,
  IN  VOID                                *FileBuffer OPTIONAL,
  IN  UINTN                               FileSize OPTIONAL,
  IN  BOOLEAN                             BootPolicy OPTIONAL
  )
{
  EFI_STATUS                        Status;
  UINT32                            ImageFromMask;
  BOOLEAN                           SecureBootEnabled;
  UINT32                            LoadPolicy;
  SECUREBOOT_HELPER_PROTOCOL        *SecureBootHelperProtocol;

  SecureBootHelperProtocol = NULL;

  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG_CODE_BEGIN ();
  TraceDevPath (L"AuthFile:-", File);
  DEBUG_CODE_END ();

  if (FeaturePcdGet (PcdSupportSecureBoot)) {
    if (SecureBootHelperProtocol == NULL) {
      Status = gBS->LocateProtocol(
                 &gSecureBootHelperProtocolGuid,
                 NULL,
                 (VOID**) &SecureBootHelperProtocol
                 );
      ASSERT_EFI_ERROR (Status);
      ASSERT (SecureBootHelperProtocol != NULL);
    }
    SecureBootEnabled = SecureBootHelperProtocol->IsSecureBootEnabled (
                                                     SecureBootHelperProtocol
                                                     );
    if (SecureBootEnabled) {
      //
      // Secure boot provisioned so let other handlers decide.
      //
      return EFI_SUCCESS;
    }
  }

  ImageFromMask = GetImageFromMask (
                   File
                   );
  LoadPolicy = (UINT32) ~(IMAGE_UNKNOWN);
  if (FeaturePcdGet (PcdEnableSecureLock)) {
    LoadPolicy = IMAGE_FROM_FV;
  }
  if ((ImageFromMask & LoadPolicy) == 0) {
    DEBUG ((EFI_D_INFO, "Platform Deny File FromMask 0x%08x LoadPolicy 0x%08x\n", ImageFromMask, LoadPolicy));
    Status = EFI_ACCESS_DENIED;
  } else {
    Status = EFI_SUCCESS;
  }
  return Status;
}

/** Register the security handler for Un Provisioned image verification.

  @param  ImageHandle  ImageHandle of the loaded driver.
  @param  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS            Register successfully.
  @retval  EFI_OUT_OF_RESOURCES   No enough memory to register this handler.
**/
EFI_STATUS
EFIAPI
PlatformUnProvisionedHandlerLibConstructor (
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_SYSTEM_TABLE                     *SystemTable
  )
{

  //
  // For this platform only enforce a policy if Secure Lock enabled or Secure
  // boot is supported in build.
  //
  if (FeaturePcdGet (PcdEnableSecureLock) || FeaturePcdGet (PcdSupportSecureBoot)) {
    return RegisterSecurity2Handler (
            UnProvisionedHandler,
            EFI_AUTH_OPERATION_VERIFY_IMAGE
            );
  }
  return EFI_SUCCESS;
}
