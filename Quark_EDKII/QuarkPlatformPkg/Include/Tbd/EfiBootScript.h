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

  EfiBootScript.h

Abstract:

  

--*/

#ifndef _EFI_BOOT_SCRIPT_H_
#define _EFI_BOOT_SCRIPT_H_

#define EFI_ACPI_S3_RESUME_SCRIPT_TABLE         0x00

//
// Boot Script Opcode Definitions
//
typedef const UINT16  EFI_BOOT_SCRIPT_OPCODE;

#define EFI_BOOT_SCRIPT_IO_WRITE_OPCODE               0x00
#define EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE          0x01
#define EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE              0x02
#define EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE         0x03
#define EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE       0x04
#define EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE  0x05
#define EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE          0x06
#define EFI_BOOT_SCRIPT_STALL_OPCODE                  0x07
#define EFI_BOOT_SCRIPT_DISPATCH_OPCODE               0x08

//
// Extensions to boot script definitions
//
//#define EFI_BOOT_SCRIPT_MEM_POLL_OPCODE               0x09
#define EFI_BOOT_SCRIPT_INFORMATION_OPCODE            0x0A
#define EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE      0x0B
#define EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE 0x0C

#define EFI_BOOT_SCRIPT_TABLE_OPCODE                  0xAA
#define EFI_BOOT_SCRIPT_TERMINATE_OPCODE              0xFF

#pragma pack(1)

//
// EFI Boot Script Width
//
/*typedef enum {
  EfiBootScriptWidthUint8,
  EfiBootScriptWidthUint16,
  EfiBootScriptWidthUint32,
  EfiBootScriptWidthUint64,
  EfiBootScriptWidthFifoUint8,
  EfiBootScriptWidthFifoUint16,
  EfiBootScriptWidthFifoUint32,
  EfiBootScriptWidthFifoUint64,
  EfiBootScriptWidthFillUint8,
  EfiBootScriptWidthFillUint16,
  EfiBootScriptWidthFillUint32,
  EfiBootScriptWidthFillUint64,
  EfiBootScriptWidthMaximum
} EFI_BOOT_SCRIPT_WIDTH;
*/

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
} EFI_BOOT_SCRIPT_GENERIC_HEADER;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT16  Version;
  UINT32  TableLength;
  UINT16  Reserved[2];
} EFI_BOOT_SCRIPT_TABLE_HEADER;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
} EFI_BOOT_SCRIPT_COMMON_HEADER;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT32                Count;
  UINT64                Address;
} EFI_BOOT_SCRIPT_IO_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT64                Address;
} EFI_BOOT_SCRIPT_IO_READ_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT32                Count;
  UINT64                Address;
} EFI_BOOT_SCRIPT_MEM_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT64                Address;
} EFI_BOOT_SCRIPT_MEM_READ_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT32                Count;
  UINT64                Address;
} EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT32                Count;
  UINT64                Address;
  UINT16                Segment;
} EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT64                Address;
} EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT64                Address;
  UINT16                Segment;
} EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE;


typedef struct {
  UINT16                    OpCode;
  UINT8                     Length;
  UINT64                    SlaveAddress;
  UINT64                    Command;
  UINT32                    Operation;
  BOOLEAN                   PecCheck;
  UINT32                    DataSize;
} EFI_BOOT_SCRIPT_SMBUS_EXECUTE;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT64  Duration;
} EFI_BOOT_SCRIPT_STALL;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  EFI_PHYSICAL_ADDRESS  EntryPoint;
} EFI_BOOT_SCRIPT_DISPATCH;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT32  Width;
  UINT64  Address;
  UINT64  BitMask;
  UINT64  BitValue;
  UINT64  Duration;
  UINT64  LoopTimes;
} EFI_BOOT_SCRIPT_MEM_POLL;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                InformationLength;  
  EFI_PHYSICAL_ADDRESS  Information;
} EFI_BOOT_SCRIPT_INFORMATION;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
} EFI_BOOT_SCRIPT_TERMINATE;

typedef union {
  EFI_BOOT_SCRIPT_GENERIC_HEADER        *Header;
  EFI_BOOT_SCRIPT_TABLE_HEADER          *TableInfo;
  EFI_BOOT_SCRIPT_IO_WRITE              *IoWrite;
  EFI_BOOT_SCRIPT_IO_READ_WRITE         *IoReadWrite;
  EFI_BOOT_SCRIPT_MEM_WRITE             *MemWrite;
  EFI_BOOT_SCRIPT_MEM_READ_WRITE        *MemReadWrite;
  EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE      *PciWrite;
  EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE *PciReadWrite;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE      *PciWrite2;
  EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE *PciReadWrite2;
  EFI_BOOT_SCRIPT_SMBUS_EXECUTE         *SmbusExecute;
  EFI_BOOT_SCRIPT_STALL                 *Stall;
  EFI_BOOT_SCRIPT_DISPATCH              *Dispatch;
  EFI_BOOT_SCRIPT_MEM_POLL              *MemPoll;
  EFI_BOOT_SCRIPT_INFORMATION           *Information;  
  EFI_BOOT_SCRIPT_TERMINATE             *Terminate;
  EFI_BOOT_SCRIPT_COMMON_HEADER         *CommonHeader;
  UINT8                                 *Raw;
} BOOT_SCRIPT_POINTERS;

#pragma pack()

#endif
