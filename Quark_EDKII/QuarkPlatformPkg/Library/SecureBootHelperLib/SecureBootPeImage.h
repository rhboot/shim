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

  SecureBootPeImage.h

Abstract:

  Header file for SecureBootPeImage.c.

--*/

#ifndef __SECUREBOOT_HELPER_PEIMAGE_HEADER_H_
#define __SECUREBOOT_HELPER_PEIMAGE_HEADER_H_

//
// Worker functions for use within this component.
//

SECUREBOOT_PEIMAGE_LOAD_INFO *
LoadPeImageWork (
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST BOOLEAN                        CalcHash
  );

//
// Services functions for export in protocols.
//

SECUREBOOT_PEIMAGE_LOAD_INFO *
EFIAPI
LoadPeImage (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST BOOLEAN                        CalcHash
  );

UINT32
EFIAPI
GetImageFromMask (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *File
  );

#endif  // __SECUREBOOT_HELPER_PEIMAGE_HEADER_H_
