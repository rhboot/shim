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
  
    MiscSystemSlotDesignationData.c
  
Abstract: 

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

--*/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot1) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT1),        // SlotDesignation
  EfiSlotTypePci,                             // SlotType
  EfiSlotDataBusWidth32Bit,                   // SlotDataBusWidth
  EfiSlotUsageAvailable,                      // SlotUsage
  EfiSlotLengthLong ,                         // SlotLength
  1,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    0,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot2) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT2),        // SlotDesignation
  EfiSlotTypePciExpress,                      // SlotType
  EfiSlotDataBusWidth32Bit,                   // SlotDataBusWidth
  EfiSlotUsageAvailable,                      // SlotUsage
  EfiSlotLengthLong ,                         // SlotLength
  1,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot3) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT3),        // SlotDesignation
  EfiSlotTypePciExpress,                      // SlotType
  EfiSlotDataBusWidth32Bit,                   // SlotDataBusWidth
  EfiSlotUsageAvailable,                      // SlotUsage
  EfiSlotLengthLong ,                         // SlotLength
  2,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot4) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT4),        // SlotDesignation
  EfiSlotTypePciExpress,                      // SlotType
  EfiSlotDataBusWidth32Bit,                   // SlotDataBusWidth
  EfiSlotUsageAvailable,                      // SlotUsage
  EfiSlotLengthLong ,                         // SlotLength
  2,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot5) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT5),        // SlotDesignation
  EfiSlotTypePciExpress,                      // SlotType
  EfiSlotDataBusWidth32Bit,                   // SlotDataBusWidth
  EfiSlotUsageAvailable,                      // SlotUsage
  EfiSlotLengthLong ,                         // SlotLength
  3,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot6) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT6),        // SlotDesignation
  EfiSlotTypePciExpress,                      // SlotType
  EfiSlotDataBusWidth32Bit,                   // SlotDataBusWidth
  EfiSlotUsageAvailable,                      // SlotUsage
  EfiSlotLengthLong ,                         // SlotLength
  3,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot7) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT7),        // SlotDesignation
  EfiSlotTypePciExpress,                      // SlotType
  EfiSlotDataBusWidth32Bit,                   // SlotDataBusWidth
  EfiSlotUsageAvailable,                      // SlotUsage
  EfiSlotLengthLong ,                         // SlotLength
  3,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot8) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT8),        // SlotDesignation
  EfiSlotTypePciExpress,                      // SlotType
  EfiSlotDataBusWidth32Bit,                   // SlotDataBusWidth
  EfiSlotUsageAvailable,                      // SlotUsage
  EfiSlotLengthLong ,                         // SlotLength
  3,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot9) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT9),        // SlotDesignation
  EfiSlotTypeUnknown,                         // SlotType
  EfiSlotDataBusWidthUnknown,                 // SlotDataBusWidth
  EfiSlotUsageUnknown,                        // SlotUsage
  EfiSlotLengthUnknown ,                      // SlotLength
  0,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot10) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT10),       // SlotDesignation
  EfiSlotTypeUnknown,                         // SlotType
  EfiSlotDataBusWidthUnknown,                 // SlotDataBusWidth
  EfiSlotUsageUnknown,                        // SlotUsage
  EfiSlotLengthUnknown ,                      // SlotLength
  0,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot11) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT11),       // SlotDesignation
  EfiSlotTypeUnknown,                         // SlotType
  EfiSlotDataBusWidthUnknown,                 // SlotDataBusWidth
  EfiSlotUsageUnknown,                        // SlotUsage
  EfiSlotLengthUnknown ,                      // SlotLength
  0,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot12) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT12),       // SlotDesignation
  EfiSlotTypeUnknown,                         // SlotType
  EfiSlotDataBusWidthUnknown,                 // SlotDataBusWidth
  EfiSlotUsageUnknown,                        // SlotUsage
  EfiSlotLengthUnknown ,                      // SlotLength
  0,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot13) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT13),       // SlotDesignation
  EfiSlotTypeUnknown,                         // SlotType
  EfiSlotDataBusWidthUnknown,                 // SlotDataBusWidth
  EfiSlotUsageUnknown,                        // SlotUsage
  EfiSlotLengthUnknown ,                      // SlotLength
  0,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlot14) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT14),       // SlotDesignation
  EfiSlotTypeUnknown,                         // SlotType
  EfiSlotDataBusWidthUnknown,                 // SlotDataBusWidth
  EfiSlotUsageUnknown,                        // SlotUsage
  EfiSlotLengthUnknown ,                      // SlotLength
  0,                                          // SlotId
  {                                           // SlotCharacteristics
    0,                                        // CharacteristicsUnknown  :1;
    0,                                        // Provides50Volts         :1;
    1,                                        // Provides33Volts         :1;
    0,                                        // SharedSlot              :1;
    0,                                        // PcCard16Supported       :1;
    0,                                        // CardBusSupported        :1;
    0,                                        // ZoomVideoSupported      :1;
    0,                                        // ModemRingResumeSupported:1;
    1,                                        // PmeSignalSupported      :1;
    1,                                        // HotPlugDevicesSupported :1;
    1,                                        // SmbusSignalSupported    :1;
    0                                         // Reserved                :21;
  },
  0                                           // SlotDevicePath
};


