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
  
    SystemNvDataHobGuid.h
    
Abstract:

  GUIDs used for System Non Volatile HOB entries in the in the HOB list and FV Guids carrying
  the System specific information.

--*/

#ifndef _SYSTEM_NV_DATA_HOB_GUID_H_
#define _SYSTEM_NV_DATA_HOB_GUID_H_

#define EFI_SYSTEM_NV_DATA_HOB_GUID \
  {0xd6e5092d, 0xc7b2, 0x4872, {0xaf, 0x66, 0xfd, 0xc0, 0xe6, 0xf9, 0x5e, 0x78}}

typedef struct {
  EFI_GUID                  SystemNvDataHobGuid;
  EFI_GUID                  SystemNvDataFvGuid;
  EFI_LBA                   StartLba;    
  UINTN                     StartLbaOffset;
  EFI_LBA                   EndLba;    
  UINTN                     EndLbaOffset;
  UINT32                    DataTypeSignature;
} NV_SYSTEM_DATA_GUID_TYPE;

extern EFI_GUID gEfiSystemNvDataHobGuid;

#endif
