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

  EfiFlashMap.h

Abstract:

  Defines for the EFI Flash Map functionality
  
--*/

#ifndef _EFI_FLASH_MAP_H_
#define _EFI_FLASH_MAP_H_

#include <Pi/PiHob.h>

//
// Definition for flash map GUIDed HOBs
//
typedef UINT32  EFI_FLASH_AREA_ATTRIBUTES;

#define EFI_FLASH_AREA_FV           0x0001
#define EFI_FLASH_AREA_SUBFV        0x0002
#define EFI_FLASH_AREA_MEMMAPPED_FV 0x0004
#define EFI_FLASH_AREA_REQUIRED     0x0008
#define EFI_FLASH_AREA_CORRUPT      0x0010

typedef UINT8   EFI_FLASH_AREA_TYPE;

#define EFI_FLASH_AREA_RECOVERY_BIOS  0x0   // Recovery code
#define EFI_FLASH_AREA_MAIN_BIOS      0x1   // Regular BIOS code
#define EFI_FLASH_AREA_PAL_B          0x2   // PAL-B
#define EFI_FLASH_AREA_RESERVED_03    0x3   // Reserved for backwards compatibility
#define EFI_FLASH_AREA_RESERVED_04    0x4   // Reserved for backwards compatibility
#define EFI_FLASH_AREA_DMI_FRU        0x5   // DMI FRU information
#define EFI_FLASH_AREA_OEM_BINARY     0x6   // OEM Binary Code/data
#define EFI_FLASH_AREA_RESERVED_07    0x7   // Reserved for backwards compatibility
#define EFI_FLASH_AREA_RESERVED_08    0x8   // Reserved for backwards compatibility
#define EFI_FLASH_AREA_RESERVED_09    0x9   // Reserved for backwards compatibility
#define EFI_FLASH_AREA_RESERVED_0A    0x0a  // Reserved for backwards compatibility
#define EFI_FLASH_AREA_EFI_VARIABLES  0x0b  // EFI variables
#define EFI_FLASH_AREA_MCA_LOG        0x0c  // MCA error log
#define EFI_FLASH_AREA_SMBIOS_LOG     0x0d  // SMBIOS error log
#define EFI_FLASH_AREA_FTW_BACKUP     0x0e  // A backup block during FTW operations
#define EFI_FLASH_AREA_FTW_STATE      0x0f  // State information during FTW operations
#define EFI_FLASH_AREA_UNUSED         0x0fd // Not used
#define EFI_FLASH_AREA_GUID_DEFINED   0x0fe // Usage defined by a GUID
#pragma pack(1)
//
// An individual sub-area Entry.
// A single flash area may consist of  more than one sub-area.
//
typedef struct {
  EFI_FLASH_AREA_ATTRIBUTES Attributes;
  UINT32                    Reserved;
  EFI_PHYSICAL_ADDRESS      Base;
  EFI_PHYSICAL_ADDRESS      Length;
  EFI_GUID                  FileSystem;
} EFI_FLASH_SUBAREA_ENTRY;

typedef struct {
  UINT8                   Reserved[3];
  EFI_FLASH_AREA_TYPE     AreaType;
  EFI_GUID                AreaTypeGuid;
  UINT32                  NumEntries;
  EFI_FLASH_SUBAREA_ENTRY Entries[1];
} EFI_FLASH_MAP_ENTRY_DATA;

typedef struct {
  UINT8                   Reserved[3];
  EFI_FLASH_AREA_TYPE     AreaType;
  EFI_GUID                AreaTypeGuid;
  UINT32                  NumberOfEntries;
  EFI_FLASH_SUBAREA_ENTRY Entries[1];
  //
  // Extended Hob data.
  //
  // VolumeId and FilePath indicating a unique file.
  //
  UINT32                  VolumeId;
  CHAR16                  FilePath[256];
  UINT32                  ActuralSize;
  UINT32                  Offset;
} EFI_FLASH_MAP_FS_ENTRY_DATA;

typedef struct {
  EFI_HOB_GENERIC_HEADER  Header;
  EFI_GUID                Name;
  UINT8                   Reserved[3];
  EFI_FLASH_AREA_TYPE     AreaType;
  EFI_GUID                AreaTypeGuid;
  UINT32                  NumEntries;
  EFI_FLASH_SUBAREA_ENTRY Entries[1];
} EFI_HOB_FLASH_MAP_ENTRY_TYPE;

//
// Internal definitions
//
typedef struct {
  UINT8                   Reserved[3];
  EFI_FLASH_AREA_TYPE     AreaType;
  EFI_GUID                AreaTypeGuid;
  UINT32                  NumberOfEntries;
  EFI_FLASH_SUBAREA_ENTRY SubAreaData;
} EFI_FLASH_AREA_HOB_DATA;

typedef struct {
  UINTN                     Base;
  UINTN                     Length;
  EFI_FLASH_AREA_ATTRIBUTES Attributes;
  EFI_FLASH_AREA_TYPE       AreaType;
  UINT8                     Reserved[3];
  EFI_GUID                  AreaTypeGuid;
} EFI_FLASH_AREA_DATA;

#pragma pack()

#endif // #ifndef _EFI_FLASH_MAP_H_
