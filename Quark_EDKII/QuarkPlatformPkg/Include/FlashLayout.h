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

  FlashLayout.h

Abstract:

  Platform specific flash layout.

--*/

#ifndef _FLASH_LAYOUT_H_
#define _FLASH_LAYOUT_H_

#include "EfiFlashMap.h"
#include <FlashMap.h>

// 
// Length of Efi Runtime Updatable Firmware Volume Header
//
#define EFI_RUNTIME_UPDATABLE_FV_HEADER_LENGTH  0x48
#define FLASH_BLOCK_SIZE            0x1000

//
// Flash supports 32K block size
//
#define FVB_MEDIA_BLOCK_SIZE        FLASH_BLOCK_SIZE

#define RUNTIME_FV_BASE_ADDRESS     FLASH_REGION_RUNTIME_UPDATABLE_BASE
#define RUNTIME_FV_LENGTH           (FLASH_REGION_RUNTIME_UPDATABLE_SIZE + FLASH_REGION_NV_FTW_SPARE_SIZE)
#define RUNTIME_FV_BLOCK_NUM        (RUNTIME_FV_LENGTH / FVB_MEDIA_BLOCK_SIZE)

#define FV_MAIN_BASE_ADDRESS        FLASH_REGION_FVMAIN_BASE
#define MAIN_BIOS_BLOCK_NUM         (FLASH_REGION_FVMAIN_SIZE / FVB_MEDIA_BLOCK_SIZE)

#endif
