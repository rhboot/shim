/** @file
  Header file for the PCH SPI Runtime Driver.
  
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

**/

#ifndef _PCH_SPI_H_
#define _PCH_SPI_H_

#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/S3BootScriptLib.h>  
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Spi.h>
#include "SpiCommon.h"
#include <Library/PciExpressLib.h>
#include <Protocol/BootScriptSave.h>
#include <IntelQNCRegs.h>
#include <IntelQNCLib.h>
#include <QNCAccessLib.h>
#include <Library/TimerLib.h>

#define EFI_INTERNAL_POINTER  0x00000004


//
// Function prototypes used by the SPI protocol.
//
VOID
PchSpiVirtualddressChangeEvent (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
/*++

Routine Description:

  Fixup internal data pointers so that the services can be called in virtual mode.

Arguments:

  Event     The event registered.
  Context   Event context. Not used in this event handler.

Returns: 

  None.

--*/
;

VOID
EFIAPI
SpiPhaseInit (
  VOID
  )
/*++
Routine Description:

  This function is a hook for Spi Dxe phase specific initialization

Arguments:

  None

Returns:
  
  None

--*/
;
#endif
