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

  MiscOnboardDeviceData.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to smbios.

--*/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_ONBOARD_DEVICE, MiscOnboardDeviceVideo) = {
  STRING_TOKEN(STR_MISC_ONBOARD_DEVICE_VIDEO),    // OnBoardDeviceDescription
  {                                               // OnBoardDeviceStatus
    EfiOnBoardDeviceTypeVideo,                    // DeviceType
    1,                                            // DeviceEnabled
    0                                             // Reserved
  },
  0                                               // OnBoardDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_ONBOARD_DEVICE, MiscOnboardDeviceNetwork) = {
  STRING_TOKEN(STR_MISC_ONBOARD_DEVICE_NETWORK),  // OnBoardDeviceDescription
  {                                               // OnBoardDeviceStatus
    EfiOnBoardDeviceTypeEthernet,                 // DeviceType
    1,                                            // DeviceEnabled
    0                                             // Reserved
  },
  0                                               // OnBoardDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_ONBOARD_DEVICE, MiscOnboardDeviceAudio) = {
  STRING_TOKEN(STR_MISC_ONBOARD_DEVICE_AUDIO),    // OnBoardDeviceDescription
  {                                               // OnBoardDeviceStatus
    EfiOnBoardDeviceTypeSound,                    // DeviceType
    1,                                            // DeviceEnabled
    0                                             // Reserved
  },
  DP_END                                          // OnBoardDevicePath
};
