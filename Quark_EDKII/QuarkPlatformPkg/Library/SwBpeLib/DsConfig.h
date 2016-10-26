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

  DsConfig.h

Abstract:

  This header provides Debug Store configuration using Software Breakpoint Events.

--*/


#ifndef _DS_CONFIG_H_
#define _DS_CONFIG_H_

//
// This is the GUID that is used to uniquely identify DS Config SWBPEs
// to the debugger
//
#define DS_CONFIG_SWBPE_GUID   \
    { 0x1a237f6e, 0xfa50, 0x4338, 0xae, 0x36, 0x6c, 0x89, 0x38, 0xa7, 0xf2, 0xc2 }

//
// These are the ID values for DS SWBPEs
//
typedef enum {
  DsConfigSwbpeIdUndefined         = 0,
  DsConfigSwBpeIdCheck             = 1,
  DsConfigSwBpeIdNemStart          = 2,
  DsConfigSwBpeIdNemStop           = 3,
  DsConfigSwBpeIdDramStart         = 4,
  DsConfigSwBpeIdDramStop          = 5,
  DsConfigSwBpeIdMax               = 6
} DS_CONFIG_SWBPE_ID;


//
// This is the DS-specific data that firmware will
// pass to the debugger via the DebuggerArgs field of
// SWBPE_MAILBOX.
//
#pragma pack (push, 1)
typedef struct {
    EFI_GUID                    Guid;
    UINT64                      Id;
    EFI_PHYSICAL_ADDRESS        BaseAddress;
    UINT64                      Size;
    EFI_STATUS                  *Status;
} DS_CONFIG_SWBPE_MAILBOX;
#pragma pack (pop)


VOID
DsBreakpointEvent (
  IN  DS_CONFIG_SWBPE_ID        Id,
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress, OPTIONAL
  IN  UINTN                     Size, OPTIONAL
  OUT EFI_STATUS                *Status
  )
/*++

Routine Description:

  This function generates a SWBPE event.  It will create a DS_CONFIG_SWBPE_MAILBOX structure
  with the DS_CONFIG_SWBPE_GUID and the input arguments and trigger an implementation specific
  SWBPE.

Arguments:

  Id                - The event being signalled to the debugger
  BaseAddress       - The address of the buffer.  This is not used for some ID.
  Size              - The size of the buffer.  This is not used for some ID.
  Status            - The status returned by the debugger.
  
Returns: 

  EFI_SUCCESS       -  The function event was intercepted by the debugger and handled successfully
  EFI_UNSUPPORTED   -  The function event was not intercepted by the debugger and not handled 
                       
--*/
;

#endif // _DS_CONFIG_H_
