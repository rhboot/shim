/** @file
  This module produce main entry for BDS phase - BdsEntry.
  When this module was dispatched by DxeCore, gEfiBdsArchProtocolGuid will be installed
  which contains interface of BdsEntry.
  After DxeCore finish DXE phase, gEfiBdsArchProtocolGuid->BdsEntry will be invoked
  to enter BDS phase.

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

#include "Bds.h"
#include "Language.h"
#include "HwErrRecSupport.h"

#define SET_BOOT_OPTION_SUPPORT_KEY_COUNT(a, c) {  \
      (a) = ((a) & ~EFI_BOOT_OPTION_SUPPORT_COUNT) | (((c) << LowBitSet32 (EFI_BOOT_OPTION_SUPPORT_COUNT)) & EFI_BOOT_OPTION_SUPPORT_COUNT); \
      }

//TODO: BdsDxe driver produces BDS ARCH protocol but also BDS UTILITY protocol to replace UefiBootManagerLib.
///
/// BDS arch protocol instance initial value.
///
EFI_BDS_ARCH_PROTOCOL  gBds = {
  BdsEntry
};

//
// gConnectConInEvent - Event which is signaled when ConIn connection is required
//
EFI_EVENT      gConnectConInEvent = NULL;


/**
  Event to Connect ConIn.

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               Pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
BdsDxeOnConnectConInCallBack (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  EFI_STATUS Status;

  //
  // When Osloader call ReadKeyStroke to signal this event
  // no driver dependency is assumed existing. So use a non-dispatch version
  //
  Status = EfiBootManagerConnectConsoleVariable (ConIn, FALSE);
  if (EFI_ERROR (Status)) {
    //
    // Should not enter this case, if enter, the keyboard will not work.
    // May need platfrom policy to connect keyboard.
    //
    DEBUG ((EFI_D_WARN, "[Bds] ASSERT Connect ConIn failed!!!\n"));
  }
}

/**

  Install Boot Device Selection Protocol

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  BDS has finished initializing.
                        Return the dispatcher and recall BDS.Entry
  @retval  Other        Return status from AllocatePool() or gBS->InstallProtocolInterface

**/
EFI_STATUS
EFIAPI
BdsInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;
  //
  // Install protocol interface
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiBdsArchProtocolGuid, &gBds,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Callback function to change the L"Timeout" variable.
**/
VOID
SetNoWaiting (
  IN OUT VOID  **Variable,
  IN OUT UINTN *VariableSize
  )
{
  UINT16       *Timeout;

  ASSERT ((*VariableSize == sizeof (UINT16)) ||
          (*Variable == NULL && *VariableSize == 0)
         );
  Timeout  = AllocatePool (sizeof (UINT16));
  ASSERT (Timeout != NULL);
  *Timeout = 0;

  *Variable     = Timeout;
  *VariableSize = sizeof (UINT16);
}

