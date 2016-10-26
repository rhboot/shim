/** @file
  BDS Lib functions which contain all the code to connect console device

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

#include "InternalBdsLib.h"

CHAR16       *mConVarName[] = {
  L"ConIn",
  L"ConOut",
  L"ErrOut",
  L"ConInDev",
  L"ConOutDev",
  L"ErrOutDev"
};

/**
  Search out the video controller.

  @return  PCI device path of the video controller.
**/
EFI_HANDLE
GetVideoController (
  VOID
  )
{
  EFI_STATUS                Status;
  UINTN                     RootBridgeHandleCount;
  EFI_HANDLE                *RootBridgeHandleBuffer;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     RootBridgeIndex;
  UINTN                     Index;
  EFI_HANDLE                VideoController;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

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

  VideoController = NULL;
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
          // TODO: use IS_PCI_DISPLAY??
          VideoController = HandleBuffer[Index];
          break;
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

/**
  Query all the children of VideoController and return the device paths of all the 
  children that support GraphicsOutput protocol.

  @param VideoController       PCI handle of video controller.

  @return  Device paths of all the children that support GraphicsOutput protocol.
**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiBootManagerGetGopDevicePath (
  IN  EFI_HANDLE               VideoController
  )
{
  UINTN                                Index;
  EFI_STATUS                           Status;
  EFI_DEVICE_PATH_PROTOCOL             *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL             *ChildDevicePath;
  EFI_DEVICE_PATH_PROTOCOL             *GopPool;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  UINTN                                EntryCount;

  ASSERT (VideoController != NULL);

  GopPool = NULL;

  Status = gBS->OpenProtocolInformation (
                  VideoController,
                  &gEfiPciIoProtocolGuid,
                  &OpenInfoBuffer,
                  &EntryCount
                  );
  if (EFI_ERROR (Status)) {
    EntryCount = 0;
  }

  for (Index = 0; Index < EntryCount; Index++) {
    //
    // Query all the children created by the GOP driver
    //
    if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
      Status = gBS->OpenProtocol (
                      OpenInfoBuffer[Index].ControllerHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID **) &ChildDevicePath,
                      NULL,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (!EFI_ERROR (Status)) {
        Status = gBS->OpenProtocol (
                        OpenInfoBuffer[Index].ControllerHandle,
                        &gEfiGraphicsOutputProtocolGuid,
                        NULL,
                        NULL,
                        NULL,
                        EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                        );
        if (!EFI_ERROR (Status)) {
          //
          // Append the device path to GOP pool when there is GOP protocol installed.
          //
          TempDevicePath = GopPool;
          GopPool = AppendDevicePathInstance (GopPool, ChildDevicePath);
          if (TempDevicePath != NULL) {
            FreePool (TempDevicePath);
          }
        }
      }
    }
  }
  FreePool (OpenInfoBuffer);

  return GopPool;
}

/**
  Find the platform active active video controller and connect it.

  @retval EFI_NOT_FOUND There is no active video controller.
  @retval EFI_SUCCESS   The video controller is connected.
**/
EFI_STATUS
ConnectVideoController (
  VOID
  )
{
  EFI_HANDLE                 VideoController;
  EFI_DEVICE_PATH_PROTOCOL   *Gop;
  
  //
  // Get the platform vga device
  //
  VideoController = GetVideoController ();
  
  if (VideoController == NULL) {
    return EFI_NOT_FOUND;
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
    //
    // Last chance - just try VGA controller
    // Don't assume all VGA controller support GOP operation
    //
    DEBUG ((EFI_D_ERROR, "[Bds] GOP not found, try VGA device path!\n"));
    Gop = DuplicateDevicePath (DevicePathFromHandle (VideoController));
    if (Gop == NULL) {
      return EFI_NOT_FOUND;
    }
  }

  EfiBootManagerUpdateConsoleVariable (ConOut, Gop, NULL);
  FreePool (Gop);

  //
  // Necessary for ConPlatform and ConSplitter driver to start up again after ConOut is updated.
  //
  return gBS->ConnectController (VideoController, NULL, NULL, TRUE);
}

/**
  Check if the video controller is connected.

  @retval TRUE  The video controller is connected.
  @retval FALSE The video controller isn't connected.
**/
BOOLEAN
HasLocalDisplay (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         HandleCount;

  HandleCount  = 0;
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleTextOutProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  while (HandleCount-- != 0) {
    Status = gBS->OpenProtocol (
                    HandleBuffer[HandleCount],
                    &gEfiGraphicsOutputProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      Status = gBS->OpenProtocol (
                      HandleBuffer[HandleCount],
                      &gEfiDevicePathProtocolGuid,
                      NULL,
                      NULL,
                      NULL,
                      EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                      );
      if (!EFI_ERROR (Status)) {
        return TRUE;
      }
    }
  }
  
  return FALSE;  
}

/**
  Fill console handle in System Table if there are no valid console handle in.

  Firstly, check the validation of console handle in System Table. If it is invalid,
  update it by the first console device handle from EFI console variable. 

  @param  VarName            The name of the EFI console variable.
  @param  ConsoleGuid        Specified Console protocol GUID.
  @param  ConsoleHandle      On IN,  console handle in System Table to be checked. 
                             On OUT, new console hanlde in system table.
  @param  ProtocolInterface  On IN,  console protocol on console handle in System Table to be checked. 
                             On OUT, new console protocol on new console hanlde in system table.

  @retval TRUE               System Table has been updated.
  @retval FALSE              System Table hasn't been updated.

**/
BOOLEAN 
UpdateSystemTableConsole (
  IN     CHAR16                          *VarName,
  IN     EFI_GUID                        *ConsoleGuid,
  IN OUT EFI_HANDLE                      *ConsoleHandle,
  IN OUT VOID                            **ProtocolInterface
  )
{
  EFI_STATUS                Status;
  UINTN                     DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *FullDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  VOID                      *Interface;
  EFI_HANDLE                NewHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *TextOut;

  ASSERT (VarName != NULL);
  ASSERT (ConsoleHandle != NULL);
  ASSERT (ConsoleGuid != NULL);
  ASSERT (ProtocolInterface != NULL);

  if (*ConsoleHandle != NULL) {
    Status = gBS->HandleProtocol (
                   *ConsoleHandle,
                   ConsoleGuid,
                   &Interface
                   );
    if (Status == EFI_SUCCESS && Interface == *ProtocolInterface) {
      //
      // If ConsoleHandle is valid and console protocol on this handle also
      // also matched, just return.
      //
      return FALSE;
    }
  }
  
  //
  // Get all possible consoles device path from EFI variable
  //
  VarConsole = EfiBootManagerGetVariableAndSize (
                 VarName,
                 &gEfiGlobalVariableGuid,
                 &DevicePathSize
                 );
  if (VarConsole == NULL) {
    //
    // If there is no any console device, just return.
    //
    return FALSE;
  }

  FullDevicePath = VarConsole;

  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&VarConsole, &DevicePathSize);
    if (Instance == NULL) {
      DEBUG ((EFI_D_ERROR, "[Bds] No valid console instance is found for %s!\n", VarName));
      // We should not ASSERT when all the console devices are removed.
      // ASSERT_EFI_ERROR (EFI_NOT_FOUND);
      FreePool (FullDevicePath);
      return FALSE;
    }
    
    //
    // Find console device handle by device path instance
    //
    Status = gBS->LocateDevicePath (
                    ConsoleGuid,
                    &Instance,
                    &NewHandle
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Get the console protocol on this console device handle
      //
      Status = gBS->HandleProtocol (
                      NewHandle,
                      ConsoleGuid,
                      &Interface
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Update new console handle in System Table.
        //
        *ConsoleHandle     = NewHandle;
        *ProtocolInterface = Interface;
        if (CompareGuid (ConsoleGuid, &gEfiSimpleTextOutProtocolGuid)) {
          //
          // If it is console out device, set console mode 80x25 if current mode is invalid.
          //
          TextOut = (EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *) Interface;
          if (TextOut->Mode->Mode == -1) {
            TextOut->SetMode (TextOut, 0);
          }
        }
        return TRUE;
      }
    }

  } while (Instance != NULL);

  //
  // No any available console devcie found.
  //
  return FALSE;
}

