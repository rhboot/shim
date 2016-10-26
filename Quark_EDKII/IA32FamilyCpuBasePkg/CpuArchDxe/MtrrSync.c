/** @file

  Code for MTRR synchronzation.

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

  Module Name:  MtrrSync.c

**/

#include "Cpu.h"

MTRR_SETTINGS   *mMtrrTable;

/**
  Initialize memory region for MTRR data.
  
  This function allocates ACPI NVS memory for MTRR data.
  Each time MTRRs are written, this memory region will be updated accordingly.

**/
VOID
InitializeMtrrData (
  VOID
  )
{
  //
  // Allocate memory for fixed MTRRs, variable MTRRs and MTRR_DEF_TYPE
  //
  mMtrrTable = AllocateAcpiNvsMemoryBelow4G (sizeof (MTRR_SETTINGS));

  PcdSet64 (PcdCpuMtrrTableAddress, (UINT64) (UINTN) mMtrrTable);
}

/**
  Synchronzies up the MTRR values with BSP for calling processor.

  This function synchronzies up the MTRR values with BSP for calling processor.

  @param  Buffer         Mtrr table address.

**/
VOID
EFIAPI
LoadMtrrData (
  VOID    *Buffer
  )
{
  MtrrSetAllMtrrs (mMtrrTable);
}

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.
  
  @return Allocated address for output.

**/
VOID*
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN   Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID*                 Buffer;

  Pages = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiACPIMemoryNVS,
                   Pages,
                   &Address
                   );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *) (UINTN) Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}
