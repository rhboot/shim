/** @file
  PlatformInfo GUID

  These GUIDs point the ACPI tables as defined in the ACPI specifications.
  ACPI 2.0 specification defines the ACPI 2.0 GUID. UEFI 2.0 defines the 
  ACPI 2.0 Table GUID and ACPI Table GUID.

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

  @par Revision Reference:
  GUIDs defined in UEFI 2.0 spec.

**/

#ifndef _PLATFORM_INFO_GUID_H_
#define _PLATFORM_INFO_GUID_H_

#include "PlatformData.h"
#include <Uefi.h>

extern EFI_GUID gEfiPlatformInfoGuid;
extern CHAR16   EfiPlatformInfoVariable[];

#pragma pack(1)

typedef struct {
  //
  // These are global resource mapping of the system. Per IOH resource mapping is available in UDS.
  //
  UINT16  PciResourceIoBase;
  UINT16  PciResourceIoLimit;
  UINT32  PciResourceMem32Base;
  UINT32  PciResourceMem32Limit;
  UINT64  PciResourceMem64Base;
  UINT64  PciResourceMem64Limit;
  UINT64  PciExpressBase;
  UINT32  PciExpressSize;
} EFI_PLATFORM_PCI_DATA;

typedef struct {
  UINT8   CpuAddressWidth;
  UINT8   UncoreBusNum[4];
} EFI_PLATFORM_CPU_DATA;

typedef struct {
  UINT8   SysIoApicEnable;
  UINT8   SysSioExist;
  UINT8   IohMac0Address[6];
  UINT8   IohMac1Address[6];
} EFI_PLATFORM_SYS_DATA;

typedef struct {
  UINT32  MemTolm;
  UINT32  MemMaxTolm;
  UINT32  MemTsegSize;
  UINT32  MemIedSize;
  UINT64  MemMir0;
  UINT64  MemMir1;
  PDAT_MRC_ITEM MemMrcConfig;
} EFI_PLATFORM_MEM_DATA;

//
// This HOB definition must be consistent with what is created in the 
// PlatformInfo protocol definition.  This way the information in the
// HOB can be directly copied upon the protocol and only the strings
// will need to be updated.
//
typedef struct _EFI_PLATFORM_INFO_HOB {
  UINT8                       SystemUuid[16];     // 16 bytes
  UINT32                      Signature;          // "$PIT" 0x54495024
  UINT32                      Size;               // Size of the table
  UINT16                      Revision;           // Revision of the table
  UINT16                      Type;               // Platform Type
  UINT32                      TypeRevisionId;     // Board Revision ID
  UINT8                       CpuType;            // Cpu Type
  UINT8                       CpuStepping;        // Cpu Stepping
  UINT16                      IioSku;
  UINT16                      IioRevision;
  UINT16                      QncSku;
  UINT16                      QncRevision;
  UINT32                      FirmwareVersion;
  BOOLEAN                     ExtendedInfoValid;  // If TRUE then below fields are Valid
  UINT8                       Checksum;           // Checksum minus SystemUuid is valid in DXE only.
  EFI_STRING                  TypeStringPtr;
  EFI_GUID                    BiosPlatformDataFile;
  EFI_PLATFORM_PCI_DATA       PciData;
  EFI_PLATFORM_CPU_DATA       CpuData;
  EFI_PLATFORM_MEM_DATA       MemData;
  EFI_PLATFORM_SYS_DATA       SysData;
} EFI_PLATFORM_INFO;

#pragma pack()

#endif
