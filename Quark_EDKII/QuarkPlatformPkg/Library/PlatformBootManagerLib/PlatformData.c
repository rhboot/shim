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

  PlatformData.c

Abstract:

  Defined the platform specific device path which will be used by
  platform Bbd to perform the platform policy connect.

--*/

#include "BdsPlatform.h"

//
// Predefined platform default time out value
//
UINT16                            gPlatformBootTimeOutDefault = 10;

//
// Predefined platform root bridge
//
PLATFORM_ROOT_BRIDGE_DEVICE_PATH  gPlatformRootBridge0 = {
  gPciRootBridge,
  gEndEntire
};

EFI_DEVICE_PATH_PROTOCOL          *gPlatformRootBridges[] = {
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformRootBridge0,
  NULL
};

//
// Platform specific serial device path
//
PLATFORM_PCI_SERIAL_DEVICE_PATH   gSerialDevicePath = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8) (sizeof (PCI_DEVICE_PATH)),
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8),
    0xff,
    0xff
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_UART_DP,
    (UINT8) (sizeof (UART_DEVICE_PATH)),
    (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8),
    0,
    0xffffffffffffffff,
    0xff,
    0xff,
    0xff
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  gEndEntire
};

USB_CLASS_FORMAT_DEVICE_PATH gUsbClassKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      (UINT8) (sizeof (USB_CLASS_DEVICE_PATH)),
      (UINT8) ((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
    },
    0xffff,           // VendorId
    0xffff,           // ProductId
    CLASS_HID,        // DeviceClass
    SUBCLASS_BOOT,    // DeviceSubClass
    PROTOCOL_KEYBOARD // DeviceProtocol
  },

  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    END_DEVICE_PATH_LENGTH,
    0
  }
};

//
// Predefined platform default console device path
//
BDS_CONSOLE_CONNECT_ENTRY         gPlatformConsole[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gSerialDevicePath,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gUsbClassKeyboardDevicePath,
    CONSOLE_IN
  },
  {
    NULL,
    0
  }
};

//
// Predefined platform specific perdict boot option
//
EFI_DEVICE_PATH_PROTOCOL          *gPlatformBootOption[] = {
  NULL
};

//
// Predefined platform specific driver option
//
EFI_DEVICE_PATH_PROTOCOL          *gPlatformDriverOption[] = { NULL };

//
// Platform specific USB controller device path
//
PLATFORM_USB_DEVICE_PATH gUsbDevicePath0 = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8)(sizeof(PCI_DEVICE_PATH)),
    (UINT8)((sizeof(PCI_DEVICE_PATH)) >> 8),
    IOH_EHCI_FUNCTION_NUMBER,
    IOH_USB_EHCI_DEVICE_NUMBER
  },
  gEndEntire
};

//
// Platform specific USB controller device path
//
PLATFORM_USB_DEVICE_PATH gUsbDevicePath1 = {
  gPciRootBridge,
  {
    HARDWARE_DEVICE_PATH,
    HW_PCI_DP,
    (UINT8)(sizeof(PCI_DEVICE_PATH)),
    (UINT8)((sizeof(PCI_DEVICE_PATH)) >> 8),
    IOH_OHCI_FUNCTION_NUMBER,
    IOH_USB_OHCI_DEVICE_NUMBER
  },
  gEndEntire
};

//
// Predefined platform device path for user authtication
//
EFI_DEVICE_PATH_PROTOCOL* gUserAuthenticationDevice[] = {
  //
  // Predefined device path for secure card (USB disk).
  //
  (EFI_DEVICE_PATH_PROTOCOL*)&gUsbDevicePath0,
  (EFI_DEVICE_PATH_PROTOCOL*)&gUsbDevicePath1,
  NULL
};

//
// Predefined platform connect sequence
//
EFI_DEVICE_PATH_PROTOCOL          *gPlatformConnectSequence[] = {
  (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformRootBridge0,  // Force PCI enumer before Legacy OpROM shadow
  (EFI_DEVICE_PATH_PROTOCOL*)&gUsbDevicePath0,
  (EFI_DEVICE_PATH_PROTOCOL*)&gUsbDevicePath1,
  NULL
};
