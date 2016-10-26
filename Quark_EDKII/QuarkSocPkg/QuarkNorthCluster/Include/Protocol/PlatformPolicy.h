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

  PlatformPolicy.h
    
Abstract:

  Protocol used for Platform Policy definition.

--*/

#ifndef _PLATFORM_POLICY_H_
#define _PLATFORM_POLICY_H_

typedef struct _EFI_PLATFORM_POLICY_PROTOCOL EFI_PLATFORM_POLICY_PROTOCOL;

#define EFI_PLATFORM_POLICY_PROTOCOL_GUID \
  { \
    0x2977064f, 0xab96, 0x4fa9, { 0x85, 0x45, 0xf9, 0xc4, 0x02, 0x51, 0xe0, 0x7f } \
  }

//
// Protocol to describe various platform information. Add to this as needed.
//
struct _EFI_PLATFORM_POLICY_PROTOCOL {
  UINT8 NumRsvdSmbusAddresses;
  UINT8 *RsvdSmbusAddresses;
};

extern EFI_GUID gEfiPlatformPolicyProtocolGuid;

#endif
