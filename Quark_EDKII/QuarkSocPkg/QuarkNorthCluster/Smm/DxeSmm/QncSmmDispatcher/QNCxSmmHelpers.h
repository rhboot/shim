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

  QNCxSmmHelpers.h

Abstract:
    
--*/

#ifndef QNCX_SMM_HELPERS_H
#define QNCX_SMM_HELPERS_H

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmm.h"

EFI_STATUS
QNCSmmInitHardware (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

EFI_STATUS
QNCSmmEnableGlobalSmiBit (
  VOID
  )
/*++

Routine Description:

  Enables the QNC to generate SMIs. Note that no SMIs will be generated
  if no SMI sources are enabled. Conversely, no enabled SMI source will
  generate SMIs if SMIs are not globally enabled. This is the main 
  switchbox for SMI generation.

Arguments:

  None

Returns:

  EFI_SUCCESS.  
  Asserts, otherwise.

--*/
;

EFI_STATUS
QNCSmmClearSmi (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
QNCSmmSetAndCheckEos (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
QNCSmmGetSciEn (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

//
// ///////////////////////////////////////////////////////////////////////////
//
// These may or may not need to change w/ the QNC version;
// they're here because they're highly IA-32 dependent.
//
BOOLEAN
ReadBitDesc (
  CONST QNC_SMM_BIT_DESC *BitDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  BitDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
WriteBitDesc (
  CONST QNC_SMM_BIT_DESC  *BitDesc,
  CONST BOOLEAN          ValueToWrite
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  BitDesc       - GC_TODO: add argument description
  ValueToWrite  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