/**
  This function update console variable based on ConVarName, it can
  add or remove one specific console device path from the variable

  @param  ConVarName               Console related variable name, ConIn, ConOut,
                                   ErrOut.
  @param  CustomizedConDevicePath  The console device path which will be added to
                                   the console variable ConVarName, this parameter
                                   can not be multi-instance.
  @param  ExclusiveDevicePath      The console device path which will be removed
                                   from the console variable ConVarName, this
                                   parameter can not be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is same to the removed one.
  @retval EFI_SUCCESS              Success add or remove the device path from  the
                                   console variable.

**/
EFI_STATUS
EFIAPI
EfiBootManagerUpdateConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *VarConsole;
  UINTN                     DevicePathSize;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempNewDevicePath;

  if (ConsoleType >= sizeof (mConVarName) / sizeof (mConVarName[0])) {
    return EFI_INVALID_PARAMETER;
  }

  VarConsole      = NULL;
  DevicePathSize  = 0;

  //
  // Notes: check the device path point, here should check
  // with compare memory
  //
  if (CustomizedConDevicePath == ExclusiveDevicePath) {
    return EFI_UNSUPPORTED;
  }
  //
  // Delete the ExclusiveDevicePath from current default console
  //
  VarConsole = EfiBootManagerGetVariableAndSize (
                 mConVarName[ConsoleType],
                 &gEfiGlobalVariableGuid,
                 &DevicePathSize
                 );

  //
  // Initialize NewDevicePath
  //
  NewDevicePath = VarConsole;

  //
  // If ExclusiveDevicePath is even the part of the instance in VarConsole, delete it.
  // In the end, NewDevicePath is the final device path.
  //
  if (ExclusiveDevicePath != NULL && VarConsole != NULL) {
      NewDevicePath = EfiBootManagerDelPartMatchInstance (VarConsole, ExclusiveDevicePath);
  }
  //
  // Try to append customized device path to NewDevicePath.
  //
  if (CustomizedConDevicePath != NULL) {
    if (!EfiBootManagerMatchDevicePaths (NewDevicePath, CustomizedConDevicePath)) {
      //
      // Check if there is part of CustomizedConDevicePath in NewDevicePath, delete it.
      //
      NewDevicePath = EfiBootManagerDelPartMatchInstance (NewDevicePath, CustomizedConDevicePath);
      //
      // In the first check, the default console variable will be _ModuleEntryPoint,
      // just append current customized device path
      //
      TempNewDevicePath = NewDevicePath;
      NewDevicePath = AppendDevicePathInstance (NewDevicePath, CustomizedConDevicePath);
      if (TempNewDevicePath != NULL) {
        FreePool(TempNewDevicePath);
      }
    }
  }

  //
  // Finally, Update the variable of the default console by NewDevicePath
  //
  gRT->SetVariable (
         mConVarName[ConsoleType],
         &gEfiGlobalVariableGuid,
         EFI_VARIABLE_BOOTSERVICE_ACCESS
         | EFI_VARIABLE_RUNTIME_ACCESS
         | ((ConsoleType < ConInDev) ? EFI_VARIABLE_NON_VOLATILE : 0),
         GetDevicePathSize (NewDevicePath),
         NewDevicePath
         );

  if (VarConsole == NewDevicePath) {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
  } else {
    if (VarConsole != NULL) {
      FreePool(VarConsole);
    }
    if (NewDevicePath != NULL) {
      FreePool(NewDevicePath);
    }
  }

  return EFI_SUCCESS;

}


