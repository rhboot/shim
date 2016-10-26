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
  
    FvbExtension.h

Abstract:

  FVB Extension protocol that extends the FVB Class in a component fashion.

--*/

#ifndef _FVB_EXTENSION_H_
#define _FVB_EXTENSION_H_

 typedef struct _EFI_FVB_EXTENSION_PROTOCOL EFI_FVB_EXTENSION_PROTOCOL;

//  FVB Extension Function Prototypes
//
typedef
EFI_STATUS
(EFIAPI * EFI_FV_ERASE_CUSTOM_BLOCK) (
  IN EFI_FVB_EXTENSION_PROTOCOL   *This,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba
);

//
// IPMI TRANSPORT PROTOCOL
//
 struct _EFI_FVB_EXTENSION_PROTOCOL {
  EFI_FV_ERASE_CUSTOM_BLOCK               EraseFvbCustomBlock;
 };

//

extern EFI_GUID                           gEfiFvbExtensionProtocolGuid;

#endif

