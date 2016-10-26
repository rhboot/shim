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
  FirmwareVolumeBlockSecurity.h

Abstract:
  Protocol header file for FirmwareVolmeBlockSecurity Protocol.

--*/

#ifndef _FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL_H_
#define _FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL_H_

//
// Constant definitions.
//

//
// Type definitions.
//

///
/// Forward reference for pure ANSI compatability
///
typedef struct _FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL  FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL;

///
/// Extern the GUID for protocol users.
///
extern EFI_GUID gFirmwareVolumeBlockSecurityGuid;

//
// Service typedefs to follow.
//

/**
  Security library to authenticate the Firmware Volume.

  @param ImageBaseAddress         Pointer to the current Firmware Volume under consideration

  @retval EFI_SUCCESS             Firmware Volume is legal.
  @retval EFI_SECURITY_VIOLATION  Firmware Volume is NOT legal.

  **/
typedef
EFI_STATUS
(EFIAPI *FVB_SECURITY_AUTHENTICATE_IMAGE) (
  IN VOID *ImageBaseAddress
  );

///
/// Fvb security protocol declaration.
///
struct _FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL {
  FVB_SECURITY_AUTHENTICATE_IMAGE     SecurityAuthenticateImage;              ///< see service typedef.
};

#endif  // _FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL_H_
