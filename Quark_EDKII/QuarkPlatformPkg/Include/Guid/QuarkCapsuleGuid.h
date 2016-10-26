/*++

Capsule format guid for Quark capsule image.

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
--*/

#ifndef _QUARK_CAPSULE_GUID_H_
#define _QUARK_CAPSULE_GUID_H_

#define QUARK_CAPSULE_GUID \
  { 0xd400d1e4, 0xa314, 0x442b, { 0x89, 0xed, 0xa9, 0x2e, 0x4c, 0x81, 0x97, 0xcb } }

#define QUARK_CAPSULE_SECURITY_HEADER_GUID \
  { 0xc3cdd08c, 0x796d, 0x491c, { 0x93, 0x54, 0x5d, 0x59, 0xc2, 0x89, 0xf2, 0xd6 }}

#define SMI_INPUT_UPDATE_CAP 0x27
#define SMI_INPUT_GET_CAP    0x28

#define SMI_CAP_FUNCTION     0xEF

#pragma pack(1)
typedef struct {
   UINT64	Address;
   UINT32	BufferOffset;	
   UINT32	Size;
   UINT32	Flags;
   UINT32	Reserved;
} CAPSULE_FRAGMENT;

typedef struct {
  UINTN			   CapsuleLocation;	// Top of the capsule that point to structure CAPSULE_FRAGMENT
  UINTN			   CapsuleSize;		// Size of the capsule
  EFI_STATUS	 Status;			// Returned status
} CAPSULE_INFO_PACKET;

typedef struct {
  UINTN           BlocksCompleted;	// # of blocks processed
  UINTN           TotalBlocks;			// Total # of blocks to be processed
  EFI_STATUS	    Status;			      // returned status
} UPDATE_STATUS_PACKET;
#pragma pack()

extern EFI_GUID gEfiQuarkCapsuleGuid;
extern EFI_GUID gEfiQuarkCapsuleSecurityHeaderGuid;

#endif
