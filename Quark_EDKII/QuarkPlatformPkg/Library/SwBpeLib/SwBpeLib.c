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

  SwBpeLib.c

Abstract:

  This library provides Software Breakpoint Event interface with Debugger.

--*/

#include "CommonHeader.h"

VOID 
DoSwbpeTypeIoOut (
  UINTN       PortAddress,
  UINTN       PortData,
  UINTN       PortDataWidth,
  UINTN       MailboxAddress
  );

UINTN
DoSwbpeTypeIoIn (
  UINTN       PortAddress,
  UINTN       PortDataWidth,
  UINTN       MailboxAddress
  );

UINTN
DoSwbpeTypeMemRead (
  UINTN       MemAddress,
  UINTN       MemDataWidth,
  UINTN       MailboxAddress
  );

VOID
DoSwbpeTypeMemWrite (
  UINTN       MemAddress,
  UINTN       MemData,
  UINTN       MemDataWidth,
  UINTN       MailboxAddress
  );

VOID
DoSwbpeTypeInt3 (
  UINTN MailboxAddress
  );

VOID
SoftwareBreakpointEvent (
  SWBPE_MAILBOX *Mailbox
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
{
  //
  // Initialize the mailbox to check for debugger presence
  //
  Mailbox->DebuggerStatus    = EFI_UNSUPPORTED;

  //
  // Determine the type of event, and invoke it
  //
  switch (Mailbox->EventType) {

    case SwbpeTypeIoOut:
      DoSwbpeTypeIoOut (
        (UINTN) Mailbox->EventAddress,
        (UINTN) Mailbox->EventData,
        (UINTN) Mailbox->EventDataWidth,
        (UINTN) Mailbox
        );
       break;

    case SwbpeTypeIoIn:
      Mailbox->EventData = DoSwbpeTypeIoIn (
                             (UINTN) Mailbox->EventAddress,
                             (UINTN) Mailbox->EventDataWidth,
                             (UINTN) Mailbox
                             );
      break;

    case SwbpeTypeMemWrite:
      DoSwbpeTypeMemWrite (
        (UINTN) Mailbox->EventAddress,
        (UINTN) Mailbox->EventData,
        (UINTN) Mailbox->EventDataWidth,
        (UINTN) Mailbox
        );
      break;

    case SwbpeTypeMemRead:
      Mailbox->EventData = DoSwbpeTypeMemRead (
                             (UINTN) Mailbox->EventAddress,
                             (UINTN) Mailbox->EventDataWidth,
                             (UINTN) Mailbox
                              );
      break;

    case SwbpeTypeIntXX:
      DoSwbpeTypeInt3 ((UINTN) Mailbox);
      break;

    default:
      //
      // If type not recognized then just leave EFI_UNSUPPORTED
      //
      break;
  }
}

