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

 Platform.c

 Abstract:

 The interface layer for memory controller access.
 It is supporting both real hardware platform and simulation environment.

 --*/
#include "mrc.h"
#include "memory_options.h"
#include "meminit_utils.h"
#include "io.h"

#ifdef SIM

void SimMmio32Write (
    uint32_t be,
    uint32_t address,
    uint32_t data );

void SimMmio32Read (
    uint32_t be,
    uint32_t address,
    uint32_t *data );

void SimDelayClk (
    uint32_t x2clk );

// This is a simple delay function.
// It takes "nanoseconds" as a parameter.
void delay_n(uint32_t nanoseconds)
{
  SimDelayClk( 800*nanoseconds/1000);
}
#endif

/****
 *
 ***/
uint32_t Rd32(
    uint32_t unit,
    uint32_t addr)
{
  uint32_t data;

  switch (unit)
  {
  case MEM:
    case MMIO:
#ifdef SIM
    SimMmio32Read( 1, addr, &data);
#else
    data = *PTR32(addr);
#endif
    break;

  case MCU:
    case HOST_BRIDGE:
    case MEMORY_MANAGER:
    case HTE:
    // Handle case addr bigger than 8bit
    pciwrite32(0, 0, 0, SB_HADR_REG, addr & 0xFFF00);
    addr &= 0x00FF;

    pciwrite32(0, 0, 0, SB_PACKET_REG,
        SB_COMMAND(SB_REG_READ_OPCODE, unit, addr));
    data = pciread32(0, 0, 0, SB_DATA_REG);
    break;

  case DDRPHY:
    // Handle case addr bigger than 8bit
    pciwrite32(0, 0, 0, SB_HADR_REG, addr & 0xFFF00);
    addr &= 0x00FF;

    pciwrite32(0, 0, 0, SB_PACKET_REG,
        SB_COMMAND(SB_DDRIO_REG_READ_OPCODE, unit, addr));
    data = pciread32(0, 0, 0, SB_DATA_REG);
    break;

  default:
    DEAD_LOOP()
    ;
  }

  if (unit < MEM)
    DPF(D_REGRD, "RD32 %03X %08X %08X\n", unit, addr, data);

  return data;
}

/****
 *
 ***/
void Wr32(
    uint32_t unit,
    uint32_t addr,
    uint32_t data)
{
  if (unit < MEM)
    DPF(D_REGWR, "WR32 %03X %08X %08X\n", unit, addr, data);

  switch (unit)
  {
  case MEM:
    case MMIO:
#ifdef SIM
    SimMmio32Write( 1, addr, data);
#else
    *PTR32(addr) = data;
#endif
    break;

  case MCU:
    case HOST_BRIDGE:
    case MEMORY_MANAGER:
    case HTE:
    // Handle case addr bigger than 8bit
    pciwrite32(0, 0, 0, SB_HADR_REG, addr & 0xFFF00);
    addr &= 0x00FF;

    pciwrite32(0, 0, 0, SB_DATA_REG, data);
    pciwrite32(0, 0, 0, SB_PACKET_REG,
        SB_COMMAND(SB_REG_WRITE_OPCODE, unit, addr));
    break;

  case DDRPHY:
    // Handle case addr bigger than 8bit
    pciwrite32(0, 0, 0, SB_HADR_REG, addr & 0xFFF00);
    addr &= 0x00FF;

    pciwrite32(0, 0, 0, SB_DATA_REG, data);
    pciwrite32(0, 0, 0, SB_PACKET_REG,
        SB_COMMAND(SB_DDRIO_REG_WRITE_OPCODE, unit, addr));
    break;

  case DCMD:
    pciwrite32(0, 0, 0, SB_HADR_REG, 0);
    pciwrite32(0, 0, 0, SB_DATA_REG, data);
    pciwrite32(0, 0, 0, SB_PACKET_REG,
        SB_COMMAND(SB_DRAM_CMND_OPCODE, MCU, 0));
    break;

  default:
    DEAD_LOOP()
    ;
  }
}

/****
 *
 ***/
void WrMask32(
    uint32_t unit,
    uint32_t addr,
    uint32_t data,
    uint32_t mask)
{
  Wr32(unit, addr, ((Rd32(unit, addr) & ~mask) | (data & mask)));
}

/****
 *
 ***/
void pciwrite32(
    uint32_t bus,
    uint32_t dev,
    uint32_t fn,
    uint32_t reg,
    uint32_t data)
{
  Wr32(MMIO, PCIADDR(bus,dev,fn,reg), data);
}

/****
 *
 ***/
uint32_t pciread32(
    uint32_t bus,
    uint32_t dev,
    uint32_t fn,
    uint32_t reg)
{
  return Rd32(MMIO, PCIADDR(bus,dev,fn,reg));
}

