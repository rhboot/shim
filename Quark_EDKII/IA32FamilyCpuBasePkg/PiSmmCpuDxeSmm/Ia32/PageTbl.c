/** @file

Page table manipulation functions for IA-32 processors

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

#include "PiSmmCpuDxeSmm.h"

SPIN_LOCK                           mPFLock;

/**
  Create PageTable for SMM use.

  @return     PageTable Address    

**/
UINT32
SmmInitPageTable (
  VOID
  )
{
  //
  // Initialize spin lock
  //
  InitializeSpinLock (&mPFLock);

 //
 // Register Smm Page Fault Handler
 //
  SmmRegisterExceptionHandler (&mSmmCpuService, EXCEPT_IA32_PAGE_FAULT, SmiPFHandler);


  return Gen4GPageTable (0);
}

/**
  Page Fault handler for SMM use.

**/
VOID
SmiDefaultPFHandler (
  VOID
  )
{
  CpuDeadLoop ();
}

/**
  ThePage Fault handler wrapper for SMM use.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.
**/
VOID
EFIAPI
SmiPFHandler (
    IN EFI_EXCEPTION_TYPE   InterruptType,
    IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINTN             PFAddress;

  ASSERT (InterruptType == EXCEPT_IA32_PAGE_FAULT);

  AcquireSpinLock (&mPFLock);
  
  PFAddress = AsmReadCr2 ();

  if ((FeaturePcdGet (PcdCpuSmmStackGuard)) && 
      (PFAddress >= mCpuHotPlugData.SmrrBase) && 
      (PFAddress < (mCpuHotPlugData.SmrrBase + mCpuHotPlugData.SmrrSize))) {
    DEBUG ((EFI_D_ERROR, "SMM stack overflow!\n"));
    CpuDeadLoop ();
  }
  
  if (FeaturePcdGet (PcdCpuSmmProfileEnable)) {
    SmmProfilePFHandler (
      SystemContext.SystemContextIa32->Eip,
      SystemContext.SystemContextIa32->ExceptionData
      );
  } else {
    SmiDefaultPFHandler ();
  }  
  
  ReleaseSpinLock (&mPFLock);
}
