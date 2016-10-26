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

 Io.h

 Abstract:

 Declaration of IO handling routines.

 --*/
#ifndef __IO_H
#define __IO_H

#include "core_types.h"

#include "general_definitions.h"
#include "gen5_iosf_sb_definitions.h"

// Instruction not present on Quark
#define SFENCE()

#define DEAD_LOOP()   for(;;);

////
// Define each of the IOSF_SB ports used by MRC
//

//
// Has to be 0 because of emulation static data
// initialisation:
//   Space_t EmuSpace[ SPACE_COUNT] = {0};
//
#define FREE                0x000

// Pseudo side-band ports for access abstraction
// See Wr32/Rd32 functions
#define MEM                 0x101
#define MMIO                0x102
#define DCMD                0x0A0

// Real side-band ports
// See Wr32/Rd32 functions
#define MCU                 0x001
#define HOST_BRIDGE               0x003
#define MEMORY_MANAGER               0x005
#define HTE                0x011
#define DDRPHY              0x012
#define FUSE                0x033

// End of IOSF_SB ports
////

// Pciexbar address
#define EC_BASE					0xE0000000

#define PCIADDR(bus,dev,fn,reg) ( \
        (EC_BASE) + \
		((bus) << 20) + \
		((dev) << 15) + \
		((fn)  << 12) + \
		(reg))

// Various offsets used in the building sideband commands.
#define SB_OPCODE_OFFSET      24
#define SB_PORT_OFFSET        16
#define SB_REG_OFFEST          8

// Sideband opcodes
#define SB_REG_READ_OPCODE        0x10
#define SB_REG_WRITE_OPCODE       0x11

#define SB_FUSE_REG_READ_OPCODE	  0x06
#define SB_FUSE_REG_WRITE_OPCODE  0x07

#define SB_DDRIO_REG_READ_OPCODE  0x06
#define SB_DDRIO_REG_WRITE_OPCODE 0x07

#define SB_DRAM_CMND_OPCODE       0x68
#define SB_WAKE_CMND_OPCODE       0xCA
#define SB_SUSPEND_CMND_OPCODE    0xCC

// Register addresses for sideband command and data.
#define SB_PACKET_REG        0x00D0
#define SB_DATA_REG          0x00D4
#define SB_HADR_REG          0x00D8

// We always flag all 4 bytes in the register reads/writes as required.
#define SB_ALL_BYTES_ENABLED  0xF0

#define SB_COMMAND(Opcode, Port, Reg)  \
 ((Opcode << SB_OPCODE_OFFSET) |  \
  (Port   << SB_PORT_OFFSET) |  \
  (Reg    << SB_REG_OFFEST) |  \
   SB_ALL_BYTES_ENABLED)

// iosf
#define isbM32m   WrMask32
#define isbW32m   Wr32
#define isbR32m   Rd32

// pci

void pciwrite32(
    uint32_t bus,
    uint32_t dev,
    uint32_t fn,
    uint32_t reg,
    uint32_t data);

uint32_t pciread32(
    uint32_t bus,
    uint32_t dev,
    uint32_t fn,
    uint32_t reg);

// general

uint32_t Rd32(
    uint32_t unit,
    uint32_t addr);

void Wr32(
    uint32_t unit,
    uint32_t addr,
    uint32_t data);

void WrMask32(
    uint32_t unit,
    uint32_t addr,
    uint32_t data,
    uint32_t mask);


#endif
