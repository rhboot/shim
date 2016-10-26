/************************************************************************
 *
 * Copyright (c) 2013 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************************/
#include "mrc.h"
#include "memory_options.h"

#include "meminit.h"
#include "meminit_utils.h"
#include "prememinit.h"
#include "io.h"

// Base address for UART registers
extern uint32_t UartMmioBase;

//
// Memory Reference Code entry point when executing from BIOS
//
void Mrc( MRCParams_t *mrc_params)
{
  // configure uart base address assuming code relocated to eSRAM
  UartMmioBase = mrc_params->uart_mmio_base;

  ENTERFN();

  DPF(D_INFO, "MRC Version %04X %s %s\n", MRC_VERSION, __DATE__, __TIME__);

  // this will set up the data structures used by MemInit()
  PreMemInit(mrc_params);

  // this will initialize system memory
  MemInit(mrc_params);

  LEAVEFN();
  return;
}

