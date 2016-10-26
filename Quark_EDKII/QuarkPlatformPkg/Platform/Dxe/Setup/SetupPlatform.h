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

  SetupPlatform.h

Abstract:

  Header file for Platform Initialization Driver.

Revision History

++*/

#ifndef _SETUP_PLATFORM_H
#define _SETUP_PLATFORM_H

//
// Data
//
#define PLATFORM_NUM_SMBUS_RSVD_ADDRESSES 4
#define VAR_OFFSET(Field)     ((UINT16) ((UINTN) &(((SYSTEM_CONFIGURATION *) 0)->Field)))
#define QUESTION_ID(Field)    (VAR_OFFSET (Field) + 1)

#define SMBUS_ADDR_CH_A_1     0xA0
#define SMBUS_ADDR_CK505      0xD2
#define SMBUS_ADDR_THERMAL_SENSOR1 0x4C
#define SMBUS_ADDR_THERMAL_SENSOR2 0x4D

///
/// HII specific Vendor Device Path Node definition.
///
#pragma pack(1)

typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  UINT16                         UniqueId;
} HII_VENDOR_DEVICE_PATH_NODE;

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  HII_VENDOR_DEVICE_PATH_NODE    Node;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

//
// Prototypes
//
VOID
ProducePlatformCpuData (
  VOID
  );
  
VOID
PlatformInitQNCRegs (
  VOID
  );
  
EFI_STATUS
InitKeyboardLayout (
  VOID
  );

//
// Global externs
//
extern UINT8 UefiSetupDxeStrings[];

extern EFI_HII_DATABASE_PROTOCOL        *mHiiDataBase;
extern EFI_HII_CONFIG_ROUTING_PROTOCOL  *mHiiConfigRouting;

typedef struct {
  EFI_EXP_BASE10_DATA   *CoreFrequencyList;
  EFI_EXP_BASE10_DATA   *FsbFrequencyList;
} PLATFORM_CPU_FREQUENCY_LIST;
#endif
