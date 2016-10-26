/** @file

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

  PlatformSecServicesLib.h

Abstract:

  Provide platform specific services to SEC stage Reset Vector Driver.

--*/

#ifndef __PLATFORM_SEC_SERVICES_LIB_H__
#define __PLATFORM_SEC_SERVICES_LIB_H__

/** Copy and call fixed recovery image.

**/
VOID
EFIAPI
PlatformCopyFixedRecoveryImageToSramAndCall (
  VOID
  );

/** Check if running recovery image in SRAM.

  @retval  TRUE    If running recovery image from SRAM.
  @retval  FALSE   If not recovery image from SRAM.
**/
BOOLEAN
EFIAPI
PlatformAmIFixedRecoveryRunningFromSram (
  VOID
  );

/** Check if force recovery conditions met.

  @return 0 if conditions not met or already running recovery image in SRAM.
  @return address PlatformCopyFixedRecoveryImageToSramAndCall if conditions met.
**/
UINT32
EFIAPI
PlatformIsForceRecoveryConditionsMet (
  VOID
  );


/** Copy boot stage1 image to sram and Call it.

  This routine is called from the QuarkResetVector to copy stage1 image to SRAM
  the routine should NOT be called from QuarkSecLib.

**/
VOID
EFIAPI
PlatformCopyBootStage1ImageToSramAndCall (
  VOID
  );
#endif
