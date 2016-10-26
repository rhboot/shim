/** @file
  Set a IDT entry for debug purpose

  Set a IDT entry for interrupt vector 3 for debug purpose for IA32 platform

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
#include "ScriptExecute.h"
//
// INTERRUPT_GATE_DESCRIPTOR and SetIdtEntry () are used to setup IDT to do debug
//

#pragma pack(1)

typedef struct {
  UINT16  OffsetLow;
  UINT16  SegmentSelector;
  UINT16  Attributes;
  UINT16  OffsetHigh;
} INTERRUPT_GATE_DESCRIPTOR;

#define INTERRUPT_GATE_ATTRIBUTE   0x8e00

#pragma pack()
/**
  Set a IDT entry for interrupt vector 3 for debug purpose.

  @param  AcpiS3Context  a pointer to a structure of ACPI_S3_CONTEXT

**/
VOID
SetIdtEntry (
  IN ACPI_S3_CONTEXT     *AcpiS3Context
  )
{
  INTERRUPT_GATE_DESCRIPTOR                     *IdtEntry;
  IA32_DESCRIPTOR                               *IdtDescriptor;
  UINTN                                         S3DebugBuffer;

  //
  // Restore IDT for debug
  //
  IdtDescriptor = (IA32_DESCRIPTOR *) (UINTN) (AcpiS3Context->IdtrProfile);
  IdtEntry = (INTERRUPT_GATE_DESCRIPTOR *)(IdtDescriptor->Base + (3 * sizeof (INTERRUPT_GATE_DESCRIPTOR)));
  S3DebugBuffer = (UINTN) (AcpiS3Context->S3DebugBufferAddress);

  IdtEntry->OffsetLow       = (UINT16)S3DebugBuffer;
  IdtEntry->SegmentSelector = (UINT16)AsmReadCs ();
  IdtEntry->Attributes      = (UINT16)INTERRUPT_GATE_ATTRIBUTE;
  IdtEntry->OffsetHigh      = (UINT16)(S3DebugBuffer >> 16);

  AsmWriteIdtr (IdtDescriptor);
}

