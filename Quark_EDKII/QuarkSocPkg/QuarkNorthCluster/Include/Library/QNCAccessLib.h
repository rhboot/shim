/** @file
  Library functions for Setting QNC internal network port 

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

**/

#ifndef __QNC_ACCESS_LIB_H__
#define __QNC_ACCESS_LIB_H__

#include <IntelQNCRegs.h>

#define MESSAGE_READ_DW(Port, Reg)  \
        (UINT32)((QUARK_OPCODE_READ << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define MESSAGE_WRITE_DW(Port, Reg)  \
        (UINT32)((QUARK_OPCODE_WRITE << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define ALT_MESSAGE_READ_DW(Port, Reg)  \
        (UINT32)((QUARK_ALT_OPCODE_READ << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define ALT_MESSAGE_WRITE_DW(Port, Reg)  \
        (UINT32)((QUARK_ALT_OPCODE_WRITE << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define MESSAGE_IO_READ_DW(Port, Reg)  \
        (UINT32)((QUARK_OPCODE_IO_READ << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define MESSAGE_IO_WRITE_DW(Port, Reg)  \
        (UINT32)((QUARK_OPCODE_IO_WRITE << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define MESSAGE_SHADOW_DW(Port, Reg)  \
        (UINT32)((QUARK_DRAM_BASE_ADDR_READY << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)


/**
  Read required data from QNC internal message network
**/
UINT32
EFIAPI
QNCPortRead(
  UINT8 Port, 
  UINT32 RegAddress
  );  

/**
  Write prepared data into QNC internal message network.

**/
VOID
EFIAPI
QNCPortWrite (
  UINT8 Port, 
  UINT32 RegAddress, 
  UINT32 WriteValue
  );  

/**
  Read required data from QNC internal message network
**/
UINT32
EFIAPI
QNCAltPortRead(
  UINT8 Port, 
  UINT32 RegAddress
  );  

/**
  Write prepared data into QNC internal message network.

**/
VOID
EFIAPI
QNCAltPortWrite (
  UINT8 Port, 
  UINT32 RegAddress, 
  UINT32 WriteValue
  );  

/**
  Read required data from QNC internal message network
**/
UINT32
EFIAPI
QNCPortIORead(
  UINT8 Port, 
  UINT32 RegAddress
  );  

/**
  Write prepared data into QNC internal message network.

**/
VOID
EFIAPI
QNCPortIOWrite (
  UINT8 Port, 
  UINT32 RegAddress, 
  UINT32 WriteValue
  );  

/**
  This is for the special consideration for QNC MMIO write, as required by FWG, 
  a reading must be performed after MMIO writing to ensure the expected write 
  is processed and data is flushed into chipset

**/ 
EFI_STATUS 
EFIAPI
QNCMmIoWrite (
  UINT32             MmIoAddress, 
  QNC_MEM_IO_WIDTH    Width, 
  UINT32             DataNumber, 
  VOID               *pData
  );  

VOID
EFIAPI
QncHsmmcWrite (
  UINT32 WriteValue
  );

VOID
EFIAPI
QncImrWrite (
  UINT32 ImrBaseOffset,
  UINT32 ImrLow,
  UINT32 ImrHigh,
  UINT32 ImrReadMask,
  UINT32 ImrWriteMask
  );

VOID
EFIAPI
QncIClkAndThenOr (
  UINT32 RegAddress,
  UINT32 AndValue,
  UINT32 OrValue
  );

VOID
EFIAPI
QncIClkOr (
  UINT32 RegAddress,
  UINT32 OrValue
  );

UINTN
EFIAPI
QncGetPciExpressBaseAddress (
  VOID
  );

#endif
