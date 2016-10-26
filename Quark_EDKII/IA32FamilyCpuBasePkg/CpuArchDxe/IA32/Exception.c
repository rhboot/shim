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

  Exception.c
    
Abstract:

  IA-32 Exception Handler.

**/

#include "Cpu.h"
#include "Exception.h"

//
// Error code flag indicating whether or not an error code will be 
// pushed on the stack if an exception occurs.
//
// 1 means an error code will be pushed, otherwise 0
//
// bit 0 - exception 0
// bit 1 - exception 1
// etc.
//
UINT32 mErrorCodeFlag = 0x00027d00;

//
// Local Table
//
EFI_EXCEPTION_HANDLER mExceptionTable[] = {
  {
    EFI_SW_EC_IA32_DIVIDE_ERROR,
    INTERRUPT_HANDLER_DIVIDE_ZERO
  },
  {
    EFI_SW_EC_IA32_DEBUG,
    INTERRUPT_HANDLER_DEBUG
  },
  {
    EFI_SW_EC_IA32_NMI,
    INTERRUPT_HANDLER_NMI
  },
  {
    EFI_SW_EC_IA32_BREAKPOINT,
    INTERRUPT_HANDLER_BREAKPOINT
  },
  {
    EFI_SW_EC_IA32_OVERFLOW,
    INTERRUPT_HANDLER_OVERFLOW
  },
  {
    EFI_SW_EC_IA32_BOUND,
    INTERRUPT_HANDLER_BOUND
  },
  {
    EFI_SW_EC_IA32_INVALID_OPCODE,
    INTERRUPT_HANDLER_INVALID_OPCODE
  },
  //
  // Interrupt 7, 9, 15 not defined in the debug support protocol. Hence no status codes for them!
  //
  {
    EFI_SW_EC_IA32_INVALID_TSS,
    INTERRUPT_HANDLER_INVALID_TSS
  },
  {
    EFI_SW_EC_IA32_SEG_NOT_PRESENT,
    INTERRUPT_HANDLER_SEGMENT_NOT_PRESENT
  },
  {
    EFI_SW_EC_IA32_STACK_FAULT,
    INTERRUPT_HANDLER_STACK_SEGMENT_FAULT
  },
  {
    EFI_SW_EC_IA32_GP_FAULT,
    INTERRUPT_HANDLER_GP_FAULT
  },
  {
    EFI_SW_EC_IA32_PAGE_FAULT,
    INTERRUPT_HANDLER_PAGE_FAULT
  },
  {
    EFI_SW_EC_IA32_FP_ERROR,
    INTERRUPT_HANDLER_MATH_FAULT
  },
  {
    EFI_SW_EC_IA32_ALIGNMENT_CHECK,
    INTERRUPT_HANDLER_ALIGNMENT_FAULT
  },
  {
    EFI_SW_EC_IA32_MACHINE_CHECK,
    INTERRUPT_HANDLER_MACHINE_CHECK
  },
  {
    EFI_SW_EC_IA32_SIMD,
    INTERRUPT_HANDLER_STREAMING_SIMD
  }
};

UINTN mExceptionNumber = sizeof (mExceptionTable) / sizeof (EFI_EXCEPTION_HANDLER);

CPU_STATUS_CODE_TEMPLATE  mStatusCodeData  =  {
  {
    sizeof (EFI_STATUS_CODE_DATA),
    sizeof (EFI_SYSTEM_CONTEXT_IA32),
    { 0 }
  },
  {
    {0}
  }
};

/**
  Reports StatusCode for Exception

  This function reports status code for exception.

  @param  InterruptType   Interrupt type
  @param  SystemContext   EFI_SYSTEM_CONTEXT

**/
VOID
ReportData (
  IN EFI_EXCEPTION_TYPE   InterruptType, 
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINT32                ErrorMessage;
  UINT32                Index;

  CopyMem (
    &mStatusCodeData.SystemContext.SystemContextIa32,
    SystemContext.SystemContextIa32,
    sizeof (EFI_SYSTEM_CONTEXT_IA32)
    );

  ErrorMessage = EFI_SOFTWARE_DXE_BS_DRIVER;
  for (Index = 0; Index < mExceptionNumber; Index++) {
    if (mExceptionTable[Index].Interrupt == InterruptType) {
      ErrorMessage |= mExceptionTable[Index].ErrorMessage;
      break;
    }
  }

  REPORT_STATUS_CODE_EX (
    (EFI_ERROR_CODE | EFI_ERROR_UNRECOVERED),
    EFI_SOFTWARE_UNSPECIFIED | ErrorMessage,
    0,
    NULL,
    NULL,
    (UINT8 *)&mStatusCodeData + sizeof (EFI_STATUS_CODE_DATA),
    sizeof (EFI_SYSTEM_CONTEXT_IA32)
    ); 
}

