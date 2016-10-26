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

    SmmScriptLib.h
    
Abstract:

  This is an implementation of the BootScript at run time.  

--*/

#ifndef _SMM_SCRIPT_LIB_H_
#define _SMM_SCRIPT_LIB_H_

#include <PiDxe.h>
#include <Tbd/EfiBootScript.h>

typedef EFI_PHYSICAL_ADDRESS     EFI_SMM_SCRIPT_TABLE;

EFI_STATUS
EFIAPI
InitializeSmmScriptLib (
  IN  EFI_SYSTEM_TABLE     *SystemTable,
  IN  UINTN                SmmScriptTablePages,
  OUT EFI_PHYSICAL_ADDRESS *SmmScriptTableBase
  )
/*++

Routine Description:

  Intialize Boot Script table.
  
Arguments:

  SystemTable         -  Pointer to the EFI sytem table
  SmmScriptTablePages -  The expected ScriptTable page number
  SmmScriptTableBase  -  The returned ScriptTable base address

Returns:  

  EFI_OUT_OF_RESOURCES   -  No resource to do the initialization.
  EFI_SUCCESS            -  Function has completed successfully.
  
--*/
;

EFI_STATUS 
EFIAPI
SmmBootScriptCreateTable (
  IN OUT   EFI_SMM_SCRIPT_TABLE    *ScriptTable,
  IN       UINTN                   Type
  )
/*++

Routine Description:

  Create Boot Script table.
  
Arguments:

  ScriptTable  -  Pointer to the boot script table to create.
  Type         -  The type of table to creat.


Returns:  

  EFI_INVALID_PARAMETER  -  Invalid parameter detected.
  EFI_SUCCESS            -  Function has completed successfully.
  
--*/
;

EFI_STATUS 
EFIAPI
SmmBootScriptWrite (
  IN OUT   EFI_SMM_SCRIPT_TABLE    *ScriptTable,
  IN       UINTN                   Type,
  IN       UINT16                  OpCode,
  ...
  )
/*++

Routine Description:

  Write to boot script table. Format is defined in AcpiScritSave Protocol.
  Currently it supports MemWrite, IoWrite, PciCfgWrite.

Arguments:

  ScriptTable  -  Pointer to the script table to write to.
  Type         -  Not used.
  OpCode       -  Table write's Opcode.
  ...          -  Parameter of the OpCode.

Returns:

  EFI_INVALID_PARAMETER  -  Invalid parameter passed in detected.
  EFI_SUCCESS            -  Function completed successfully.
  Other                  -  Error occurred during execution.
  
--*/
;

EFI_STATUS
EFIAPI
SmmBootScriptCloseTable (
  IN EFI_SMM_SCRIPT_TABLE        ScriptTableBase,
  IN EFI_SMM_SCRIPT_TABLE        ScriptTablePtr,
  IN UINTN                       Type
  )
/*++

Routine Description:

  Close Boot Script table.
  
Arguments:

  ScriptTableBase  -  Pointer to the boot script table to create.
  ScriptTablePtr   -  Pointer to the script table to write to.
  Type             -  The type of table to creat.


Returns:  

  EFI_SUCCESS            -  Function has completed successfully.
  
--*/
;

#endif
