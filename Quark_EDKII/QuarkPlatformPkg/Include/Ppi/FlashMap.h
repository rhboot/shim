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

  FlashMap.h

Abstract:

  FlashMap PPI defined in Tiano

  This code abstracts FlashMap access

--*/

#ifndef _PEI_FLASH_MAP_PPI_H_
#define _PEI_FLASH_MAP_PPI_H_

#define PEI_FLASH_MAP_PPI_GUID \
  { \
    0xf34c2fa0, 0xde88, 0x4270, {0x84, 0x14, 0x96, 0x12, 0x22, 0xf4, 0x52, 0x1c} \
  }

#include "EfiFlashMap.h"

typedef struct _PEI_FLASH_MAP_PPI PEI_FLASH_MAP_PPI;

//
// Functions
//
typedef
EFI_STATUS
(EFIAPI *PEI_GET_FLASH_AREA_INFO) (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN PEI_FLASH_MAP_PPI            * This,
  IN  EFI_FLASH_AREA_TYPE         AreaType,
  IN  EFI_GUID                    * AreaTypeGuid,
  OUT UINT32                      *NumEntries,
  OUT EFI_FLASH_SUBAREA_ENTRY     **Entries
  );

//
// PEI_FLASH_MAP_PPI
//
struct _PEI_FLASH_MAP_PPI {
  PEI_GET_FLASH_AREA_INFO GetAreaInfo;
};

extern EFI_GUID gPeiFlashMapPpiGuid;

#endif // _PEI_FLASH_MAP_PPI_H_
