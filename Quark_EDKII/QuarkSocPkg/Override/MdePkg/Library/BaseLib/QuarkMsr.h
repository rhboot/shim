/** @file
  QuarkMsr header function

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
// Structure to describe a fixed MTRR
//
typedef struct {
  UINT32  StandardMsr;
  UINT8   QuarkMsr;
} QUARK_MTRR_CONVERSION_TABLE;

/**
  Read required data from QNC internal message network
**/
UINT32
EFIAPI
QNCPortRead(
  UINT8 Port, 
  UINT8 RegAddress
  );  

/**
  Write prepared data into QNC internal message network.

**/
VOID
EFIAPI
QNCPortWrite (
  UINT8 Port, 
  UINT8 RegAddress, 
  UINT32 WriteValue
  );  

UINT8
EFIAPI
TranslateMsrIndex (
  IN UINT32  Index
  );

/**
  Loads the results of an MSR read operation into processor registers:
  eax (QuarkMsrDataLow)
  edx (QuarkMsrDataHigh)
  ecx (Index)

  @param  Index               The MSR index
  @param  QuarkMsrDataLow   The low DWORD of the MSR read operation
  @param  QuarkMsrDataHigh  The high DWORD of the MSR read operation

  @return 64-bit MSR result

**/
UINT64
EFIAPI
AsmReadMsrReturnResult (
  IN      UINT32                    Index,
  IN      UINT32                    QuarkMsrDataLow,
  IN      UINT32                    QuarkMsrDataHigh
  );

/**
  Performs an MSR read operation

  @param  Index               The MSR index

  @return 64-bit MSR result

**/
UINT64
EFIAPI
AsmReadMsrRealMsrAccess (
  IN      UINT32                    Index
  );

/**
  Performs an MSR write operation

  @param  Index               The MSR index
  @param  QuarkMsrDataLow   The low DWORD of the MSR write operation
  @param  QuarkMsrDataHigh  The high DWORD of the MSR write operation

**/
UINT64
EFIAPI
AsmWriteMsrRealMsrAccess (
  IN      UINT32                    Index,
  IN      UINT32                    QuarkMsrDataLow,
  IN      UINT32                    QuarkMsrDataHigh
  );

#define EFI_MSR_IA32_APIC_BASE                 0x1B
#define EFI_MSR_PENTIUM_SMRR_PHYS_BASE         0xA0
#define EFI_MSR_PENTIUM_SMRR_PHYS_MASK         0xA1
#define MTRR_LIB_IA32_MTRR_PHYSBASE0           0x200
#define MTRR_LIB_IA32_MTRR_PHYSMASK0           0x201
#define MTRR_LIB_IA32_MTRR_PHYSBASE1           0x202
#define MTRR_LIB_IA32_MTRR_PHYSMASK1           0x203
#define MTRR_LIB_IA32_MTRR_PHYSBASE2           0x204
#define MTRR_LIB_IA32_MTRR_PHYSMASK2           0x205
#define MTRR_LIB_IA32_MTRR_PHYSBASE3           0x206
#define MTRR_LIB_IA32_MTRR_PHYSMASK3           0x207
#define MTRR_LIB_IA32_MTRR_PHYSBASE4           0x208
#define MTRR_LIB_IA32_MTRR_PHYSMASK4           0x209
#define MTRR_LIB_IA32_MTRR_PHYSBASE5           0x20A
#define MTRR_LIB_IA32_MTRR_PHYSMASK5           0x20B
#define MTRR_LIB_IA32_MTRR_PHYSBASE6           0x20C
#define MTRR_LIB_IA32_MTRR_PHYSMASK6           0x20D
#define MTRR_LIB_IA32_MTRR_PHYSBASE7           0x20E
#define MTRR_LIB_IA32_MTRR_PHYSMASK7           0x20F



