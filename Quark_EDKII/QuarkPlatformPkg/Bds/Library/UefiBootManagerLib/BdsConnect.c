/** @file
  BDS Lib functions which relate with connect the device

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


/**
  This function will connect all the system driver to controller
  first, and then special connect the default console, this make
  sure all the system controller available and the platform default
  console connected.

**/
VOID
EFIAPI
EfiBootManagerConnectAll (
  VOID
  )
{
  //
  // Connect the platform console first
  //
  EfiBootManagerConnectAllDefaultConsoles ();

  //
  // Generic way to connect all the drivers
  //
  ConnectAllDriversToAllControllers ();

  //
  // Here we have the assumption that we have already had
  // platform default console
  //
  EfiBootManagerConnectAllDefaultConsoles ();
}

/**
  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created successfully, Dispatch service which load the missing drivers
  is called according to input parameter, since in some cases no driver 
  dependency is assumed exist, so may need not to call this service.

  @param  DevicePathToConnect   The device path which will be connected, it CANNOT be
                                a multi-instance device path
  @param  NeedDispatch          Whether requires dispatch service during connection                                 
  @param  MatchingHandle        Return the controller handle closest to the DevicePathToConnect

  @retval EFI_INVALID_PARAMETER DevicePathToConnect is NULL.
  @retval EFI_NOT_FOUND         Failed to create all handles associate with every device path node.
  @retval EFI_SUCCESS           Successful to create all handles associate with every device path node.

**/
EFI_STATUS
ConnectDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect,
  IN BOOLEAN                    NeedDispatch,
  OUT EFI_HANDLE                *MatchingHandle          OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;
  EFI_HANDLE                Handle;
  EFI_HANDLE                PreviousHandle;

  if (DevicePathToConnect == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Start the real work of connect with RemainingDevicePath
  //
  PreviousHandle = NULL;
  do {
    //
    // Find the handle that best matches the Device Path. If it is only a
    // partial match the remaining part of the device path is returned in
    // RemainingDevicePath.
    //
    RemainingDevicePath = DevicePathToConnect;
    Status              = gBS->LocateDevicePath (&gEfiDevicePathProtocolGuid, &RemainingDevicePath, &Handle);
    if (!EFI_ERROR (Status)) {
      if (Handle == PreviousHandle) {
        //
        // If no forward progress is made try invoking the Dispatcher.
        // A new FV may have been added to the system an new drivers
        // may now be found.
        // Status == EFI_SUCCESS means a driver was dispatched
        // Status == EFI_NOT_FOUND means no new drivers were dispatched
        //
        if (NeedDispatch) {
          Status = gDS->Dispatch ();
        } else {
          //
          // Always return EFI_NOT_FOUND here
          // to prevent dead loop when control handle is found but connection failded case
          //
          Status = EFI_NOT_FOUND;
        }
      }


      if (!EFI_ERROR (Status)) {
        PreviousHandle = Handle;
        //
        // Connect all drivers that apply to Handle and RemainingDevicePath,
        // the Recursive flag is FALSE so only one level will be expanded.
        //
        // Do not check the connect status here, if the connect controller fail,
        // then still give the chance to do dispatch, because partial
        // RemainingDevicepath may be in the new FV
        //
        // 1. If the connect fail, RemainingDevicepath and handle will not
        //    change, so next time will do the dispatch, then dispatch's status
        //    will take effect
        // 2. If the connect success, the RemainingDevicepath and handle will
        //    change, then avoid the dispatch, we have chance to continue the
        //    next connection
        //
        gBS->ConnectController (Handle, NULL, RemainingDevicePath, FALSE);
        if (MatchingHandle != NULL) {
          *MatchingHandle = Handle;
        }
      }
    }
    //
    // Loop until RemainingDevicePath is an empty device path
    //
  } while (!EFI_ERROR (Status) && !IsDevicePathEnd (RemainingDevicePath));

  ASSERT (EFI_ERROR (Status) || IsDevicePathEnd (RemainingDevicePath));

  return Status;
}

