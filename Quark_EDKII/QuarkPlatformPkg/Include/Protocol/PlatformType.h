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
  PlatformType.h

Abstract:
  This file defines platform policies for Platform Type.

--*/

#ifndef _PLATFORM_TYPE_H_
#define _PLATFORM_TYPE_H_

#include <Guid/PlatformInfo.h>

typedef struct _EFI_PLATFORM_TYPE_PROTOCOL {
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
  UINT32                      FlashDeviceSize;    // Size in bytes of flash device on this platform.  
  UINT64                      IioStringPtr;
  UINT64                      QncStringPtr;
} EFI_PLATFORM_TYPE_PROTOCOL;

extern EFI_GUID gEfiPlatformTypeProtocolGuid;

#endif
