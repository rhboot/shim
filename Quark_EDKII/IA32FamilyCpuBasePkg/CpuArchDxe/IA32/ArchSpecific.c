/** @file

  Memory Operation Functions for IA32 Architecture.

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

  Module Name:  ArchSpecific.c

**/

#include "Cpu.h"
#include "MemoryAttribute.h"

extern EFI_CPU_INTERRUPT_HANDLER   mExternalVectorTable[];

INTERRUPT_HANDLER_TEMPLATE_MAP mTemplateMap;
INTERRUPT_GATE_DESCRIPTOR      *mIdtTable = NULL;
VOID                           *mInterruptHandler = NULL;
INTERRUPT_GATE_DESCRIPTOR      *mOrigIdtEntry = NULL;
UINT16                         mOrigIdtEntryCount = 0;

UINT64   mValidMtrrAddressMask = MTRR_LIB_CACHE_VALID_ADDRESS;
UINT64   mValidMtrrBitsMask    = MTRR_LIB_MSR_VALID_MASK;

/**
  Creates an IDT and a new GDT in RAM.

  This function creates an IDT and a new GDT in RAM.

**/
VOID
InitializeDescriptorTables (
  VOID
  )
{
  UINT16                          CodeSegment;
  INTERRUPT_GATE_DESCRIPTOR       *IdtEntry;
  UINTN                           Index;
  INTERRUPT_GATE_DESCRIPTOR       *IdtTable;
  UINT8                           *InterruptHandler;
  IA32_DESCRIPTOR                 Idtr;

  //
  // Load Global Descriptor Table and update segment selectors
  //
  InitializeGdt ();

  //
  // Create Interrupt Descriptor Table
  //
  IdtTable = AllocateZeroPool (sizeof (INTERRUPT_GATE_DESCRIPTOR) * INTERRUPT_VECTOR_NUMBER);

  GetTemplateAddressMap (&mTemplateMap);
  InterruptHandler  = AllocatePool (mTemplateMap.Size * INTERRUPT_VECTOR_NUMBER);

  CodeSegment       = AsmReadCs ();

  //
  // Get original IDT address and size and save original IDT entry.
  //
  AsmReadIdtr ((IA32_DESCRIPTOR *) &Idtr);
  mOrigIdtEntryCount = (UINT16) ((Idtr.Limit + 1) / sizeof (INTERRUPT_GATE_DESCRIPTOR));
  ASSERT (mOrigIdtEntryCount <= INTERRUPT_VECTOR_NUMBER);
  mOrigIdtEntry = AllocateCopyPool (Idtr.Limit + 1, (VOID *) Idtr.Base);
  ASSERT (mOrigIdtEntry != NULL);

  //
  // Copy original IDT entry.
  //
  IdtEntry = IdtTable;
  CopyMem (IdtEntry, (VOID *) Idtr.Base, Idtr.Limit + 1);

  //
  // Save IDT table pointer and Interrupt table pointer
  //
  mIdtTable         = IdtTable;
  mInterruptHandler = InterruptHandler;

  for (Index = 0; Index < INTERRUPT_VECTOR_NUMBER; Index ++) {
    //
    // Update all IDT entries to use current CS value.
    //
    IdtEntry[Index].SegmentSelector = CodeSegment;
    if (Index < mOrigIdtEntryCount) {
      //
      // Skip original IDT entry.
      //
      continue;
    }
    //
    // Set the address of interrupt handler to the rest IDT entry.
    //
    SetInterruptDescriptorTableHandlerAddress (Index);
  }

  InitializeIdt (
    &(mExternalVectorTable[0]),
    (UINTN *) IdtTable,
    (UINT16) (sizeof (INTERRUPT_GATE_DESCRIPTOR) * INTERRUPT_VECTOR_NUMBER)
    );

  //
  // Initialize Exception Handlers
  //
  InitializeException (mOrigIdtEntryCount);
}

/**
  Set Interrupt Descriptor Table Handler Address.

  @param Index        The Index of the interrupt descriptor table handle.

**/
VOID
SetInterruptDescriptorTableHandlerAddress (
  IN UINTN       Index
  )
{
  UINTN          ExceptionHandle;

  //
  // Get the address of handler for entry
  //
  ExceptionHandle = (UINTN)mInterruptHandler + Index * mTemplateMap.Size;
  CopyMem ((VOID *)ExceptionHandle, mTemplateMap.Start, mTemplateMap.Size);
  *(UINT32 *) (ExceptionHandle + mTemplateMap.FixOffset) = Index;

  mIdtTable[Index].OffsetLow  = (UINT16) ExceptionHandle;
  mIdtTable[Index].Attributes = INTERRUPT_GATE_ATTRIBUTE;
  //
  // 8e00;
  //
  mIdtTable[Index].OffsetHigh = (UINT16) (ExceptionHandle >> 16);
}

/**
  Restore original Interrupt Descriptor Table Handler Address.

  @param Index        The Index of the interrupt descriptor table handle.

**/
VOID
RestoreInterruptDescriptorTableHandlerAddress (
  IN UINTN       Index
  )
{
  if (Index < mOrigIdtEntryCount) {
    mIdtTable[Index].OffsetLow  = mOrigIdtEntry[Index].OffsetLow;
    mIdtTable[Index].OffsetHigh = mOrigIdtEntry[Index].OffsetHigh;
  }
}
