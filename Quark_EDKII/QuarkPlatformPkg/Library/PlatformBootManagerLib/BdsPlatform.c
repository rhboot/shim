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

  BdsPlatform.c

Abstract:

  This file include all platform action which can be customized
  by IBV/OEM.

--*/

#include "BdsPlatform.h"

extern USB_CLASS_FORMAT_DEVICE_PATH gUsbClassKeyboardDevicePath;
EFI_USER_PROFILE_HANDLE             mCurrentUser = NULL;
UINT16                              mPayloadBootOptionNumber = 0xffff;
SECUREBOOT_HELPER_PROTOCOL          *gSecureBootHelperProtocol = NULL;
EFI_PLATFORM_TYPE_PROTOCOL          *gPlatformType = NULL;

/**
  Identify a user and, if authenticated, returns the current user profile handle.

  @param[out]  User           Point to user profile handle.

  @retval EFI_SUCCESS         User is successfully identified, or user identification
                              is not supported.
  @retval EFI_ACCESS_DENIED   User is not successfully identified

**/
EFI_STATUS
UserIdentify (
  OUT EFI_USER_PROFILE_HANDLE         *User
  )
{
  EFI_STATUS                          Status;
  EFI_USER_MANAGER_PROTOCOL           *Manager;

  Status = gBS->LocateProtocol (
                  &gEfiUserManagerProtocolGuid,
                  NULL,
                  (VOID **) &Manager
                  );
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  return Manager->Identify (Manager, User);
}

/**
  Search out the video controller.

  @return  PCI device path of the video controller.
**/
EFI_HANDLE
PlatformGetVideoController (
  EFI_HANDLE                *OnboardVideoController,
  EFI_HANDLE                *AddinVideoController
  )
{
  EFI_STATUS                Status;
  UINTN                     RootBridgeHandleCount;
  EFI_HANDLE                *RootBridgeHandleBuffer;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     RootBridgeIndex;
  UINTN                     Index;
  EFI_HANDLE                VideoController = NULL;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;
  EFI_PCI_PLATFORM_PROTOCOL *PciPlatformProtocol;

  //
  // Make all the PCI_IO protocols show up
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &RootBridgeHandleCount,
                  &RootBridgeHandleBuffer
                  );
  if (EFI_ERROR (Status) || (RootBridgeHandleCount == 0)) {
    return NULL;
  }

  Status = gBS->LocateProtocol (
                &gEfiPciPlatformProtocolGuid,
                NULL,
                (VOID **) &PciPlatformProtocol
            );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {
    gBS->ConnectController (RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, FALSE);

    //
    // Start to check all the pci io to find the first video controller
    //
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiPciIoProtocolGuid,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID **) &PciIo);
      if (!EFI_ERROR (Status)) {
        //
        // Check for all video controller
        //
        Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint32,
                          0,
                          sizeof (Pci) / sizeof (UINT32),
                          &Pci
                          );
        if (!EFI_ERROR (Status) && IS_PCI_VGA (&Pci)) {
          // Set VideoController to default to first video controller.
          // This will be the device used if Video Select is set to Auto.
          if(VideoController == NULL) {
            VideoController = HandleBuffer[Index];
            *AddinVideoController = HandleBuffer[Index];
          }
        }
      }
    }
    FreePool (HandleBuffer);

    if (VideoController != NULL) {
      break;
    }
  }
  FreePool (RootBridgeHandleBuffer);

  return VideoController;
}


VOID
UpdateConOut (
  VOID
  )
{
  EFI_HANDLE                 VideoController;
  EFI_DEVICE_PATH_PROTOCOL   *Gop;
  EFI_HANDLE                 OnboardVideoController = NULL;
  EFI_HANDLE                 AddinVideoController = NULL;
  UINTN                      VideoSelect;

  //
  // Get the platform vga device
  //
  VideoController = PlatformGetVideoController (&OnboardVideoController, &AddinVideoController);

  if (VideoController == NULL) {
    return ;
  }


  // Force to Auto
  VideoSelect = 0;

  // If device selected by SystemConfig is not present, force to Auto
  if (VideoSelect == 1 && OnboardVideoController == NULL) {
    VideoSelect = 0;
  }
  if (VideoSelect == 2 && AddinVideoController == NULL) {
    VideoSelect = 0;
  }

  switch(VideoSelect) {
  case 1:
    // Onboard selected
    VideoController = OnboardVideoController;
    DEBUG((EFI_D_INFO, "Video select: Onboard\n"));
    break;
  case 2:
    // Add-in selected
    VideoController = AddinVideoController;
    DEBUG((EFI_D_INFO, "Video select: Add-in\n"));
    break;
  case 0:
  default:
    // Use first VideoController found, which is what GetVideoController returns as VideoController
    DEBUG((EFI_D_INFO, "Video select: Auto\n"));
    break;
  }

  //
  // Try to connect the PCI device path, so that GOP dirver could start on this
  // device and create child handles with GraphicsOutput Protocol installed
  // on them, then we get device paths of these child handles and select
  // them as possible console device.
  //
  gBS->ConnectController (VideoController, NULL, NULL, FALSE);

  Gop = EfiBootManagerGetGopDevicePath (VideoController);
  if (Gop == NULL) {
    return ;
  }

  //
  // Update ConOut variable to remove device path of on-board/add-in video controller
  // and add the device path of the specified video controller
  //
  EfiBootManagerUpdateConsoleVariable (ConOut, NULL, DevicePathFromHandle (OnboardVideoController));
  EfiBootManagerUpdateConsoleVariable (ConOut, Gop, DevicePathFromHandle (AddinVideoController));
  FreePool (Gop);

}


