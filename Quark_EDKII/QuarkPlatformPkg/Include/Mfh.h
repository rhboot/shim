/** @file

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

  Mfh.h

Abstract:

  Defines for Quark Master Flash Header (MFH).
  Please refer to Quark Secure Boot Programmers Reference Manual.

--*/

#ifndef __MFH_H__
#define __MFH_H__

//
// Unique identifier to allow quick sanity checking on our MFH.
// Corresponds to _MFH in ASCII characters.
//
#define MFH_IDENTIFIER                            0x5F4D4648

//
// MFH Max values for sanity checks
//
#define MFH_MAX_NUM_BOOT_ITEMS                    24
#define MFH_MAX_NUM_FLASH_ITEMS                   240

//
// Flash Item type definitions.
//

#define MFH_FLASH_ITEM_TYPE_FW_STAGE1             0x00000000
#define MFH_FLASH_ITEM_TYPE_FW_STAGE1_SIGNED      0x00000001
#define MFH_FLASH_ITEM_TYPE_RSVD_02H              0x00000002
#define MFH_FLASH_ITEM_TYPE_FW_STAGE2             0x00000003
#define MFH_FLASH_ITEM_TYPE_FW_STAGE2_SIGNED      0x00000004
#define MFH_FLASH_ITEM_TYPE_FW_STAGE2_CFG         0x00000005
#define MFH_FLASH_ITEM_TYPE_FW_STAGE2_CFG_SIGNED  0x00000006
#define MFH_FLASH_ITEM_TYPE_FW_PARAMETERS         0x00000007
#define MFH_FLASH_ITEM_TYPE_FW_RECOVERY           0x00000008
#define MFH_FLASH_ITEM_TYPE_FW_RECOVERY_SIGNED    0x00000009
#define MFH_FLASH_ITEM_TYPE_RSVD_0AH              0x0000000A
#define MFH_FLASH_ITEM_TYPE_BOOTLOADER            0x0000000B
#define MFH_FLASH_ITEM_TYPE_BOOTLOADER_SIGNED     0x0000000C
#define MFH_FLASH_ITEM_TYPE_BOOTLOADER_CFG        0x0000000D
#define MFH_FLASH_ITEM_TYPE_BOOTLOADER_CFG_SIGNED 0x0000000E
#define MFH_FLASH_ITEM_TYPE_RSVD_0F               0x0000000F
#define MFH_FLASH_ITEM_TYPE_KERNEL                0x00000010
#define MFH_FLASH_ITEM_TYPE_KERNEL_SIGNED         0x00000011
#define MFH_FLASH_ITEM_TYPE_RAMDISK               0x00000012
#define MFH_FLASH_ITEM_TYPE_RAMDISK_SIGNED        0x00000013
#define MFH_FLASH_ITEM_TYPE_RSVD_14H              0x00000014
#define MFH_FLASH_ITEM_TYPE_PROGRAM               0x00000015
#define MFH_FLASH_ITEM_TYPE_PROGRAM_SIGNED        0x00000016
#define MFH_FLASH_ITEM_TYPE_RSVD_17H              0x00000017
#define MFH_FLASH_ITEM_TYPE_BUILD_INFORMATION     0x00000018
#define MFH_FLASH_ITEM_TYPE_IMAGE_VERSION         0x00000019

#pragma pack(1)

///
/// Initial header fields of MFH.
///
typedef union {
  UINT32              ImageVersion;                   // Definition for MFH_FLASH_ITEM_TYPE_IMAGE_VERSION
  UINT32              Reserved;                       // Definition for all other item types
} MFH_TYPE_SPECIFIC;

///
/// An entry in the MFH describing some element stored in Flash.
///
typedef struct {
  UINT32              Type;                           // Type of module that this Flash Item describes.
  UINT32              FlashAddress;                   // Starting address of the given element in SPI Flash device.
  UINT32              LengthBytes;                    // Length of given module in bytes.
  MFH_TYPE_SPECIFIC   TypeSpecific;                   // This field is specific to the MFH Item type.
} MFH_FLASH_ITEM;

///
/// Initial header fields of MFH.
///
typedef struct {
  UINT32 QuarkMFHIdentifier;          // Unique identifier to sanity check MFH presence. This value will always be set to 0xC3F4 C3F4.
  UINT32 Version;                       // Current version of MFH used.
  UINT32 RsvdFlags;                     // Reserved for use as flags.
  UINT32 NextHeaderBlock;               // Offset from beginning of flash to the next header block.
  UINT32 FlashItemCount;                // Number of flash items within this header block.
  UINT32 BootPriorityListCount;         // Number of boot images within this MFH. All boot images must be within first MFH block.
} MFH_HEADER;

#pragma pack()

#endif // #ifndef __MFH_H__