/**
  Generic function to update the EFI global variables.

  @param VariableName    The name of the variable to be updated.
  @param ProcessVariable The function pointer to update the variable.
                         NULL means to restore to the original value.
**/
VOID
BdsUpdateEfiGlobalVariable (
  CHAR16               *VariableName,
  BDS_PROCESS_VARIABLE ProcessVariable
  )
{
  EFI_STATUS  Status;
  CHAR16      BackupVariableName[20];
  CHAR16      FlagVariableName[20];
  VOID        *Variable;
  VOID        *BackupVariable;
  VOID        *NewVariable;
  UINTN       VariableSize;
  UINTN       BackupVariableSize;
  UINTN       NewVariableSize;
  BOOLEAN     Flag;
  BOOLEAN     *FlagVariable;
  UINTN       FlagSize;

  ASSERT (StrLen (VariableName) <= 13);
  UnicodeSPrint (BackupVariableName, sizeof (BackupVariableName), L"%sBackup", VariableName);
  UnicodeSPrint (FlagVariableName, sizeof (FlagVariableName), L"%sModify", VariableName);

  Variable       = EfiBootManagerGetVariableAndSize (VariableName, &gEfiGlobalVariableGuid, &VariableSize);
  BackupVariable = EfiBootManagerGetVariableAndSize (BackupVariableName, &gEfiCallerIdGuid, &BackupVariableSize);
  FlagVariable   = EfiBootManagerGetVariableAndSize (FlagVariableName, &gEfiCallerIdGuid, &FlagSize);
  if ((ProcessVariable != NULL) && (FlagVariable == NULL)) {
    //
    // Current boot is a modified boot and last boot is a normal boot
    // Set flag to indicate it's a modified boot
    // BackupVariable <- Variable
    // Variable       <- ProcessVariable (Variable)
    //
    Flag   = TRUE;
    Status = gRT->SetVariable (
                    FlagVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof (Flag),
                    &Flag
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gRT->SetVariable (
                    BackupVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    VariableSize,
                    Variable
                    );
    ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));


    NewVariable     = Variable;
    NewVariableSize = VariableSize;
    ProcessVariable (&NewVariable, &NewVariableSize);

    Status = gRT->SetVariable (
                    VariableName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    NewVariableSize,
                    NewVariable
                    );
    ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));

    if (NewVariable != NULL) {
      FreePool (NewVariable);
    }
  } else if ((ProcessVariable == NULL) && (FlagVariable != NULL)) {
    //
    // Current boot is a normal boot and last boot is a modified boot
    // Clear flag to indicate it's a normal boot
    // Variable       <- BackupVariable
    // BackupVariable <- NULL
    //
    Status = gRT->SetVariable (
                    FlagVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    0,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gRT->SetVariable (
                    VariableName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    BackupVariableSize,
                    BackupVariable
                    );
    ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));

    Status = gRT->SetVariable (
                    BackupVariableName,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    0,
                    NULL
                    );
    ASSERT ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND));
  }

  if (Variable != NULL) {
    FreePool (Variable);
  }

  if (BackupVariable != NULL) {
    FreePool (BackupVariable);
  }

  if (FlagVariable != NULL) {
    FreePool (FlagVariable);
  }
}


/**
  Function waits for a given event to fire, or for an optional timeout to expire.

  @param   Event              The event to wait for
  @param   Timeout            An optional timeout value in 100 ns units.

  @retval  EFI_SUCCESS      Event fired before Timeout expired.
  @retval  EFI_TIME_OUT     Timout expired before Event fired..

**/
EFI_STATUS
BdsWaitForSingleEvent (
  IN  EFI_EVENT                  Event,
  IN  UINT64                     Timeout       OPTIONAL
  )
{
  UINTN       Index;
  EFI_STATUS  Status;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[2];

  if (Timeout != 0) {
    //
    // Create a timer event
    //
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      //
      // Set the timer event
      //
      gBS->SetTimer (
             TimerEvent,
             TimerRelative,
             Timeout
             );

      //
      // Wait for the original event or the timer
      //
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status      = gBS->WaitForEvent (2, WaitList, &Index);
      ASSERT_EFI_ERROR (Status);
      gBS->CloseEvent (TimerEvent);

      //
      // If the timer expired, change the return to timed out
      //
      if (Index == 1) {
        Status = EFI_TIMEOUT;
      }
    }
  } else {
    //
    // No timeout... just wait on the event
    //
    Status = gBS->WaitForEvent (1, &Event, &Index);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (Index == 0);
  }

  return Status;
}

/**
  The function reads user inputs.

**/

VOID
BdsReadKeys (
  VOID
  )
{
  EFI_STATUS         Status;
  EFI_INPUT_KEY      Key;
  while (gST->ConIn != NULL) {
    
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    
    if (EFI_ERROR (Status)) {
      //
      // No more keys.
      //
      break;
    }
  }
}

