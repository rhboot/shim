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

  MiscSystemManufacturerData.c
  
Abstract: 

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data using smbios protocol.

--*/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) System Manufacturer data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_MANUFACTURER, MiscSystemManufacturer) = {
  STRING_TOKEN(STR_MISC_SYSTEM_MANUFACTURER),   // SystemManufactrurer
  STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME),   // SystemProductName
  STRING_TOKEN(STR_MISC_SYSTEM_VERSION),        // SystemVersion
  STRING_TOKEN(STR_MISC_SYSTEM_SERIAL_NUMBER),  // SystemSerialNumber
  {                                             // SystemUuid
    0x13ffef23, 0x8654, 0x46da, 0xa4, 0x7, 0x39, 0xc9, 0x12, 0x2, 0xd3, 0x56
  },
  EfiSystemWakeupTypePowerSwitch,               // SystemWakeupType  
  STRING_TOKEN(STR_MISC_SYSTEM_SKU_NUMBER),     // SystemSKUNumber
  STRING_TOKEN(STR_MISC_SYSTEM_FAMILY),         // SystemFamily
};
