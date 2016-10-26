/** @file
Implementation of CPU driver for PEI phase.

This PEIM is to expose the Cache Ppi and BIST hob build notification

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
  
Module Name: CpuPeim.c

**/

#include "CpuPei.h"
#include "Bist.h"

extern PEI_CACHE_PPI                mCachePpi;
//
// Ppis to be installed
//
EFI_PEI_PPI_DESCRIPTOR           mPpiList[] = { 
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gPeiCachePpiGuid,
    &mCachePpi
  }
};

//
// Notification to build BIST information
//
STATIC EFI_PEI_NOTIFY_DESCRIPTOR        mNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMasterBootModePpiGuid,
    BuildBistHob
  }
};

/**
  Initialize SSE support.
**/
VOID
InitXMM (
  VOID
  )
{

  UINT32  RegEdx;

  AsmCpuid (EFI_CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx);

  //
  //Check whether SSE2 is supported
  //
  if ((RegEdx & BIT26) == 0) {
    AsmWriteCr0 (AsmReadCr0 () | BIT1);
    AsmWriteCr4 (AsmReadCr4 () | BIT9 | BIT10);
  }
}


/**
  The Entry point of the CPU PEIM

  This function is the Entry point of the CPU PEIM which will install the CachePpi and 
  BuildBISTHob notifier. And also the function will deal with the relocation to memory when 
  permanent memory is ready
 
  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services. 
                          
  @retval EFI_SUCCESS   CachePpi and BIST hob build notification is installed
                        successfully.

**/
EFI_STATUS
EFIAPI
CpuPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
	EFI_STATUS  Status;

  //
  // Report Status Code to indicate the start of CPU PEIM
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_COMPUTING_UNIT_HOST_PROCESSOR + EFI_CU_HP_PC_POWER_ON_INIT
    );

  //
  // Install PPIs
  //
  Status = PeiServicesInstallPpi(&mPpiList[0]);
  ASSERT_EFI_ERROR (Status);

  //
  // Register for PPI Notifications
  //
  Status = PeiServicesNotifyPpi (&mNotifyList[0]);
  ASSERT_EFI_ERROR (Status);

  //
  // Report Status Code to indicate the start of CPU PEI initialization
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_COMPUTING_UNIT_HOST_PROCESSOR + EFI_CU_PC_INIT_BEGIN
    );
   
  InitXMM ();
  
  return Status;
}