/**
  The function waits for the boot manager timeout expires or hotkey is pressed.

  It calls PlatformBootManagerWaitCallback each second.
**/
VOID
BdsWait (
  IN EFI_EVENT      HotkeyTriggered
  )
{
  EFI_STATUS            Status;
  UINT16                TimeoutRemain;

  DEBUG ((EFI_D_INFO, "[Bds]BdsWait ...Zzzzzzzzzzzz...\n"));

  TimeoutRemain = PcdGet16 (PcdPlatformBootTimeOut);
  while (TimeoutRemain != 0) {
    DEBUG ((EFI_D_INFO, "[Bds]BdsWait(%d)..Zzzz...\n", (UINTN) TimeoutRemain));
    PlatformBootManagerWaitCallback (TimeoutRemain);
    if (!PcdGetBool (PcdConInConnectOnDemand)) {
      BdsReadKeys (); // BUGBUG: Only reading can signal HotkeyTriggered
                      //   Can be removed after all keyboard drivers invoke callback in timer callback.
    }

    if (HotkeyTriggered != NULL) {
      Status = BdsWaitForSingleEvent (HotkeyTriggered, EFI_TIMER_PERIOD_SECONDS (1));
      if (!EFI_ERROR (Status)) {
        break;
      }
    } else {
      gBS->Stall (1000000);
    }

    //
    // 0xffff means waiting forever
    // BDS with no hotkey provided and 0xffff as timeout will "hang" in the loop
    //
    if (TimeoutRemain != 0xffff) {
      TimeoutRemain--;
    }
  }
  DEBUG ((EFI_D_INFO, "[Bds]Exit the waiting!\n"));
}

/**
  Attempt to boot each boot option in the BootOptions array.

  @retval TRUE  Successfully boot one of the boot options.
  @retval FALSE Failed boot any of the boot options.
**/
BOOLEAN
BootAllBootOptions (
  IN EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptions,
  IN UINTN                           BootOptionCount
  )
{
  UINTN                              Index;

  //
  // Attempt boot each boot option
  //
  for (Index = 0; Index < BootOptionCount; Index++) {
    //
    // According to EFI Specification, if a load option is not marked
    // as LOAD_OPTION_ACTIVE, the boot manager will not automatically
    // load the option.
    //
    if ((BootOptions[Index].Attributes & LOAD_OPTION_ACTIVE) == 0) {
      continue;
    }

    //
    // All the driver options should have been processed since
    // now boot will be performed.
    //
    EfiBootManagerBoot (&BootOptions[Index]);

    //
    // Successful boot breaks the loop, otherwise tries next boot option
    //
    if (BootOptions[Index].Status == EFI_SUCCESS) {
      break;
    }
  }

  return (BOOLEAN) (Index < BootOptionCount);
}

/**
  This function attempts to boot per the boot order specified by platform policy.

  If the boot via Boot#### returns with a status of EFI_SUCCESS the boot manager will stop 
  processing the BootOrder variable and present a boot manager menu to the user. If a boot via 
  Boot#### returns a status other than EFI_SUCCESS, the boot has failed and the next Boot####
  in the BootOrder variable will be tried until all possibilities are exhausted.
                                  -- Chapter 3.1.1 Boot Manager Programming, the 4th paragraph
**/
VOID
DefaultBootBehavior (
  VOID
  )
{
  EFI_STATUS                   Status;
  UINTN                        BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOptions;
  EFI_BOOT_MANAGER_LOAD_OPTION BootManagerMenu;

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  if (BootAllBootOptions (BootOptions, BootOptionCount)) {
    //
    // Follow generic rule, Call BdsDxeOnConnectConInCallBack to connect ConIn before enter UI
    //
    if (PcdGetBool (PcdConInConnectOnDemand)) {
      BdsDxeOnConnectConInCallBack (NULL, NULL);
    }

    //
    // Show the Boot Manager Menu after successful boot
    //
    Status = EfiBootManagerGetBootManagerMenu (&BootManagerMenu);
    ASSERT_EFI_ERROR (Status);
    EfiBootManagerBoot (&BootManagerMenu);
    EfiBootManagerFreeLoadOption (&BootManagerMenu);
  } else {
    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
    //
    // Re-scan all EFI boot options in case all the boot#### are deleted or failed to boot
    //
    // TODO: Can move the below connect all to EfiBootManagerEnumerateBootOptions
    EfiBootManagerConnectAll ();
    BootOptions = EfiBootManagerEnumerateBootOptions (&BootOptionCount);

    if (!BootAllBootOptions (BootOptions, BootOptionCount)) {
      DEBUG ((EFI_D_ERROR, "[Bds]No bootable device!\n"));
      ASSERT_EFI_ERROR (EFI_NOT_FOUND);
    }
  }

  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
}

