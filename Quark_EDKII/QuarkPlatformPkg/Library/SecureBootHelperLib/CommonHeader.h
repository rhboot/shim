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

  CommonHeader.h

Abstract:

  The common header file for the SecureBootHelperLib component.

--*/

#ifndef _SECUREBOOT_HELPER_LIB_COMMON_HEADER_H_
#define _SECUREBOOT_HELPER_LIB_COMMON_HEADER_H_

#include <PiDxe.h>

#include <Protocol/TcgService.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DebugPort.h>
#include <Protocol/VariableWrite.h>
#include <Protocol/Security.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DxeServicesLib.h>

#include <Library/BaseCryptLib.h>
#include <Library/SecurityManagementLib.h>
#include <Library/PlatformSecureLib.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/ImageAuthentication.h>

#include <Protocol/SecureBootHelper.h>

//
// Structure definitions.
//

/// Private Data shared between modules in this component.
typedef struct {
  UINT32                            Signature;  ///< 32bit structure signature.
  SECUREBOOT_HELPER_PROTOCOL        SBHP;       ///< Secure Boot Helper protocol storage.
  EFI_HANDLE                        ImageHandle;///< Image handle passed to SecureBootHelperInitialize.
} SECUREBOOT_HELPER_PRIVATE_DATA;

#define SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('S', 'B', 'H', 'P')
#define SECUREBOOT_HELPER_PRIVATE_FROM_SBHP(a)    CR (a, SECUREBOOT_HELPER_PRIVATE_DATA, SBHP, SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE)

//
// Prototypes for functions private to this component.
//

VOID *
EFIAPI
MyGetFileBufferByFilePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FilePath,
  OUT      UINTN                          *FileSize,
  OUT UINT32                              *AuthenticationStatus
  );

EFI_STATUS
EFIAPI
MyInt2OctStr (
  IN     CONST UINTN                      *Integer,
  IN     UINTN                            IntSizeInWords,
     OUT UINT8                            *OctetString,
  IN     UINTN                            OSSizeInBytes
  );

//
// Include prototypes for local source modules.
//
#include "SecureBootCrypto.h"
#include "SecureBootVariable.h"
#include "SecureBootPeImage.h"

#endif  // _SECUREBOOT_HELPER_LIB_COMMON_HEADER_H_
