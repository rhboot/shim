/** @file
  Semaphore mechanism to indicate to the BSP that an AP has exited SMM
  after SMBASE relocation.

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

UINTN   mSmmRelocationOriginalAddress;
BOOLEAN *mRebasedFlag;

/**
  Hook return address of SMM Save State so that semaphore code
  can be executed immediately after AP exits SMM to indicate to
  the BSP that an AP has exited SMM after SMBASE relocation.

  @param CpuIndex  The processor index.
**/
VOID
SemaphoreHook (
  IN UINTN  CpuIndex
  )
{
  EFI_SMM_CPU_STATE  *CpuState;

  mRebasedFlag = (BOOLEAN *) &mRebased[CpuIndex];

  CpuState = (EFI_SMM_CPU_STATE *)(UINTN)(SMM_DEFAULT_SMBASE + SMM_CPU_STATE_OFFSET);

  //
  // The offset of EIP/RIP is different depending on the SMMRevId
  //
  if (CpuState->x86.SMMRevId < SOCKET_LGA_775_SMM_MIN_REV_ID_x64) {
    mSmmRelocationOriginalAddress = (UINTN) CpuState->x86._EIP;
    CpuState->x86._EIP            = (UINT32) (UINTN) &SmmRelocationSemaphoreComplete;
  } else {
    mSmmRelocationOriginalAddress = (UINTN) CpuState->x64._RIP;
    CpuState->x64._RIP            = (UINT64) (UINTN) &SmmRelocationSemaphoreComplete;
  }

  if (CpuState->x86.AutoHALTRestart & BIT0) {
    //
    // Clear the auto HALT restart flag so the RSM instruction returns 
    //   program control to the instruction following the HLT instruction,
    //   actually returns to SmmRelocationSemaphoreComplete
    //
    CpuState->x86.AutoHALTRestart &= ~BIT0;
  }
}
          
