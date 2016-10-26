/** @file
  Null instance of Platform Sec Lib.

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

#include <PiPei.h>

#include <Ppi/SecPlatformInformation.h>


/**
  A developer supplied function to perform platform specific operations.

  It's a developer supplied function to perform any operations appropriate to a
  given platform. It's invoked just before passing control to PEI core by SEC
  core. Platform developer may modify the SecCoreData passed to PEI Core.
  It returns a platform specific PPI list that platform wishes to pass to PEI core.
  The Generic SEC core module will merge this list to join the final list passed to
  PEI core.

  @param  SecCoreData           The same parameter as passing to PEI core. It
                                could be overridden by this function.

  @return The platform specific PPI list to be passed to PEI core or
          NULL if there is no need of such platform specific PPI list.

**/
EFI_PEI_PPI_DESCRIPTOR *
EFIAPI
SecPlatformMain (
  IN OUT   EFI_SEC_PEI_HAND_OFF        *SecCoreData
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
  return EFI_SUCCESS;
}


/**
  This function provides dummy function so that SecCore can pass pass build
  Validation in IA32FamilyCpuBasePkg. All real platform library instances needs 
  to implement the real entry point in assembly.
**/
VOID
EFIAPI
_ModuleEntryPoint (
  VOID
  )
{
  return;
}
