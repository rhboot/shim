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

  Platform.h

Abstract:

  Quark platform specific information.

--*/
#include "Uefi.h"

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

//
// Constant definition
//
#define MAX_SMRAM_RANGES                    4
#define MAX_NODE                            1

#define QUARK_STAGE1_IMAGE_TYPE_MASK      0xF0
#define QUARK_STAGE1_BOOT_IMAGE_TYPE      0x00  // Stage1 Boot images 0x00 -> 0x0F.
#define QUARK_STAGE1_RECOVERY_IMAGE_TYPE  0x10  // Stage1 Recovery images 0x10 -> 0x1F.

#define QUARK_BOOTROM_BASE_ADDRESS        0xFFFE0000  // Base address of Quark ROM in memory map.
#define QUARK_BOOTROM_SIZE_BYTES          0x20000     // Quark ROM is 128KB.
#define SMM_DEFAULT_SMBASE                  0x30000     // Default SMBASE address.
#define SMM_DEFAULT_SMBASE_SIZE_BYTES       0x10000     // Size in bytes of default SMRAM.

//
// Gpio to be used to assert / deassert PCI express PERST# signal.
//
#define PCIEXP_PERST_RESUMEWELL_GPIO        3

//
// Minimum time in microseconds for assertion of PERST# signal.
//
#define PCIEXP_PERST_MIN_ASSERT_US          100

//
// Microsecond delay post issueing common lane reset.
//
#define PCIEXP_DELAY_US_POST_CMNRESET_RESET 1

//
// Microsecond delay to wait for PLL to lock.
//
#define PCIEXP_DELAY_US_WAIT_PLL_LOCK       80

//
// Microsecond delay post issueing sideband interface reset.
//
#define PCIEXP_DELAY_US_POST_SBI_RESET      20

//
// Microsecond delay post deasserting PERST#.
//
#define PCIEXP_DELAY_US_POST_PERST_DEASSERT 10

//
// Catastrophic Trip point in degrees Celsius for this platform.
//
#define PLATFORM_CATASTROPHIC_TRIP_CELSIUS  105

//
// Platform flash update LED common definitions.
//
#define PLATFORM_FLASH_UPDATE_LED_TOGGLE_COUNT   7
#define PLATFORM_FLASH_UPDATE_LED_TOGGLE_DELTA   (1000 * 1000)  // In Microseconds for EFI_STALL.

//
// This structure stores the base and size of the ACPI reserved memory used when
// resuming from S3.  This region must be allocated by the platform code.
//
typedef struct {
  UINT32  AcpiReservedMemoryBase;
  UINT32  AcpiReservedMemorySize;
  UINT32  SystemMemoryLength;
} RESERVED_ACPI_S3_RANGE;

#define RESERVED_ACPI_S3_RANGE_OFFSET (EFI_PAGE_SIZE - sizeof (RESERVED_ACPI_S3_RANGE))

//
// Define valid platform types.
// First add value before TypePlatformMax in EFI_PLATFORM_TYPE definition
// and then add string description to end of EFI_PLATFORM_TYPE_NAME_TABLE_DEFINITION.
// Value shown for supported platforms to help sanity checking with build tools
// and ACPI method usage.
//
typedef enum {
  TypeUnknown = 0,      // !!! SHOULD BE THE FIRST ENTRY !!!
  QuarkEmulation = 1,
  ClantonPeakSVP = 2,
  KipsBay = 3,
  CrossHill = 4,
  ClantonHill = 5,
  Galileo = 6,
  TypePlatformRsv7 = 7,
  GalileoGen2 = 8,
  TypePlatformMax       // !!! SHOULD BE THE LAST ENTRY !!!
} EFI_PLATFORM_TYPE;

#define EFI_PLATFORM_TYPE_NAME_TABLE_DEFINITION \
  L"TypeUnknown",\
  L"QuarkEmulation",\
  L"ClantonPeakSVP",\
  L"KipsBay",\
  L"CrossHill",\
  L"ClantonHill",\
  L"Galileo",\
  L"TypePlatformRsv7",\
  L"GalileoGen2",\

typedef struct {
  UINT32  EntryOffset;
  UINT8   ImageIndex;
} QUARK_EDKII_STAGE1_HEADER;

#endif
