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

  IohData.c

Abstract:

  Defined the Ioh device path which will be used by
  platform Bbd to perform the platform policy connect.

--*/



#include "IohBds.h"

//
// Predefined platform root bridge
//
PLATFORM_ROOT_BRIDGE_DEVICE_PATH gPlatformRootBridge0 = {
  gPciRootBridge,
  gEndEntire
};

EFI_DEVICE_PATH_PROTOCOL* gPlatformRootBridges [] = {
  (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformRootBridge0,
  NULL
};



//
// Ioh USB EHCI controller device path
//
IOH_PCI_USB_DEVICE_PATH gIohUsbDevicePath0 = {
  gPciRootBridge,
  PCI_DEVICE_PATH_NODE(3, 0x14), // Ioh EHCI Usb device 20, function 3
  gEndEntire
};

//
// Ioh predefined device connecting option
//
EFI_DEVICE_PATH_PROTOCOL* gDeviceConnectOption [] = {
  //  (EFI_DEVICE_PATH_PROTOCOL*)&gIohUsbDevicePath0,
  NULL
};

