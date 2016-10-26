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

  QNCSmmHelpers.h

Abstract:
    
--*/

#ifndef QNC_SMM_HELPERS_H
#define QNC_SMM_HELPERS_H

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmm.h"
#include "QNCxSmmHelpers.h"

//
// /////////////////////////////////////////////////////////////////////////////
// SUPPORT / HELPER FUNCTIONS (QNC version-independent)
//
VOID
QNCSmmPublishDispatchProtocols (
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
CompareEnables (
  CONST IN QNC_SMM_SOURCE_DESC *Src1,
  CONST IN QNC_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Src1  - GC_TODO: add argument description
  Src2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
CompareStatuses (
  CONST IN QNC_SMM_SOURCE_DESC *Src1,
  CONST IN QNC_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Src1  - GC_TODO: add argument description
  Src2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
CompareSources (
  CONST IN QNC_SMM_SOURCE_DESC *Src1,
  CONST IN QNC_SMM_SOURCE_DESC *Src2
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Src1  - GC_TODO: add argument description
  Src2  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

BOOLEAN
SourceIsActive (
  CONST IN QNC_SMM_SOURCE_DESC *Src
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Src - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
QNCSmmEnableSource (
  CONST QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
QNCSmmDisableSource (
  CONST QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
QNCSmmClearSource (
  CONST QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

VOID
QNCSmmClearSourceAndBlock (
  CONST QNC_SMM_SOURCE_DESC *SrcDesc
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SrcDesc - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
