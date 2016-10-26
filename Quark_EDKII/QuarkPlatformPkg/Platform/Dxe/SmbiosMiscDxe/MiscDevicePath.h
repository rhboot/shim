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

  MiscDevicePath.h

Abstract:

  Misc class required EFI Device Path definitions (Ports, slots &
  onboard devices)

--*/

#ifndef _MISC_DEVICE_PATH_H
#define _MISC_DEVICE_PATH_H

#pragma pack(1)

//USB
/* For reference:
#define USB1_1_STR  "ACPI(PNP0A03,0)/PCI(1D,0)."
#define USB1_2_STR  "ACPI(PNP0A03,0)/PCI(1D,1)."
#define USB1_3_STR  "ACPI(PNP0A03,0)/PCI(1D,2)."
#define USB2_1_STR  "ACPI(PNP0A03,0)/PCI(1D,7)."
*/

#define DP_ACPI { ACPI_DEVICE_PATH,\
    ACPI_DP, (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),\
   (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), EISA_PNP_ID(0x0A03), 0 }
#define DP_PCI( device,function)  { HARDWARE_DEVICE_PATH,\
    HW_PCI_DP, (UINT8) (sizeof (PCI_DEVICE_PATH)),\
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8), function, device }
#define DP_END  { END_DEVICE_PATH_TYPE, \
    END_ENTIRE_DEVICE_PATH_SUBTYPE, END_DEVICE_PATH_LENGTH, 0 }

#define DP_LPC(eisaid,function ){ ACPI_DEVICE_PATH, \
ACPI_DP,(UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),\
(UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8),EISA_PNP_ID(eisaid), function }


#pragma pack()


#endif