/**
  The function will go through the driver option link list, load and start
  every driver the driver option device path point to.

  @param  BdsDriverLists        The header of the current driver option link list

**/
VOID
LoadDrivers (
  IN EFI_BOOT_MANAGER_LOAD_OPTION       *DriverOption,
  IN UINTN                              DriverOptionCount
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  EFI_HANDLE                ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
  BOOLEAN                   ReconnectAll;

  ReconnectAll = FALSE;

  //
  // Process the driver option
  //
  for (Index = 0; Index < DriverOptionCount; Index++) {
    //
    // If a load option is not marked as LOAD_OPTION_ACTIVE,
    // the boot manager will not automatically load the option.
    //
    if ((DriverOption[Index].Attributes & LOAD_OPTION_ACTIVE) == 0) {
      continue;
    }
    
    //
    // If a driver load option is marked as LOAD_OPTION_FORCE_RECONNECT,
    // then all of the EFI drivers in the system will be disconnected and
    // reconnected after the last driver load option is processed.
    //
    if ((DriverOption[Index].Attributes & LOAD_OPTION_FORCE_RECONNECT) != 0) {
      ReconnectAll = TRUE;
    }
    
    //
    // Make sure the driver path is connected.
    //
    EfiBootManagerConnectDevicePath (DriverOption[Index].FilePath, NULL);

    //
    // Load and start the image that Driver#### describes
    //
    Status = gBS->LoadImage (
                    FALSE,
                    gImageHandle,
                    DriverOption[Index].FilePath,
                    NULL,
                    0,
                    &ImageHandle
                    );

    if (!EFI_ERROR (Status)) {
      gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &ImageInfo);

      //
      // Verify whether this image is a driver, if not,
      // exit it and continue to parse next load option
      //
      if (ImageInfo->ImageCodeType != EfiBootServicesCode && ImageInfo->ImageCodeType != EfiRuntimeServicesCode) {
        gBS->Exit (ImageHandle, EFI_INVALID_PARAMETER, 0, NULL);
        continue;
      }

      ImageInfo->LoadOptionsSize  = DriverOption[Index].OptionalDataSize;
      ImageInfo->LoadOptions      = DriverOption[Index].OptionalData;
      //
      // Before calling the image, enable the Watchdog Timer for
      // the 5 Minute period
      //
      gBS->SetWatchdogTimer (5 * 60, 0x0000, 0x00, NULL);

      DriverOption[Index].Status = gBS->StartImage (ImageHandle, &DriverOption[Index].ExitDataSize, &DriverOption[Index].ExitData);
      DEBUG ((DEBUG_INFO | DEBUG_LOAD, "Driver Return Status = %r\n", DriverOption[Index].Status));

      //
      // Clear the Watchdog Timer after the image returns
      //
      gBS->SetWatchdogTimer (0x0000, 0x0000, 0x0000, NULL);
    }
  }
  
  //
  // Process the LOAD_OPTION_FORCE_RECONNECT driver option
  //
  if (ReconnectAll) {
    EfiBootManagerDisconnectAll ();
    EfiBootManagerConnectAll ();
  }

}

