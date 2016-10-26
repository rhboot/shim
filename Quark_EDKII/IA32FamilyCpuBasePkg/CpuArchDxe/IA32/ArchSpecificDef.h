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

/**
  Return address map of interrupt handler template so that C code can generate
  interrupt handlers, and dynamically do address fix.

  @param AddressMap  Pointer to a buffer where the address map is returned.
**/
VOID
EFIAPI
GetTemplateAddressMap (
  OUT INTERRUPT_HANDLER_TEMPLATE_MAP *AddressMap
  );

/**
  Creates an IDT table starting at IdtTablPtr. It has IdtLimit/8 entries.
  Table is initialized to intxx where xx is from 00 to number of entries or
  100h, whichever is smaller. After table has been initialized the LIDT
  instruction is invoked.
 
  TableStart is the pointer to the callback table and is not used by 
  InitializedIdt but by commonEntry. CommonEntry handles all interrupts,
  does the context save and calls the callback entry, if non-NULL.
  It is the responsibility of the callback routine to do hardware EOIs.

  @param TableStart     Pointer to interrupt callback table.
  @param IdtTablePtr    Pointer to IDT table.
  @param IdtTableLimit  IDT Table limit (number of interrupt entries * 8).
**/
VOID
EFIAPI
InitializeIdt (
  IN EFI_CPU_INTERRUPT_HANDLER      *TableStart,
  IN UINTN                          *IdtTablePtr,
  IN UINT16                         IdtTableLimit
  );

#endif
