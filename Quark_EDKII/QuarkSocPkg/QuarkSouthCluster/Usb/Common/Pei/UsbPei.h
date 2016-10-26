/** @file
  Define private data structure for UHCI and EHCI.
  
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

#ifndef _USB_PEI_H
#define _USB_PEI_H

#include "Ioh.h"

#define PEI_IOH_OHCI_SIGNATURE          SIGNATURE_32 ('O', 'H', 'C', 'I')
#define PEI_IOH_EHCI_SIGNATURE          SIGNATURE_32 ('E', 'H', 'C', 'I')

typedef struct {
  UINTN                   Signature;
  PEI_USB_CONTROLLER_PPI  UsbControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR  PpiList;
  UINTN                   MmioBase[IOH_MAX_OHCI_USB_CONTROLLERS];
} IOH_OHCI_DEVICE;

typedef struct {
  UINTN                   Signature;
  PEI_USB_CONTROLLER_PPI  UsbControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR  PpiList;
  UINTN                   MmioBase[IOH_MAX_EHCI_USB_CONTROLLERS];
} IOH_EHCI_DEVICE;

#define IOH_OHCI_DEVICE_FROM_THIS(a) \
  CR(a, IOH_OHCI_DEVICE, UsbControllerPpi, PEI_IOH_OHCI_SIGNATURE)

#define IOH_EHCI_DEVICE_FROM_THIS(a) \
  CR (a, IOH_EHCI_DEVICE, UsbControllerPpi, PEI_IOH_EHCI_SIGNATURE)

#endif
