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

  DsConfig.c

Abstract:

  This file implements DS debug feature configuration functionality as defined by 
  the "Enabling DS for External Debugger" feature documentation.

--*/

#include "CommonHeader.h"

VOID
DsBreakpointEvent (
  DS_CONFIG_SWBPE_ID        Id,
  EFI_PHYSICAL_ADDRESS      BaseAddress,
  UINTN                     Size,
  EFI_STATUS                *Status
  )
/*++

Routine Description:

  This function generates a SWBPE event.  It will create a DS_CONFIG_SWBPE_MAILBOX structure
  with the DS_CONFIG_SWBPE_GUID and the input arguments and trigger an implementation specific
  SWBPE.

Arguments:

  Id - 
  BaseAddress - 
  Size -
  
Returns: 

  EFI_SUCCESS       -  The function event was intercepted by the debugger and handled successfully
  EFI_UNSUPPORTED   -  The function event was not intercepted by the debugger and no 
                       
--*/
{
  SWBPE_MAILBOX               Mailbox     = {0, 0, 0, 0, 0, 0};
  DS_CONFIG_SWBPE_MAILBOX     DsMailbox   = {DS_CONFIG_SWBPE_GUID, 0, 0, 0, 0};

  //
  // Setup DS-specific data for the debugger
  //
  DsMailbox.Id               = Id;
  DsMailbox.BaseAddress      = BaseAddress;
  DsMailbox.Size             = Size;

  //
  // Setup generic SWBPE data for a DS event
  //
  Mailbox.EventType         = DS_CONFIG_SWBPE_TYPE;
  Mailbox.EventAddress      = DS_CONFIG_SWBPE_ADDRESS;
  Mailbox.EventData         = DS_CONFIG_SWBPE_DATA;
  Mailbox.EventDataWidth    = DS_CONFIG_SWBPE_DATA_WIDTH ;
  Mailbox.DebuggerArgs      = (EFI_PHYSICAL_ADDRESS) (UINTN) (&DsMailbox);

  //
  // Invoke a DS SWBPE
  //
  SoftwareBreakpointEvent (&Mailbox);

  //
  // Return the debuggers status to the caller
  //
  *Status = Mailbox.DebuggerStatus;
}

