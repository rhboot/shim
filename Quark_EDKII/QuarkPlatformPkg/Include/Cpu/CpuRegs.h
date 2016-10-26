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

  CpuRegs.h

Abstract:

--*/

#ifndef _CPU_REGS_H_
#define _CPU_REGS_H_

#include <CommonIncludes.h>

#define EFI_CPUID_VERSION_INFO                0x1
#define EFI_CPUID_EXTENDED_FUNCTION           0x80000000
#define EFI_CPUID_VIR_PHY_ADDRESS_SIZE        0x80000008

#define EFI_IA32_MTRR_CAP                     0x000000FE
#define   B_EFI_IA32_MTRR_VARIABLE_SUPPORT    0xFF

#define EFI_CACHE_VARIABLE_MTRR_BASE          0x00000200

typedef struct {
  UINT32  RegEax;
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;
} EFI_CPUID_REGISTER;

#endif
