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

  PlatformMemoryLayout.h
    
Abstract:

  GUID used by platform drivers to keep track of memory layout across system boots.

--*/

#ifndef _PLATFORM_MEMORY_LAYOUT_H_
#define _PLATFORM_MEMORY_LAYOUT_H_

//
// GUID, used for variable vendor GUID and the GUIDed HOB.
//
#define EFI_PLATFORM_MEMORY_LAYOUT_GUID \
  { \
    0x41c7b74e, 0xb839, 0x40d9, {0xa0, 0x56, 0xe3, 0xca, 0xfc, 0x98, 0xa, 0xac } \
  }

//
// Variable name
//
#define EFI_PLATFORM_MEMORY_LAYOUT_NAME L"Platform Memory Layout"

//
// Variable attributes
//
#define EFI_PLATFORM_MEMORY_LAYOUT_ATTRIBUTES (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE)

//
// Structure for tracking a single DIMM state
//
typedef struct {
  UINT8 Present : 1;
  UINT8 Configured : 1;
  UINT8 Disabled : 1;
  UINT8 Reserved : 5;
} EFI_DIMM_STATE;

//
// Structure for tracking DIMM layout information
// This must be packed.
//
#pragma pack(1)
typedef struct {
  UINT8           DimmSets;
  UINT8           DimmsPerSet;
  UINT8           RowsPerSet;
  EFI_DIMM_STATE  State[1];
} EFI_DIMM_LAYOUT;
#pragma pack()
//
// The format of the variable or GUIDed HOB is a single EFI_DIMM_LAYOUT
// structure followed by an array of (DimmSets * DimmsPerSet) of
// EFI_DIMM_STATES structures.
//
extern EFI_GUID gEfiPlatformMemoryLayoutGuid;

#endif
