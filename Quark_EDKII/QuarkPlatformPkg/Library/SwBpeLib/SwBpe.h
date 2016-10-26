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

  SwBpe.h

Abstract:

  This library provides Software Breakpoint Event interface with Debugger.

--*/


#ifndef _SW_BPE_H_
#define _SW_BPE_H_

//
// These values represent events types the SWBPE interface supports.
//
typedef enum {
  SwbpeTypeUndefined          = 0,
  SwbpeTypeIoIn               = 1,
  SwbpeTypeIoOut              = 2,
  SwbpeTypeMemRead            = 3,
  SwbpeTypeMemWrite           = 4,
  SwbpeTypeIntXX              = 5,
  SwbpeTypeMax                = 6
} SWBPE_EVENT_TYPE;

typedef enum {
  SwbpeWidthByte              = 1,
  SwbpeWidthWord              = 2,
  SwbpeWidthDword             = 4
} SWBPE_EVENT_DATA_WIDTH;


//
// This is the data structure the SWBPE interface uses 
// to communicate with a debugger.
//
#pragma pack (push, 1)
typedef struct {
  UINT64                    EventType;
  EFI_PHYSICAL_ADDRESS      EventAddress;
  UINT64                    EventData;
  UINT64                    EventDataWidth;
  EFI_PHYSICAL_ADDRESS      DebuggerArgs;
  EFI_STATUS                DebuggerStatus;
} SWBPE_MAILBOX;
#pragma pack (pop)


VOID
SoftwareBreakpointEvent (
  IN OUT SWBPE_MAILBOX    *Mailbox
  )
/*++

Routine Description:

  This function generates an action that can be trapped by an debugger.

Arguments:

  Mailbox  - A pointer to a data structure that contains information about the event and data payload.
  
Returns: 

  EFI_SUCCESS       -  The function event was intercepted by the debugger and handled successfully
  EFI_UNSUPPORTED   -  The function event was not intercepted by the debugger
                       
--*/
;

#endif // _SW_BPE_H_
