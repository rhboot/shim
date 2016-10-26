/** @file
  IA-32 processor specific header file.

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

#ifndef _SMM_PROFILE_ARCH_H_
#define _SMM_PROFILE_ARCH_H_

#pragma pack (1)

typedef struct _MSR_DS_AREA_STRUCT {
  UINT32  BTSBufferBase;
  UINT32  BTSIndex;
  UINT32  BTSAbsoluteMaximum;
  UINT32  BTSInterruptThreshold;
  UINT32  PEBSBufferBase;
  UINT32  PEBSIndex;
  UINT32  PEBSAbsoluteMaximum;
  UINT32  PEBSInterruptThreshold;
  UINT32  PEBSCounterReset[4];
  UINT32  Reserved;
} MSR_DS_AREA_STRUCT;

typedef struct _BRANCH_TRACE_RECORD {
  UINT32  LastBranchFrom;
  UINT32  LastBranchTo;
  UINT32  Rsvd0 : 4;
  UINT32  BranchPredicted : 1;
  UINT32  Rsvd1 : 27;
} BRANCH_TRACE_RECORD;

typedef struct _PEBS_RECORD {
  UINT32  Eflags;
  UINT32  LinearIP;
  UINT32  Eax;
  UINT32  Ebx;
  UINT32  Ecx;
  UINT32  Edx;
  UINT32  Esi;
  UINT32  Edi;
  UINT32  Ebp;
  UINT32  Esp;
} PEBS_RECORD;

#pragma pack ()

#define PHYSICAL_ADDRESS_MASK       ((1ull << 32) - SIZE_4KB)

/**
  Update page table to map the memory correctly in order to make the instruction 
  which caused page fault execute successfully. And it also save the original page
  table to be restored in single-step exception. 32-bit firmware does not need it.

  @param  PageTable           PageTable Address.
  @param  PFAddress           The memory address which caused page fault exception.
  @param  CpuIndex            The index of the processor.
  @param  ErrorCode           The Error code of exception.
  @param  IsValidPFAddress    The flag indicates if SMM profile data need be added.

**/
VOID
RestorePageTableAbove4G (
  UINT64        *PageTable,
  UINT64        PFAddress,
  UINTN         CpuIndex,
  UINTN         ErrorCode,
  BOOLEAN       *IsValidPFAddress
  );

/**
  Create SMM page table for S3 path.
  
**/
VOID
InitSmmS3Cr3 (
  VOID
  );

/**
  Allocate pages for creating 4KB-page based on 2MB-page when page fault happens.
  
**/
VOID
InitPagesForPFHandler (
  VOID
  );

#endif // _SMM_PROFILE_ARCH_H_