/**
  Common exception handler.

  @param InterruptType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
CommonExceptionHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  DEBUG ((
    EFI_D_ERROR,
    "!!!! IA32 Exception Type - %08x !!!!\n",
    InterruptType
    ));
  DEBUG ((
    EFI_D_ERROR,
    "EIP - %08x, CS - %08x, EFLAGS - %08x\n",
    SystemContext.SystemContextIa32->Eip,
    SystemContext.SystemContextIa32->Cs,
    SystemContext.SystemContextIa32->Eflags
    ));
  if ((mErrorCodeFlag & (1 << InterruptType)) != 0) {
    DEBUG ((
      EFI_D_ERROR,
      "ExceptionData - %08x\n",
      SystemContext.SystemContextIa32->ExceptionData
      ));
  }
  DEBUG ((
    EFI_D_ERROR,
    "EAX - %08x, ECX - %08x, EDX - %08x, EBX - %08x\n",
    SystemContext.SystemContextIa32->Eax,
    SystemContext.SystemContextIa32->Ecx,
    SystemContext.SystemContextIa32->Edx,
    SystemContext.SystemContextIa32->Ebx
    ));
  DEBUG ((
    EFI_D_ERROR,
    "ESP - %08x, EBP - %08x, ESI - %08x, EDI - %08x\n",
    SystemContext.SystemContextIa32->Esp,
    SystemContext.SystemContextIa32->Ebp,
    SystemContext.SystemContextIa32->Esi,
    SystemContext.SystemContextIa32->Edi
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DS - %08x, ES - %08x, FS - %08x, GS - %08x, SS - %08x\n",
    SystemContext.SystemContextIa32->Ds,
    SystemContext.SystemContextIa32->Es,
    SystemContext.SystemContextIa32->Fs,
    SystemContext.SystemContextIa32->Gs,
    SystemContext.SystemContextIa32->Ss
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GDTR - %08x %08x, IDTR - %08x %08x\n",
    SystemContext.SystemContextIa32->Gdtr[0],
    SystemContext.SystemContextIa32->Gdtr[1],
    SystemContext.SystemContextIa32->Idtr[0],
    SystemContext.SystemContextIa32->Idtr[1]
    ));
  DEBUG ((
    EFI_D_ERROR,
    "LDTR - %08x, TR - %08x\n",
    SystemContext.SystemContextIa32->Ldtr,
    SystemContext.SystemContextIa32->Tr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR0 - %08x, CR2 - %08x, CR3 - %08x, CR4 - %08x\n",
    SystemContext.SystemContextIa32->Cr0,
    SystemContext.SystemContextIa32->Cr2,
    SystemContext.SystemContextIa32->Cr3,
    SystemContext.SystemContextIa32->Cr4
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR0 - %08x, DR1 - %08x, DR2 - %08x, DR3 - %08x\n",
    SystemContext.SystemContextIa32->Dr0,
    SystemContext.SystemContextIa32->Dr1,
    SystemContext.SystemContextIa32->Dr2,
    SystemContext.SystemContextIa32->Dr3
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR6 - %08x, DR7 - %08x\n",
    SystemContext.SystemContextIa32->Dr6,
    SystemContext.SystemContextIa32->Dr7
    ));
  
  //
  // Report Status Code
  //
  ReportData (InterruptType, SystemContext);
  
  //
  // Enter a dead loop.
  //
  CpuDeadLoop ();

  return ;
}

/**
  Installs the Exception Handler.

  This function installs the Exception Handler.

  @param  ExceptionBegin   The begin number of exception

**/
VOID
InitializeException (
  IN UINT32              ExceptionBegin         
  )
{
  EFI_STATUS                      Status;
  UINT32                          Index;
  BOOLEAN                         OldInterruptState;

  //
  // Disable interrupt
  //
  OldInterruptState = SaveAndDisableInterrupts ();

  for (Index = ExceptionBegin; Index < mExceptionNumber; Index++) {
    //
    // Register sample excception handler
    //
    Status = mCpuArchProtocol.RegisterInterruptHandler (
                                 &mCpuArchProtocol, 
                                 mExceptionTable[Index].Interrupt,
                                 CommonExceptionHandler
                                 );
    ASSERT_EFI_ERROR (Status);
  }
  //
  // Restore interrupt state.
  //
  SetInterruptState (OldInterruptState);
}