/**
  Connect the console device base on the variable ConsoleType.

  @param  ConsoleType              ConIn, ConOut or ErrOut.
  @param  NeedDispatch             Whether need to dispatch.

  @retval EFI_NOT_FOUND            There is not any console devices connected
                                   success
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType,
  IN  BOOLEAN                   NeedDispatch
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *StartDevicePath;
  UINTN                     VariableSize;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  EFI_DEVICE_PATH_PROTOCOL  *CopyOfDevicePath;
  UINTN                     Size;
  BOOLEAN                   DeviceExist;
  EFI_HANDLE                Handle;

  if ((ConsoleType != ConIn) && (ConsoleType != ConOut) && (ConsoleType != ErrOut)) {
    return EFI_INVALID_PARAMETER;
  }

  Status      = EFI_SUCCESS;
  DeviceExist = FALSE;

  //
  // Check if the console variable exist
  //
  StartDevicePath = EfiBootManagerGetVariableAndSize (
                      mConVarName[ConsoleType],
                      &gEfiGlobalVariableGuid,
                      &VariableSize
                      );
  if (StartDevicePath == NULL) {
    return EFI_UNSUPPORTED;
  }

  CopyOfDevicePath = StartDevicePath;
  do {
    //
    // Check every instance of the console variable
    //
    Instance  = GetNextDevicePathInstance (&CopyOfDevicePath, &Size);
    if (Instance == NULL) {
      FreePool (StartDevicePath);
      return EFI_UNSUPPORTED;
    }
    
    Next      = Instance;
    while (!IsDevicePathEndType (Next)) {
      Next = NextDevicePathNode (Next);
    }

    SetDevicePathEndNode (Next);
    //
    // Connect the USB console
    // USB console device path is a short-form device path that 
    //  starts with the first element being a USB WWID
    //  or a USB Class device path
    //
    if (FeaturePcdGet (PcdShortformBootSupport) &&
        (DevicePathType (Instance) == MESSAGING_DEVICE_PATH) &&
        ((DevicePathSubType (Instance) == MSG_USB_CLASS_DP) || (DevicePathSubType (Instance) == MSG_USB_WWID_DP))
       ) {
      Status = EfiBootManagerConnectUsbShortFormDevicePath (Instance);
      if (!EFI_ERROR (Status)) {
        DeviceExist = TRUE;
      }
    } else {
      //
      // Connect the instance device path
      //
      Status = ConnectDevicePath (Instance, NeedDispatch, NULL);
      if (EFI_ERROR (Status)) {
        //
        // Do not delete the instance from the console variable if the device path is of GOP type
        // and the parent controller exists in the system
        // This is to support the monitor hotplug in a headless boot
        //
        for (Next = Instance; !IsDevicePathEnd (Next); Next = NextDevicePathNode (Next)) {
          if (DevicePathType (Next) == ACPI_DEVICE_PATH && DevicePathSubType (Next) == ACPI_ADR_DP) {
            break;
          }
        }
        if (!IsDevicePathEnd (Next)) {
          Next = Instance;
          Status = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &Next, &Handle);
          if (!EFI_ERROR (Status) && (DevicePathType (Next) != ACPI_DEVICE_PATH || DevicePathSubType (Next) != ACPI_ADR_DP)) {
            //
            // We found the parent controller but it's not the direct parent of the ADR device path instance
            // which indicates the video controller doesn't exist.
            // If this happens, we need to delete the invalid device path from ConOut
            //
            Status = EFI_NOT_FOUND;
          }
        }
      } else {
        DeviceExist = TRUE;
      }

      if (EFI_ERROR (Status)) {
        //
        // Delete the instance from the console variable
        //
        EfiBootManagerUpdateConsoleVariable (ConsoleType, NULL, Instance);
      }
    }
    FreePool(Instance);
  } while (CopyOfDevicePath != NULL);

  FreePool (StartDevicePath);

  if ((ConsoleType == ConOut) && FeaturePcdGet (PcdBdsFindDisplay) && !HasLocalDisplay ()) {
    //
    // Force to connect local video controller
    // Backward compatible to old platforms which don't insert the GOP device path to ConOut
    //
    DEBUG ((EFI_D_ERROR, "[Bds] Local display isn't found, find & connect it automatically\n"));
    Status = ConnectVideoController ();
    if (!EFI_ERROR (Status)) {
      DeviceExist = TRUE;
    }
  }

  if (!DeviceExist) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}


/**
  This function will search every input/output device in current system,
  and make every input/output device as potential console device.
**/
VOID
EFIAPI
EfiBootManagerConnectAllConsoles (
  VOID
  )
{
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *ConDevicePath;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;

  Index         = 0;
  HandleCount   = 0;
  HandleBuffer  = NULL;
  ConDevicePath = NULL;

  //
  // Update all the console variables
  //
  gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiSimpleTextInProtocolGuid,
          NULL,
          &HandleCount,
          &HandleBuffer
          );

  for (Index = 0; Index < HandleCount; Index++) {
    gBS->HandleProtocol (
            HandleBuffer[Index],
            &gEfiDevicePathProtocolGuid,
            (VOID **) &ConDevicePath
            );
    EfiBootManagerUpdateConsoleVariable (ConIn, ConDevicePath, NULL);
  }

  if (HandleBuffer != NULL) {
    FreePool(HandleBuffer);
    HandleBuffer = NULL;
  }

  gBS->LocateHandleBuffer (
          ByProtocol,
          &gEfiSimpleTextOutProtocolGuid,
          NULL,
          &HandleCount,
          &HandleBuffer
          );
  for (Index = 0; Index < HandleCount; Index++) {
    gBS->HandleProtocol (
            HandleBuffer[Index],
            &gEfiDevicePathProtocolGuid,
            (VOID **) &ConDevicePath
            );
    EfiBootManagerUpdateConsoleVariable (ConOut, ConDevicePath, NULL);
    EfiBootManagerUpdateConsoleVariable (ErrOut, ConDevicePath, NULL);
  }

  if (HandleBuffer != NULL) {
    FreePool(HandleBuffer);
  }

  //
  // Connect all console variables
  //
  EfiBootManagerConnectAllDefaultConsoles ();
}