/**
  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created successfully, then still give chance to do the dispatch,
  which load the missing drivers if possible.

  @param  DevicePathToConnect   The device path which will be connected, it can be
                                a multi-instance device path
  @param  MatchingHandle        Return the controller handle closest to the DevicePathToConnect

  @retval EFI_SUCCESS           All handles associate with every device path  node
                                have been created
  @retval EFI_OUT_OF_RESOURCES  There is no resource to create new handles
  @retval EFI_NOT_FOUND         Create the handle associate with one device  path
                                node failed

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect,
  OUT EFI_HANDLE                *MatchingHandle          OPTIONAL
  )
{
  return ConnectDevicePath (DevicePathToConnect, TRUE, MatchingHandle);
}

/**
  This function will disconnect all current system handles. 
  
  gBS->DisconnectController() is invoked for each handle exists in system handle buffer.
  If handle is a bus type handle, all childrens also are disconnected recursively by
  gBS->DisconnectController().
**/
VOID
EFIAPI
EfiBootManagerDisconnectAll (
  VOID
  )
{
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;

  //
  // Disconnect all
  //
  gBS->LocateHandleBuffer (
         AllHandles,
         NULL,
         NULL,
         &HandleCount,
         &HandleBuffer
         );
  for (Index = 0; Index < HandleCount; Index++) {
    gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }
}


/**
  Connect all the drivers to all the controllers.

  This function makes sure all the current system drivers manage the correspoinding
  controllers if have. And at the same time, makes sure all the system controllers
  have driver to manage it if have.
**/
VOID
ConnectAllDriversToAllControllers (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;

  do {
    //
    // Connect All EFI 1.10 drivers following EFI 1.10 algorithm
    //
    gBS->LocateHandleBuffer (
           AllHandles,
           NULL,
           NULL,
           &HandleCount,
           &HandleBuffer
           );

    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }

    //
    // Check to see if it's possible to dispatch an more DXE drivers.
    // The above code may have made new DXE drivers show up.
    // If any new driver is dispatched (Status == EFI_SUCCESS) and we will try
    // the connect again.
    //
    Status = gDS->Dispatch ();

  } while (!EFI_ERROR (Status));
}


/**
  Connect the specific Usb device which match the short form device path,
  and whose bus is determined by Host Controller (Uhci or Ehci).

  @param  DevicePath             A short-form device path that starts with the first
                                 element being a USB WWID or a USB Class device
                                 path

  @return EFI_INVALID_PARAMETER  DevicePath is NULL pointer.
                                 DevicePath is not a USB device path.

  @return EFI_SUCCESS            Success to connect USB device
  @return EFI_NOT_FOUND          Fail to find handle for USB controller to connect.

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectUsbShortFormDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  EFI_STATUS                            Status;
  EFI_HANDLE                            *Handles;
  UINTN                                 HandleCount;
  UINTN                                 Index;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  UINT8                                 Class[3];
  BOOLEAN                               AtLeastOneConnected;

  //
  // Check the passed in parameters
  //
  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DevicePathType (DevicePath) != MESSAGING_DEVICE_PATH) ||
      ((DevicePathSubType (DevicePath) != MSG_USB_CLASS_DP) && (DevicePathSubType (DevicePath) != MSG_USB_WWID_DP))
     ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the usb host controller firstly, then connect with the remaining device path
  //
  AtLeastOneConnected = FALSE;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      Handles[Index],
                      &gEfiPciIoProtocolGuid,
                      (VOID **) &PciIo
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Check whether the Pci device is the wanted usb host controller
        //
        Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
        if (!EFI_ERROR (Status) &&
            ((PCI_CLASS_SERIAL == Class[2]) && (PCI_CLASS_SERIAL_USB == Class[1]))
           ) {
          Status = gBS->ConnectController (
                          Handles[Index],
                          NULL,
                          DevicePath,
                          FALSE
                          );
          if (!EFI_ERROR(Status)) {
            AtLeastOneConnected = TRUE;
          }
        }
      }
    }

    if (Handles != NULL) {
      FreePool (Handles);
    }
  }

  return AtLeastOneConnected ? EFI_SUCCESS : EFI_NOT_FOUND;
}
