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

  ProcessorDef.h

Abstract:

  Definition for IA32 processor

**/

#ifndef _PROCESSOR_DEF_H_
#define _PROCESSOR_DEF_H_

#include <FrameworkDxe.h>
#include <Protocol/Cpu.h>

#pragma pack(1)

typedef struct {
  UINT16  OffsetLow;
  UINT16  SegmentSelector;
  UINT16  Attributes;
  UINT16  OffsetHigh;
} INTERRUPT_GATE_DESCRIPTOR;

#pragma pack()

typedef struct {
  VOID  *Start;
  UINTN Size;
  UINTN FixOffset;
} INTERRUPT_HANDLER_TEMPLATE_MAP;

typedef struct {
  UINT8 *RendezvousFunnelAddress;
  UINTN PModeEntryOffset;
  UINTN FlatJumpOffset;
  UINTN Size;
} MP_ASSEMBLY_ADDRESS_MAP;

/**
  Get starting address and size of the rendezvous entry for APs.
  Information for fixing a jump instruction in the code is also returned.

  @param AddressMap  Output buffer for address map information.
**/
VOID
EFIAPI
AsmGetAddressMap (
  OUT MP_ASSEMBLY_ADDRESS_MAP    *AddressMap
  );

#endif