/**
  This function will connect all the console devices base on the console
  device variable ConIn, ConOut and ErrOut.
**/
VOID
EFIAPI
EfiBootManagerConnectAllDefaultConsoles (
  VOID
  )
{
  BOOLEAN                   SystemTableUpdated;

  EfiBootManagerConnectConsoleVariable (ConOut, TRUE);
  PERF_START (NULL, "ConOutReady", "BDS", 1);
  PERF_END   (NULL, "ConOutReady", "BDS", 0);

  
  EfiBootManagerConnectConsoleVariable (ConIn, TRUE);
  PERF_START (NULL, "ConInReady", "BDS", 1);
  PERF_END   (NULL, "ConInReady", "BDS", 0);

  //
  // The _ModuleEntryPoint err out var is legal.
  //
  EfiBootManagerConnectConsoleVariable (ErrOut, TRUE);
  PERF_START (NULL, "ErrOutReady", "BDS", 1);
  PERF_END   (NULL, "ErrOutReady", "BDS", 0);

  SystemTableUpdated = FALSE;
  //
  // Fill console handles in System Table if no console device assignd.
  //
  if (UpdateSystemTableConsole (L"ConIn", &gEfiSimpleTextInProtocolGuid, &gST->ConsoleInHandle, (VOID **) &gST->ConIn)) {
    SystemTableUpdated = TRUE;
  }
  if (UpdateSystemTableConsole (L"ConOut", &gEfiSimpleTextOutProtocolGuid, &gST->ConsoleOutHandle, (VOID **) &gST->ConOut)) {
    SystemTableUpdated = TRUE;
  }
  if (UpdateSystemTableConsole (L"ErrOut", &gEfiSimpleTextOutProtocolGuid, &gST->StandardErrorHandle, (VOID **) &gST->StdErr)) {
    SystemTableUpdated = TRUE;
  }

  if (SystemTableUpdated) {
    //
    // Update the CRC32 in the EFI System Table header
    //
    gST->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (
          (UINT8 *) &gST->Hdr,
          gST->Hdr.HeaderSize,
          &gST->Hdr.CRC32
          );
  }
}