/**

  Validate input console variable data. 

  If found the device path is not a valid device path, remove the variable.
  
  @param VariableName             Input console variable name.

**/
VOID
BdsFormalizeConsoleVariable (
  IN  CHAR16          *VariableName
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     VariableSize;
  EFI_STATUS                Status;

  DevicePath = EfiBootManagerGetVariableAndSize (
                      VariableName,
                      &gEfiGlobalVariableGuid,
                      &VariableSize
                      );
  if ((DevicePath != NULL) && !IsDevicePathValid (DevicePath, VariableSize)) { 
    Status = gRT->SetVariable (
                    VariableName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    0,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Formalize OsIndication related variables. 
  
  For OsIndicationsSupported, Create a BS/RT/UINT64 variable to report caps 
  Delete OsIndications variable if it is not NV/BS/RT UINT64.
  
  Item 3 is used to solve case when OS corrupts OsIndications. Here simply delete this NV variable.

**/
VOID 
BdsFormalizeOSIndicationVariable (
  VOID
  )
{
  EFI_STATUS Status;
  UINT64     OsIndicationSupport;
  UINT64     OsIndication;
  UINTN      DataSize;
  UINT32     Attributes;

  //
  // OS indicater support variable
  //
  OsIndicationSupport = EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
  Status = gRT->SetVariable (
                  L"OsIndicationsSupported",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof(UINT64),
                  &OsIndicationSupport
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // If OsIndications is invalid, remove it.
  // Invalid case
  //   1. Data size != UINT64
  //   2. OsIndication value inconsistence
  //   3. OsIndication attribute inconsistence
  //
  OsIndication = 0;
  Attributes = 0;
  DataSize = sizeof(UINT64);
  Status = gRT->GetVariable (
                  L"OsIndications",
                  &gEfiGlobalVariableGuid,
                  &Attributes,
                  &DataSize,
                  &OsIndication
                  );
  if (Status == EFI_NOT_FOUND) {
    return;
  }

  if (DataSize != sizeof(UINT64) ||
      (OsIndication & ~OsIndicationSupport) != 0 ||
      Attributes != (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE)){

    DEBUG ((EFI_D_ERROR, "Unformalized OsIndications variable exists. Delete it\n"));
    Status = gRT->SetVariable (
                    L"OsIndications",
                    &gEfiGlobalVariableGuid,
                    Attributes,
                    0,
                    NULL
                    );
    ASSERT_EFI_ERROR(Status);
  }
}

/**

  Validate variables. 

**/
VOID 
BdsFormalizeEfiGlobalVariable (
  VOID
  )
{
  //
  // Validate Console variable.
  //
  BdsFormalizeConsoleVariable (L"ConIn");
  BdsFormalizeConsoleVariable (L"ConOut");
  BdsFormalizeConsoleVariable (L"ErrOut");

  //
  // Validate OSIndication related variable.
  //
  BdsFormalizeOSIndicationVariable ();
}

/**

  Service routine for BdsInstance->Entry(). Devices are connected, the
  consoles are initialized, and the boot options are tried.

  @param This             Protocol Instance structure.

**/
VOID
EFIAPI
BdsEntry (
  IN EFI_BDS_ARCH_PROTOCOL  *This
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION    *DriverOption;
  EFI_BOOT_MANAGER_LOAD_OPTION    BootOption;
  UINTN                           DriverOptionCount;
  CHAR16                          *FirmwareVendor;
  EFI_EVENT                       HotkeyTriggered;
  UINT32                          PlatformBdsRequest;
  UINT64                          OsIndication;
  UINTN                           DataSize;
  EFI_STATUS                      Status;
  UINT32                          BootOptionSupport;


  HotkeyTriggered = NULL;
  Status          = EFI_SUCCESS;

  //
  // Insert the performance probe
  //
  PERF_END (NULL, "DXE", NULL, 0);
  PERF_START (NULL, "BDS", NULL, 0);
  DEBUG ((EFI_D_INFO, "[Bds] Entry...\n"));

  //
  // Fill in FirmwareVendor and FirmwareRevision from PCDs
  //
  FirmwareVendor = (CHAR16 *) PcdGetPtr (PcdFirmwareVendor);
  gST->FirmwareVendor = AllocateRuntimeCopyPool (StrSize (FirmwareVendor), FirmwareVendor);
  ASSERT (gST->FirmwareVendor != NULL);
  gST->FirmwareRevision = PcdGet32 (PcdFirmwareRevision);

  //
  // Fixup Tasble CRC after we updated Firmware Vendor and Revision
  //
  gST->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 ((VOID *) gST, sizeof (EFI_SYSTEM_TABLE), &gST->Hdr.CRC32);

  //
  // Validate Variable.
  //
  BdsFormalizeEfiGlobalVariable();

  InitializeHwErrRecSupport ();

  //
  // Initialize the platform language variables
  //
  InitializeLanguage (TRUE);

  //
  // Initialize hotkey service
  // Do not report the hotkey capability if PcdConInConnectOnDemand is enabled.
  //
  if (!PcdGetBool (PcdConInConnectOnDemand)) {
    EfiBootManagerStartHotkeyService (&HotkeyTriggered);
  }

  //
  // Report Status Code to indicate connecting drivers will happen
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_BEGIN_CONNECTING_DRIVERS)
    );

  //
  // Do the platform init, can be customized by OEM/IBV
  // Possible things that can be done in PlatformBdsInit:
  // > Update console variable: 1. include hot-plug devices; 2. Clear ConIn and add SOL for AMT
  // > Register new Driver#### or Boot####
  // > Register new Key####: e.g.: F12 
  // > Signal ReadyToLock event
  // > Authentication action: 1. connect Auth devices; 2. Identify auto logon user.
  //
  PERF_START (NULL, "PlatformBootManagerBeforeConsole", "BDS", 0);
  PlatformBootManagerBeforeConsole ();
  PERF_END   (NULL, "PlatformBootManagerBeforeConsole", "BDS", 0);

  //
  // Load Driver Options
  //
  DriverOption = EfiBootManagerGetLoadOptions (&DriverOptionCount, LoadOptionTypeDriver);
  LoadDrivers (DriverOption, DriverOptionCount);
  EfiBootManagerFreeLoadOptions (DriverOption, DriverOptionCount);
  
  //
  // Connect consoles
  //
  PERF_START (NULL, "EfiBootManagerConnectAllDefaultConsoles", "BDS", 0);
  if (PcdGetBool (PcdConInConnectOnDemand)) {
    EfiBootManagerConnectConsoleVariable (ConOut, TRUE);
    EfiBootManagerConnectConsoleVariable (ErrOut, TRUE);

    //
    // Initialize ConnectConIn event
    //
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    BdsDxeOnConnectConInCallBack,
                    NULL,
                    &gConnectConInEventGuid,
                    &gConnectConInEvent
                    );
    if (EFI_ERROR (gConnectConInEvent)) {
      gConnectConInEvent = NULL;
    }
  } else {
    EfiBootManagerConnectAllDefaultConsoles();
  }
  PERF_END   (NULL, "EfiBootManagerConnectAllDefaultConsoles", "BDS", 0);

  //
  // Do the platform specific action after the console is ready
  // Possible things that can be done in PlatformBootManagerConsoleReady:
  // > Console post action:
  //   > Dynamically switch output mode from 100x31 to 80x25 for AMT SOL
  //   > Signal console ready platform customized event
  // > Run diagnostics: 1. driver health check; 2. memory test
  // > Connect certain devices: 1. connect all; 2. connect nothing; 3. connect platform specified devices
  // > Dispatch aditional option roms
  // > Special boot: e.g.: AMT boot, enter UI
  //
  // NOTE: New boot options could appear after connecting certain devices.
  // WHEN to enumerate the boot options? Ideally, only do the enumeration in FULL_CONFIGURATION boot mode. 
  //  -> if so, the boot option enumeration can be done in BdsDxe driver
  // BUT 1. user may plug in new HD/remove HD; 2. PXE enable/disable; 3. SATA mode (IDE/AHCI/RAID) need to enumerate the boot option again.
  // 1 can be detected by chassis intrusion, 2 & 3 can be handled by code.
  // BDS assumes platform code should set correct boot mode.
  // 
  PERF_START (NULL, "PlatformBootManagerAfterConsole", "BDS", 0);
  PlatformBdsRequest = PlatformBootManagerAfterConsole ();
  PERF_END   (NULL, "PlatformBootManagerAfterConsole", "BDS", 0);

  if (PcdGetBool (PcdConInConnectOnDemand)) {
    PlatformBdsRequest &= ~PLATFORM_BOOT_MANAGER_ENABLE_HOTKEY;
  }

  //
  // Clear/Restore Key####/Timeout per platform request
  // For Timeout, variable change is enough to control the following behavior
  // For Key####, variable change is not enough so we still need to check the *_ENABLE_HOTKEY later
  //
  if ((PlatformBdsRequest & PLATFORM_BOOT_MANAGER_ENABLE_TIMEOUT) == PLATFORM_BOOT_MANAGER_ENABLE_TIMEOUT) {
    BdsUpdateEfiGlobalVariable (L"Timeout", NULL);
  } else {
    BdsUpdateEfiGlobalVariable (L"Timeout", SetNoWaiting);
  }

  //
  // Export our capability - EFI_BOOT_OPTION_SUPPORT_KEY and EFI_BOOT_OPTION_SUPPORT_APP
  // with maximum number of key presses of 3
  //
  BootOptionSupport = EFI_BOOT_OPTION_SUPPORT_APP;
  if ((PlatformBdsRequest & PLATFORM_BOOT_MANAGER_ENABLE_HOTKEY) == PLATFORM_BOOT_MANAGER_ENABLE_HOTKEY) {
    BootOptionSupport |= EFI_BOOT_OPTION_SUPPORT_KEY;
    SET_BOOT_OPTION_SUPPORT_KEY_COUNT (BootOptionSupport, 3);
  }
  Status = gRT->SetVariable (
                  L"BootOptionSupport",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (UINT32),
                  &BootOptionSupport
                  );
  ASSERT_EFI_ERROR (Status);

  DEBUG_CODE (
    EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptions;
    UINTN                           BootOptionCount;
    UINTN                           Index;

    BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
    DEBUG ((EFI_D_INFO, "[Bds]=============Dumping Boot Options=============\n"));
    for (Index = 0; Index < BootOptionCount; Index++) {
      DEBUG ((
        EFI_D_INFO, "[Bds]Boot%04x: %s \t\t 0x%04x\n",
        BootOptions[Index].OptionNumber, 
        BootOptions[Index].Description, 
        BootOptions[Index].Attributes
        ));
    }
    DEBUG ((EFI_D_INFO, "[Bds]=============Dumping Boot Options Finished====\n"));
    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
    );

  //
  // goto FrontPage directly when EFI_OS_INDICATIONS_BOOT_TO_FW_UI is set
  //
  DataSize = sizeof(UINT64);
  Status = gRT->GetVariable (
                  L"OsIndications",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &OsIndication
                  );

  //
  // goto FrontPage directly when EFI_OS_INDICATIONS_BOOT_TO_FW_UI is set. Skip HotkeyBoot
  //
  if (!EFI_ERROR(Status) && ((OsIndication & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) != 0)) {
    //
    // Clear EFI_OS_INDICATIONS_BOOT_TO_FW_UI to acknowledge OS
    // 
    OsIndication &= ~EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
    Status = gRT->SetVariable (
               L"OsIndications",
               &gEfiGlobalVariableGuid,
               EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
               sizeof(UINT64),
               &OsIndication
               ); 
    ASSERT_EFI_ERROR (Status);

    //
    // Follow generic rule, Call BdsDxeOnConnectConInCallBack to connect ConIn before enter UI
    //
    if (PcdGetBool (PcdConInConnectOnDemand)) {
      BdsDxeOnConnectConInCallBack (NULL, NULL);
    }

    //
    // Directly enter the setup page.
    //
    Status = EfiBootManagerGetBootManagerMenu (&BootOption);
    ASSERT_EFI_ERROR (Status);
    EfiBootManagerBoot (&BootOption);
    EfiBootManagerFreeLoadOption (&BootOption);
  } else {
    PERF_START (NULL, "BdsWait", "BDS", 0);
    BdsWait (HotkeyTriggered);
    PERF_END   (NULL, "BdsWait", "BDS", 0);

    if (!PcdGetBool (PcdConInConnectOnDemand)) {
      //
      // No need to check hotkey boot if hotkey service is disabled.
      //
      if ((PlatformBdsRequest & PLATFORM_BOOT_MANAGER_ENABLE_HOTKEY) == PLATFORM_BOOT_MANAGER_ENABLE_HOTKEY) {
        //
        // BUGBUG: Only reading can trigger the notification function
        // Can be removed after all keyboard drivers are updated to invoke callback in timer callback.
        //
        BdsReadKeys ();

        EfiBootManagerHotkeyBoot ();
      }
    }
  }

  while (TRUE) {
    //
    // BDS select the boot device to load OS
    // Try next upon boot failure
    // Show Boot Manager Menu upon boot success
    //
    DefaultBootBehavior ();
  }
}