/**
  Initialize the platform default console variables when the console variable is empty.

  @param PlatformConsole  Predfined platform default console device array.
**/
VOID
InitializeConsoleVariables (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
  )
{
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *VarConOut;
  EFI_DEVICE_PATH_PROTOCOL  *VarConIn;
  EFI_BOOT_MODE             BootMode;

  BootMode = GetBootModeHob ();

  VarConOut = GetEfiGlobalVariable (L"ConOut");   if (VarConOut != NULL) { FreePool (VarConOut); }
  VarConIn  = GetEfiGlobalVariable (L"ConIn");    if (VarConIn  != NULL) { FreePool (VarConIn);  }
  if (VarConOut == NULL || VarConIn == NULL) {
    //
    // Only fill ConIn/ConOut when ConIn/ConOut is empty because we may drop to Full Configuration boot mode in non-first boot
    //
    for (Index = 0; PlatformConsole[Index].DevicePath != NULL; Index++) {
      //
      // Update the console variable with the connect type
      //
      if ((PlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
        EfiBootManagerUpdateConsoleVariable (ConIn, PlatformConsole[Index].DevicePath, NULL);
      }
      if ((PlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
        EfiBootManagerUpdateConsoleVariable (ConOut, PlatformConsole[Index].DevicePath, NULL);
      }
      if ((PlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
        EfiBootManagerUpdateConsoleVariable (ErrOut, PlatformConsole[Index].DevicePath, NULL);
      }
    }
  }
}

//
// BDS Platform Functions
//
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
{
  EFI_HANDLE                Handle;
  EFI_STATUS                Status;
  EFI_ACPI_S3_SAVE_PROTOCOL *AcpiS3Save;
  BOOLEAN                   SecureBootSupportEnabled;
  BOOLEAN                   SecureBootEnabled;

  SecureBootSupportEnabled = FALSE;
  SecureBootEnabled = FALSE;

  if (FeaturePcdGet (PcdSupportSecureBoot)) {

    //
    // Auto provision UEFI Secure boot.
    //
    SecureBootSupportEnabled = TRUE;
    SecureBootEnabled = PlatformAutoProvisionSecureBoot ();

    //
    // UEFI Secure boot not supported / validated for this firmware release.
    //
    ASSERT (FALSE);

  }

  if (FeaturePcdGet (PcdEnableSecureLock)) {
    if (!SecureBootEnabled) {
      PcdSetBool (PcdConInConnectOnDemand, TRUE);
    }

    DEBUG (
      (EFI_D_INFO, "Secure Lock ENABLED Secure Boot Support %s Secure Boot %s\n",
      SecureBootSupportEnabled ? L"ENABLED" : L"DISABLED",
      SecureBootEnabled ? L"ENABLED" : L"DISABLED"
      ));
  } else {
    DEBUG (
      (EFI_D_INFO, "Secure Lock DISABLED Secure Boot Support %s Secure Boot %s\n",
      SecureBootSupportEnabled ? L"ENABLED" : L"DISABLED",
      SecureBootEnabled ? L"ENABLED" : L"DISABLED"
      ));
  }

  //
  // Add platform string package
  //
  InitializeStringSupport ();

  InitializeConsoleVariables (gPlatformConsole);
  UpdateConOut ();

  RegisterLoadOptions ();


  Status = gBS->LocateProtocol (&gEfiAcpiS3SaveProtocolGuid, NULL, (VOID **)&AcpiS3Save);
  if (!EFI_ERROR (Status)) {
    AcpiS3Save->S3Save (AcpiS3Save, NULL);
  }

  //
  // Inform the SMM infrastructure that we're entering BDS and may run 3rd party code hereafter
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gExitPmAuthProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Append Usb Keyboard short form DevicePath into "ConInDev"
  //
  EfiBootManagerUpdateConsoleVariable (
    ConInDev,
    (EFI_DEVICE_PATH_PROTOCOL *) &gUsbClassKeyboardDevicePath,
    NULL
    );

  //
  // Before user authentication, the user identification devices need be connected
  // from the platform customized device paths
  //
  ConnectAuthDevice ();

  //
  // As console is not ready, the auto logon user will be identified.
  //
  UserIdentify (&mCurrentUser);
}

VOID
ConnectSequence (
  VOID
  )
/*++

Routine Description:

  Connect with predeined platform connect sequence,
  the OEM/IBV can customize with their own connect sequence.

Arguments:

  None.

Returns:

  None.

--*/
{
  UINTN Index;

  //
  // Here we can get the customized platform connect sequence
  // Notes: we can connect with new variable which record the
  // last time boots connect device path sequence
  //
  for (Index = 0; gPlatformConnectSequence[Index] != NULL; Index++) {
    //
    // Build the platform boot option
    //
    EfiBootManagerConnectDevicePath (gPlatformConnectSequence[Index], NULL);
  }
  //
  // For the debug tip, just use the simple policy to connect all devices
  //
  EfiBootManagerConnectAll ();

}


EFI_DEVICE_PATH *
FvFilePath (
  UINTN                         FvBaseAddress,
  UINTN                         FvSize,
  EFI_GUID                     *FileGuid
  )
{

  EFI_STATUS                         Status;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_HANDLE                         FvProtocolHandle;
  EFI_HANDLE                         *FvHandleBuffer;
  UINTN                              FvHandleCount;
  EFI_FV_FILETYPE                    Type;
  UINTN                              Size;
  EFI_FV_FILE_ATTRIBUTES             Attributes;
  UINT32                             AuthenticationStatus;
  EFI_FIRMWARE_VOLUME2_PROTOCOL      *Fv;
  UINTN                              Index;

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);

  if (FvBaseAddress == 0) {
    Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &LoadedImage
                  );
    ASSERT_EFI_ERROR (Status);
    return AppendDevicePathNode (
           DevicePathFromHandle (LoadedImage->DeviceHandle),
           (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
           );
  }
  else {
    //
    // Expose Payload file in FV
    //
    gDS->ProcessFirmwareVolume (
           (VOID *)FvBaseAddress,
           (UINT32)FvSize,
           &FvProtocolHandle
           );

    //
    // Find out the handle of FV containing the playload file
    //
    FvHandleBuffer = NULL;
    gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiFirmwareVolume2ProtocolGuid,
          NULL,
          &FvHandleCount,
          &FvHandleBuffer
          );
    for (Index = 0; Index < FvHandleCount; Index++) {
      gBS->HandleProtocol (
            FvHandleBuffer[Index],
            &gEfiFirmwareVolume2ProtocolGuid,
            (VOID **) &Fv
            );
      Status = Fv->ReadFile (
                  Fv,
                  FileGuid,
                  NULL,
                  &Size,
                  &Type,
                  &Attributes,
                  &AuthenticationStatus
                  );
      if (!EFI_ERROR (Status)) {
        break;
      }
    }
    if (Index < FvHandleCount) {
      return AppendDevicePathNode (
            DevicePathFromHandle (FvHandleBuffer[Index]),
            (  EFI_DEVICE_PATH_PROTOCOL *) &FileNode
            );
    }
  }
  return NULL;
}

EFI_GUID mBootMenuFile = {
  0xEEC25BDC, 0x67F2, 0x4D95, { 0xB1, 0xD5, 0xF8, 0x1B, 0x20, 0x39, 0xD1, 0x1D }
};

EFI_STATUS
RegisterFvBootOption (
  IN UINTN                            FvBaseAddress,
  IN UINTN                            FvSize,
  IN EFI_GUID                         *FileGuid,
  IN CHAR16                           *Description,
  IN UINTN                            Position,
  IN BOOLEAN                          IsBootCategory,
  OUT UINT16                          *OptionNumberPtr OPTIONAL
  )
{
  EFI_STATUS                       Status;
  UINTN                            OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION     NewOption;
  EFI_BOOT_MANAGER_LOAD_OPTION     *BootOptions;
  UINTN                            BootOptionCount;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;

  if (OptionNumberPtr != NULL) {
    *OptionNumberPtr = (UINT16) -1;
  }

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  DevicePath = FvFilePath (FvBaseAddress, FvSize, FileGuid);
  if (DevicePath != NULL) {
    Status = EfiBootManagerInitializeLoadOption (
               &NewOption,
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               IsBootCategory ? LOAD_OPTION_ACTIVE : LOAD_OPTION_CATEGORY_APP,
               Description,
               DevicePath,
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
    FreePool (DevicePath);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  } else {
    return EFI_INVALID_PARAMETER;
  }

  OptionIndex = EfiBootManagerFindLoadOption (&NewOption, BootOptions, BootOptionCount);

  if (OptionIndex == -1) {
    Status = EfiBootManagerAddLoadOptionVariable (&NewOption, Position);
    ASSERT_EFI_ERROR (Status);
  } else {
    NewOption.OptionNumber = BootOptions[OptionIndex].OptionNumber;
    Status = EFI_SUCCESS;
  }
  EfiBootManagerFreeLoadOption (&NewOption);
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  if (OptionNumberPtr != NULL) {
    *OptionNumberPtr = (UINT16) NewOption.OptionNumber;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
RegisterPayloadBootOption (
  VOID
  )
{
  EFI_STATUS                     Status;
  UINT32                         ExpectedBaseAddress;
  UINT32                         TryBaseAddress;
  BOOLEAN                        SecureBootEnabled;

  SecureBootEnabled = FALSE;

  ExpectedBaseAddress = (UINTN) PcdGet32(PcdFlashFvPayloadBase);
  Status = RegisterFvBootOption (
             (UINTN) ExpectedBaseAddress,
             (UINTN) PcdGet32(PcdFlashFvPayloadSize),
             PcdGetPtr (PcdPayloadFile),
             L"UEFI Payload",
             (UINTN) 1,
             TRUE,
             &mPayloadBootOptionNumber
             );
  if (!EFI_ERROR(Status)) {
    return Status;
  }
  DEBUG ((EFI_D_ERROR, "Payload Not found\n"));
  if (FeaturePcdGet (PcdEnableSecureLock)) {
    if (gSecureBootHelperProtocol != NULL) {
      SecureBootEnabled = gSecureBootHelperProtocol->IsSecureBootEnabled (
                                                       gSecureBootHelperProtocol
                                                       );
    }
    //
    // In secure lockdown builds payload must be found if UEFI SecureBoot disabled.
    //
    ASSERT (SecureBootEnabled);
  }

  //
  // Try 4G - 4M.
  //
  TryBaseAddress = 0xFFFFFFFF - (4 *1024 * 1024) + 1;
  TryBaseAddress += PcdGet32 (PcdFvSecurityHeaderSize);
  DEBUG ((EFI_D_WARN, "[PlatformBds] Try if payload at 4G-4M base = 0x%08x\n", (UINTN) TryBaseAddress));
  if (TryBaseAddress != ExpectedBaseAddress) {
    Status = RegisterFvBootOption (
               (UINTN) TryBaseAddress,
               (UINTN) PcdGet32(PcdFlashFvPayloadSize),
               PcdGetPtr (PcdPayloadFile),
               L"UEFI Payload",
               (UINTN) 1,
               TRUE,
               &mPayloadBootOptionNumber
               );
    if (!EFI_ERROR(Status)) {
      // Use EFI_D_ERROR so user on release builds knows payload found.
      DEBUG ((EFI_D_ERROR, "[PlatformBds] Payload found in 4M flash default address\n"));
      return Status;
    }
  }
  //
  // Try 4G - 8M.
  //
  TryBaseAddress = 0xFFFFFFFF - (8 *1024 * 1024) + 1;
  TryBaseAddress += PcdGet32 (PcdFvSecurityHeaderSize);
  DEBUG ((EFI_D_WARN, "[PlatformBds] Try if payload at 4G-8M base = 0x%08x\n", (UINTN) TryBaseAddress));
  if (TryBaseAddress != ExpectedBaseAddress) {
    Status = RegisterFvBootOption (
               (UINTN) TryBaseAddress,
               (UINTN) PcdGet32(PcdFlashFvPayloadSize),
               PcdGetPtr (PcdPayloadFile),
               L"UEFI Payload",
               (UINTN) 1,
               TRUE,
               &mPayloadBootOptionNumber
               );
    if (!EFI_ERROR(Status)) {
      // Use EFI_D_ERROR so user on release builds knows payload found.
      DEBUG ((EFI_D_ERROR, "[PlatformBds] Payload found in 8M flash default address\n"));
      return Status;
    }
  }
  return EFI_NOT_FOUND;
}

VOID
RegisterLoadOptions (
  VOID
  )
/*++

Routine Description:

  Load the predefined driver option, OEM/IBV can customize this
  to load their own drivers.

Arguments:

  BdsDriverLists  -  The header of the driver option link list.

Returns:

  None.

--*/
{
  EFI_STATUS                     Status;
  UINTN                          Index;
  EFI_BOOT_MANAGER_LOAD_OPTION   DriverOption;
  EFI_BOOT_MANAGER_LOAD_OPTION   *DriverOptions;
  UINTN                          DriverOptionCount;
  UINT16                         OptionNumber;
  EFI_INPUT_KEY                  F7;
  EFI_INPUT_KEY                  Enter;
  EFI_BOOT_MODE                  BootMode;

  BootMode = GetBootModeHob ();

  //
  // 0. Continue key
  //
  Enter.ScanCode    = SCAN_NULL;
  Enter.UnicodeChar = CHAR_CARRIAGE_RETURN;
  EfiBootManagerRegisterContinueKeyOption (0, &Enter, NULL);

  //
  // 1. Add Payload if not flash update and not recovery.
  //
  if (BootMode != BOOT_ON_FLASH_UPDATE && BootMode != BOOT_IN_RECOVERY_MODE) {
    RegisterPayloadBootOption ();
  }

  //
  // 2. Internal Shell if Secure Lock Disabled.
  //
  if(!FeaturePcdGet (PcdEnableSecureLock)) {
    //
    // Internal shell not allowed in Secure Lock down builds.
    //
    RegisterFvBootOption (0, 0, PcdGetPtr (PcdShellFile), L"UEFI Internal Shell", (UINTN) -1, TRUE, NULL);
  }

  //
  // 3. Boot Device List menu
  //
  F7.UnicodeChar  = CHAR_NULL;
  F7.ScanCode     = SCAN_F7;
  Status = RegisterFvBootOption (0, 0, &mBootMenuFile, L"Boot Device List", (UINTN) -1, FALSE, &OptionNumber);
  ASSERT_EFI_ERROR (Status);
  Status = EfiBootManagerAddKeyOptionVariable (NULL, OptionNumber, 0, &F7, NULL);
  DEBUG ((EFI_D_WARN, "[PlatformBds] Add Key Boot Device List Status = %r, Option %d\n", (UINTN) Status, OptionNumber));
  ASSERT (Status == EFI_ALREADY_STARTED || Status == EFI_SUCCESS);

  //
  // 4. Create platform specified Driver####
  //
  DriverOptions = EfiBootManagerGetLoadOptions (&DriverOptionCount, LoadOptionTypeDriver);

  for (Index = 0; gPlatformDriverOption[Index] != NULL; Index++) {
    Status = EfiBootManagerInitializeLoadOption (
               &DriverOption,
               LoadOptionNumberUnassigned,
               LoadOptionTypeDriver,
               LOAD_OPTION_ACTIVE,
               L"",
               gPlatformDriverOption[Index],
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);

    if (EfiBootManagerFindLoadOption (&DriverOption, DriverOptions, DriverOptionCount) == -1) {
      Status = EfiBootManagerAddLoadOptionVariable (&DriverOption, (UINTN) -1);
      ASSERT_EFI_ERROR (Status);
    }

    EfiBootManagerFreeLoadOption (&DriverOption);
  }

  EfiBootManagerFreeLoadOptions (DriverOptions, DriverOptionCount);
}

VOID
Diagnostics (
  IN EXTENDMEM_COVERAGE_LEVEL    MemoryTestLevel,
  IN BOOLEAN                     QuietBoot
  )
/*++

Routine Description:

  Perform the platform diagnostic, such like test memory. OEM/IBV also
  can customize this fuction to support specific platform diagnostic.

Arguments:

  MemoryTestLevel  - The memory test intensive level

  QuietBoot        - Indicate if need to enable the quiet boot

  BaseMemoryTest   - A pointer to BdsMemoryTest()

Returns:

  None.

--*/
{
  EFI_STATUS  Status;

  //
  // Here we can decide if we need to show
  // the diagnostics screen
  // Notes: this quiet boot code should be remove
  // from the graphic lib
  //

  if (QuietBoot) {
    EnableQuietBoot (PcdGetPtr(PcdLogoFile));

    //
    // Perform system diagnostic
    //
    Status = MemoryTest (MemoryTestLevel);
    if (EFI_ERROR (Status)) {
      DisableQuietBoot ();
    }
    return ;
  }
  //
  // Perform system diagnostic
  //
  Status = MemoryTest (MemoryTestLevel);

}

/**
  Returns the priority number.

  @param BootOption
**/
UINTN
BootOptionPriority (
  CONST EFI_BOOT_MANAGER_LOAD_OPTION *BootOption
  )
{
  //
  // Make sure Shell is in the last
  //
  if (StrCmp (BootOption->Description, L"USB Device") == 0) {
    return 10;
  }
  if (StrCmp (BootOption->Description, L"UEFI Payload") == 0) {
    return 90;
  }
  if (StrCmp (BootOption->Description, L"UEFI Internal Shell") == 0) {
    return 100;
  }
  return 0;
}

BOOLEAN
EFIAPI
CompareBootOption (
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Left,
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Right
  )
{
  return (BOOLEAN) (BootOptionPriority (Left) < BootOptionPriority (Right));
}

extern EFI_GUID  gSignalBeforeEnterSetupGuid;

/**
  This function Installs a guid before entering the Setup.

**/
VOID
SignalProtocolEvent(IN EFI_GUID *ProtocolGuid)
{
  EFI_HANDLE  Handle = NULL;

  gBS->InstallProtocolInterface (
      &Handle, ProtocolGuid, EFI_NATIVE_INTERFACE,NULL
  );

  gBS->UninstallProtocolInterface (
      Handle, ProtocolGuid, NULL
  );
}

/**
  Boot handling when secure lock down enabled.

  Boot payload forever ie if payload returns then boot it again.

  WARNING: Status variable in function needs to be volatile to handle
  unreachable code error with some compilers.

**/
VOID
EFIAPI
SecureLockBoot (
  VOID
  )
{
  volatile EFI_STATUS            Status;
  CHAR16                         OptionName[sizeof ("Boot####")];
  EFI_BOOT_MANAGER_LOAD_OPTION   BootOption;
  BOOLEAN                        SecureBootEnabled;

  SecureBootEnabled = FALSE;

  for (Status = EFI_SUCCESS; Status == EFI_SUCCESS; Status = EFI_SUCCESS) {
    if(gSecureBootHelperProtocol != NULL) {
      //
      // re-determine in every loop iteration in case payload enables UEFI SecureBoot.
      //
      SecureBootEnabled = gSecureBootHelperProtocol->IsSecureBootEnabled (
                                                       gSecureBootHelperProtocol
                                                       );
    }

    if (FeaturePcdGet (PcdEnableSecureLock) && SecureBootEnabled == FALSE) {
      //
      // Force boot of payload in secure lock down builds if UEFI SecureBoot disabled.
      //
      DEBUG ((EFI_D_INFO, "SecureLockBoot ENABLED and SecureBoot Disabled: Only Allow Payload boot.\n"));
      UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", mPayloadBootOptionNumber);
      Status = EfiBootManagerVariableToLoadOption (OptionName, &BootOption);
      ASSERT_EFI_ERROR (Status);
      EfiBootManagerBoot (&BootOption);
      EfiBootManagerFreeLoadOption (&BootOption);
    } else {
      DEBUG ((EFI_D_INFO, "SecureLockBoot Do Nothing: Continue to BootOption Boot\n"));
      break;
    }
  }
}

UINT32
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
/*++

Routine Description:

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.

Arguments:

  DriverOptionList - The header of the driver option link list

  BootOptionList   - The header of the boot option link list

  ProcessCapsules  - A pointer to ProcessCapsules()

  BaseMemoryTest   - A pointer to BaseMemoryTest()

Returns:

  None.

--*/
{
  EFI_HANDLE         Handle;
  EFI_STATUS         Status;
  EFI_BOOT_MODE      BootMode;
  BOOLEAN            DeferredImageExist;
  UINTN              Index;
  CHAR16             CapsuleVarName[30];
  CHAR16             *TempVarName;
  VOID               *BootState;
  EFI_GUID           EfiBootStateGuid = {
    0x60b5e939, 0xfcf, 0x4227, { 0xba, 0x83, 0x6b, 0xbe, 0xd4, 0x5b, 0xc0, 0xe3 }
  };
  BOOLEAN            SecureBootEnabled;
  BOOLEAN            FalseUpdateLedOn;

  SecureBootEnabled = FALSE;
  FalseUpdateLedOn = FALSE;

  if(gSecureBootHelperProtocol != NULL) {
    SecureBootEnabled = gSecureBootHelperProtocol->IsSecureBootEnabled (
                                                     gSecureBootHelperProtocol
                                                     );
  }

  //
  // Check BootState variable, NULL means it's the first boot after reflashing
  //
  BootState = GetVariable (L"BootState", &EfiBootStateGuid);
  if (BootState != NULL) {
    FreePool (BootState);
  }

  BootMode = GetBootModeHob ();
  DEBUG ((EFI_D_INFO, "[PlatformBds]BootMode = %d\n", (UINTN) BootMode));

  //
  // Clear all the capsule variables CapsuleUpdateData, CapsuleUpdateData1, CapsuleUpdateData2...
  // as early as possible which will avoid the next time boot after the capsule update
  // will still into the capsule loop
  //
  StrCpy (CapsuleVarName, EFI_CAPSULE_VARIABLE_NAME);
  TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
  Index = 0;
  while (TRUE) {
    if (Index > 0) {
      UnicodeValueToString (TempVarName, 0, Index, 0);
    }
    Status = gRT->SetVariable (
                 CapsuleVarName,
                 &gEfiCapsuleVendorGuid,
                 EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS,
                 0,
                 (VOID *)NULL
                 );
    if (EFI_ERROR (Status)) {
      //
      // There is no capsule variables, quit
      //
      break;
    }
    Index++;
  }

  //
  // No deferred image exists by default
  //
  DeferredImageExist = FALSE;

  //
  // Go the different platform policy with different boot mode
  // Notes: this part code can be change with the table policy
  //
  switch (BootMode) {

  case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
  case BOOT_WITH_MINIMAL_CONFIGURATION:
    //
    // Memory test and Logo show
    //
    Diagnostics (IGNORE, TRUE);

    //
    // Perform some platform specific connect sequence
    //
    ConnectSequence ();

    //
    // As console is ready, perform user identification again.
    //
    if (mCurrentUser == NULL) {
      CheckDeferredImage (&mCurrentUser, &DeferredImageExist);
      if (DeferredImageExist) {
        //
        // After user authentication, the deferred drivers was loaded again.
        // Here, need to ensure the deferred images are connected.
        //
        EfiBootManagerConnectAllDefaultConsoles ();
        ConnectSequence ();
      }
    }

    //
    // If unsecure refresh and sort boot options.
    //
    if (!FeaturePcdGet (PcdEnableSecureLock)) {
      DEBUG ((EFI_D_INFO, "[PlatformBds]: Refresh All and Sort.\n"));
      EfiBootManagerRefreshAllBootOption ();
      EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, CompareBootOption);
    }

    break;

  case BOOT_ON_FLASH_UPDATE:
    //
    // Clear protected ranges before updating flash.
    //
    Status = PlatformClearSpiProtect ();
    ASSERT_EFI_ERROR (Status);
    FalseUpdateLedOn = TRUE;
    PlatformFlashUpdateLed (gPlatformType->Type, FalseUpdateLedOn);
    if (FeaturePcdGet (PcdSupportUpdateCapsuleReset)) {
      EfiBootManagerProcessCapsules ();
    } else {
      ASSERT (FeaturePcdGet (PcdSupportUpdateCapsuleReset));
    }

    //
    // Toggle flash update LED for a predefined number of times with 
    // a predefined interval.
    //
    for (Index = 0; Index < PLATFORM_FLASH_UPDATE_LED_TOGGLE_COUNT; Index++) {
      FalseUpdateLedOn = (FalseUpdateLedOn) ? FALSE : TRUE;
      PlatformFlashUpdateLed (gPlatformType->Type, FalseUpdateLedOn);
      gBS->Stall (PLATFORM_FLASH_UPDATE_LED_TOGGLE_DELTA);
    }

    //
    // Cold reset the system as any flash update are now complete
    //
    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

    break;

  case BOOT_IN_RECOVERY_MODE:
    //
    // In recovery mode, just connect platform console
    // and show up the front page
    //
    Diagnostics (EXTENSIVE, FALSE);
    EfiBootManagerConnectAll ();

    //
    // Perform user identification
    //
    if (mCurrentUser == NULL) {
      CheckDeferredImage (&mCurrentUser, &DeferredImageExist);
      if (DeferredImageExist) {
        //
        // After user authentication, the deferred drivers was loaded again.
        // Here, need to ensure the deferred drivers are connected.
        //
        EfiBootManagerConnectAll ();
      }
    }

    break;

  case BOOT_WITH_FULL_CONFIGURATION:
  case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
  case BOOT_WITH_DEFAULT_SETTINGS:
  default:
    //
    // Memory test and Logo show
    //
    Diagnostics (IGNORE, TRUE);

    //
    // Perform some platform specific connect sequence
    //
    ConnectSequence ();

    //
    // Perform user identification
    //
    if (mCurrentUser == NULL) {
      CheckDeferredImage (&mCurrentUser, &DeferredImageExist);
      if (DeferredImageExist) {
        //
        // After user authentication, the deferred drivers was loaded again.
        // Here, need to ensure the deferred drivers are connected.
        //
        EfiBootManagerConnectAllDefaultConsoles ();
        ConnectSequence ();
      }
    }

    //
    // Here we have enough time to do the enumeration of boot device
    //
    EfiBootManagerRefreshAllBootOption ();
    EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, CompareBootOption);
    break;
  }

  //
  // Lock regions and config of SPI flash.
  //
  PlatformFlashLockPolicy (TRUE);

  //
  // NOTE: We need install DxeSmmReadyToLock directly here because many boot script is added via ExitPmAuth callback.
  // If we install them at same callback, these boot script will be rejected because BootScript Driver runs first to lock them done.
  // So we seperate them to be 2 different events, ExitPmAuth is last chance to let platform add boot script. DxeSmmReadyToLock will
  // make boot script save driver lock down the interface.
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiDxeSmmReadyToLockProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  if(!FeaturePcdGet (PcdEnableSecureLock) || (SecureBootEnabled)) {
    //
    // Print menu in unsecure builds or when UEFI SecureBoot enabled.
    //
    PrintBootPrompt ();
  }

  //
  // Signal a event before entering Setup.
  //
  DEBUG ((EFI_D_INFO, "Before Entering Setup...\n"));
  SignalProtocolEvent(&gSignalBeforeEnterSetupGuid);

  SecureLockBoot ();

  //
  // If SecureBoot then validate its vars for this platform.
  //
  PlatformValidateSecureBootVars ();

  return PLATFORM_BOOT_MANAGER_ENABLE_ALL;
}

/**
  Connect the predefined platform default authentication devices.

  This function connects the predefined device path for authentication device,
  and if the predefined device path has child device path, the child handle will
  be connected too. But the child handle of the child will not be connected.

**/
VOID
EFIAPI
ConnectAuthDevice (
  VOID
  )
{
  EFI_STATUS                   Status;
  UINTN                        Index;
  UINTN                        HandleIndex;
  UINTN                        HandleCount;
  EFI_HANDLE                   *HandleBuffer;
  EFI_DEVICE_PATH_PROTOCOL     *ChildDevicePath;
  EFI_USER_MANAGER_PROTOCOL    *Manager;

  Status = gBS->LocateProtocol (
                  &gEfiUserManagerProtocolGuid,
                  NULL,
                  (VOID **) &Manager
                  );
  if (EFI_ERROR (Status)) {
    //
    // As user manager protocol is not installed, the authentication devices
    // should not be connected.
    //
    return ;
  }

  Index = 0;
  while (gUserAuthenticationDevice[Index] != NULL) {
    //
    // Connect the platform customized device paths
    //
    EfiBootManagerConnectDevicePath (gUserAuthenticationDevice[Index], NULL);
    Index++;
  }

  //
  // Find and connect the child device paths of the platform customized device paths
  //
  HandleBuffer = NULL;
  for (Index = 0; gUserAuthenticationDevice[Index] != NULL; Index++) {
    HandleCount = 0;
    Status = gBS->LocateHandleBuffer (
                    AllHandles,
                    NULL,
                    NULL,
                    &HandleCount,
                    &HandleBuffer
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Find and connect the child device paths of gUserIdentificationDevice[Index]
    //
    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
      ChildDevicePath = NULL;
      Status = gBS->HandleProtocol (
                      HandleBuffer[HandleIndex],
                      &gEfiDevicePathProtocolGuid,
                      (VOID **) &ChildDevicePath
                      );
      if (EFI_ERROR (Status) || ChildDevicePath == NULL) {
        continue;
      }

      if (CompareMem (
            ChildDevicePath,
            gUserAuthenticationDevice[Index],
            (GetDevicePathSize (gUserAuthenticationDevice[Index]) - sizeof (EFI_DEVICE_PATH_PROTOCOL))
            ) != 0) {
        continue;
      }
      gBS->ConnectController (HandleBuffer[HandleIndex], NULL, NULL, TRUE);
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }
}

/**
  This function is to identify a user, and return whether deferred images exist.

  @param[out]  User               Point to user profile handle.
  @param[out]  DeferredImageExist On return, points to TRUE if the deferred image
                                  exist or FALSE if it did not exist.

**/
VOID
CheckDeferredImage (
  OUT EFI_USER_PROFILE_HANDLE        *User,
  OUT BOOLEAN                        *DeferredImageExist
  )
{
  EFI_STATUS                         Status;
  EFI_DEFERRED_IMAGE_LOAD_PROTOCOL   *DeferredImage;
  UINTN                              HandleCount;
  EFI_HANDLE                         *HandleBuf;
  UINTN                              Index;
  UINTN                              DriverIndex;
  EFI_DEVICE_PATH_PROTOCOL           *ImageDevicePath;
  VOID                               *DriverImage;
  UINTN                              ImageSize;
  BOOLEAN                            BootOption;

  //
  // Perform user identification
  //
  do {
    Status = UserIdentify (User);
  } while (EFI_ERROR (Status));

  //
  // After user authentication now, try to find whether deferred images exists
  //
  HandleCount = 0;
  HandleBuf   = NULL;
  *DeferredImageExist = FALSE;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDeferredImageLoadProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuf
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuf[Index],
                    &gEfiDeferredImageLoadProtocolGuid,
                    (VOID **) &DeferredImage
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Find whether deferred image exists in this instance.
      //
      DriverIndex = 0;
      Status = DeferredImage->GetImageInfo(
                                DeferredImage,
                                DriverIndex,
                                &ImageDevicePath,
                                (VOID **) &DriverImage,
                                &ImageSize,
                                &BootOption
                                );
      if (!EFI_ERROR (Status)) {
        //
        // The deferred image is found.
        //
        FreePool (HandleBuf);
        *DeferredImageExist = TRUE;
        return ;
      }
    }
  }

  FreePool (HandleBuf);
}

/** Constructor for this lib.

  Tasks:
    Fixup globals with PCD values.

  @param  ImageHandle  ImageHandle of the loaded driver.
  @param  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS            Register successfully.
**/
EFI_STATUS
EFIAPI
PlatformBootManagerLibConstructor (
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_SYSTEM_TABLE                     *SystemTable
  )
{
  EFI_STATUS                        Status;

  DEBUG((EFI_D_INFO, "PlatformBootManagerLibConstructor:>>\n"));
  gSerialDevicePath.SerialDevice.Function = PcdGet8 (PcdIohUartFunctionNumber);
  gSerialDevicePath.SerialDevice.Device = PcdGet8 (PcdIohUartDevNumber);
  gSerialDevicePath.Uart.BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  gSerialDevicePath.Uart.DataBits = PcdGet8 (PcdUartDefaultDataBits);
  gSerialDevicePath.Uart.Parity   = PcdGet8 (PcdUartDefaultParity);
  gSerialDevicePath.Uart.StopBits = PcdGet8 (PcdUartDefaultStopBits);

  Status = gBS->LocateProtocol(
             &gSecureBootHelperProtocolGuid,
             NULL,
             (VOID**) &gSecureBootHelperProtocol
             );

  if (EFI_ERROR(Status)) {
    //
    // Helper needed for Secure lockdown builds.
    //
    gSecureBootHelperProtocol = NULL;
  }

  //
  // Get reference to platform type protocol.
  //
  Status = gBS->LocateProtocol (&gEfiPlatformTypeProtocolGuid, NULL, (VOID **) &gPlatformType);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
