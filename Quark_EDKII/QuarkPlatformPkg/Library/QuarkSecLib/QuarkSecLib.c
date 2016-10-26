/** @file
  Null instance of Sec Platform Hook Lib.

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

//
// The package level header files this module uses
//
#include "QuarkSecLib.h"

extern UINT32                     *TopOfStack;


/**
  Library constructor for platform SEC library.

  The constructor will be invoked by SecCore module in IA32FamilyCpuBasePkg once the transition
  to C code at SecStartup() function.
  
**/
RETURN_STATUS
EFIAPI
QNCSecLibConstructor (
  VOID
  )
{
  SecCpuInitTimer ();
  
  return RETURN_SUCCESS;
}

/**
  A developer supplied function to perform platform specific operations.

  It's a developer supplied function to perform any operations appropriate to a
  given platform. It's invoked just before passing control to PEI core by SEC
  core. Platform developer may modify the SecCoreData and PPI list that is
  passed to PEI Core. 

  @param  SecCoreData           The same parameter as passing to PEI core. It
                                could be overridden by this function.
  @param  PpiList               The default PPI list passed from generic SEC
                                part.

  @return The final PPI list that platform wishes to passed to PEI core.

**/
EFI_PEI_PPI_DESCRIPTOR *
EFIAPI
SecPlatformMain (
  IN OUT   EFI_SEC_PEI_HAND_OFF        *SecCoreData,
  IN       EFI_PEI_PPI_DESCRIPTOR      *PpiList
  )
{
	
  return NULL;
}


/**
  This interface conveys state information out of the Security (SEC) phase into PEI.

  @param  PeiServices               Pointer to the PEI Services Table.
  @param  StructureSize             Pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation (  
  IN CONST EFI_PEI_SERVICES                     **PeiServices,
  IN OUT   UINT64                               *StructureSize,
     OUT   EFI_SEC_PLATFORM_INFORMATION_RECORD  *PlatformInformationRecord
  )
{
  UINT32  *BIST;
  UINT32  Size;
  UINT32  Count;

  //
  // The entries of BIST information, together with the number of them,
  // reside in the bottom of stack, left untouched by normal stack operation.
  // This routine copies the BIST information to the buffer pointed by
  // PlatformInformationRecord for output.
  //
  Count = *(UINT32 *) (UINTN) (TopOfStack - 1);
  Size  = Count * sizeof (UINT32);

  if ((*StructureSize) < (UINT64) Size) {
    *StructureSize = Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  *StructureSize  = Size;
  BIST            = (UINT32 *) ((UINT32) TopOfStack - sizeof (UINT32) - Size);

  CopyMem (PlatformInformationRecord, BIST, Size);

  return EFI_SUCCESS;
}

