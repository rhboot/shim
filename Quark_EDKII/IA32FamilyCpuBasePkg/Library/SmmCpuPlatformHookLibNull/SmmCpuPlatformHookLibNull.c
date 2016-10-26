/** @file

  CPU Configuration Library.

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

#include <Library/SmmCpuPlatformHookLib.h>


/**
  Checks if platform produces a valid SMI. 
  
  This function checks if platform produces a valid SMI. This function is 
  called at SMM entry to detect if this is a spurious SMI. This function 
  must be implemented in an MP safe way because it is called by multiple CPU
  threads.
  
  @retval TRUE              There is a valid SMI
  @retval FALSE             There is no valid SMI

**/
BOOLEAN
EFIAPI
PlatformValidSmi (
  VOID
  )
{
  return TRUE;
}


/**
  Clears platform top level SMI status bit. 
  
  This function clears platform top level SMI status bit.
  
  @retval TRUE              The platform top level SMI status is cleared.
  @retval FALSE             The paltform top level SMI status cannot be cleared.

**/
BOOLEAN
EFIAPI
ClearTopLevelSmiStatus (
  VOID
  )
{
  return TRUE;
}


/**
  Performs platform specific way of SMM BSP election.
  
  This function performs platform specific way of SMM BSP election.
  
  @param  IsBsp             Output parameter. TRUE: the CPU this function executes
                            on is elected to be the SMM BSP. FALSE: the CPU this 
                            function executes on is to be SMM AP.

  @retval EFI_SUCCESS       The function executes successfully.
  @retval EFI_NOT_READY     The function does not determine whether this CPU should be
                            BSP or AP. This may occur if hardware init sequence to
                            enable the determination is yet to be done, or the function
                            chooses not to do BSP election and will let SMM CPU driver to
                            use its default BSP election process.
  @retval EFI_DEVICE_ERROR  The function cannot determine whether this CPU should be
                            BSP or AP due to hardware error.

**/
EFI_STATUS
EFIAPI
PlatformSmmBspElection (
  OUT BOOLEAN     *IsBsp
  )
{
  return EFI_NOT_READY;
}

/**
  Get platform page table attribute .

  This function gets page table attribute of platform.

  @param  Address        Input parameter. Obtain the page table entries attribute on this address.
  @param  PageSize       Output parameter. The size of the page.
  @param  NumOfPages     Output parameter. Number of page.
  @param  PageAttribute  Output parameter. Paging Attributes (WB, UC, etc).
  
  @retval EFI_SUCCESS       The platform page table attribute from the address is determined.
  @retval EFI_UNSUPPORTED   The paltform not supoort to get page table attribute from the address.

**/
EFI_STATUS
EFIAPI
GetPlatformPageTableAttribute (
  IN  UINT64                Address,
  IN OUT SMM_PAGE_SIZE_TYPE *PageSize,
  IN OUT UINTN              *NumOfPages,
  IN OUT UINTN              *PageAttribute
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Platform hook called when exiting SMM.

**/
VOID
EFIAPI
PlatformSmmExitProcessing (
  VOID
  )
{
}
