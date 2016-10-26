/** @file
  Public include file for CPU definitions and CPU library functions that 
  apply to CPUs that fit into an LGA775 spocket.
  
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

  Module Name:  CpuOnlyResetLib.h

**/

#ifndef __SOCKET_LGA_775_H__
#define __SOCKET_LGA_775_H__

//
// Processor Family and Model information.
//
#define PENTIUM_FAMILY_ID             0x05
#define QUARK_MODEL_ID              0x09

//
// Definition for CPUID Index
//
#define EFI_CPUID_SIGNATURE                   0x0
#define EFI_CPUID_VERSION_INFO                0x1
#define EFI_CPUID_CACHE_INFO                  0x2
#define EFI_CPUID_SERIAL_NUMBER               0x3
#define EFI_CPUID_CACHE_PARAMS                0x4
#define EFI_CPUID_EXTENDED_TOPOLOGY           0xB
#define EFI_CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID 0x0
#define EFI_CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT     0x1
#define EFI_CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE    0x2

#define EFI_CPUID_EXTENDED_FUNCTION           0x80000000
#define EFI_CPUID_EXTENDED_CPU_SIG            0x80000001
#define EFI_CPUID_BRAND_STRING1               0x80000002
#define EFI_CPUID_BRAND_STRING2               0x80000003
#define EFI_CPUID_BRAND_STRING3               0x80000004
#define EFI_CPUID_ADDRESS_SIZE                0x80000008

//
// Definition for MSR address
//
#define EFI_MSR_IA32_TIME_STAMP_COUNTER       0x10
#define EFI_MSR_IA32_APIC_BASE                0x1B
#define MSR_IA32_MISC_ENABLE                  0x1A0

//
// Definition for Local APIC registers and related values
//
#define LOCAL_APIC_LVT_TIMER                  0x320
#define LOCAL_APIC_TIMER_INIT_COUNT           0x380
#define LOCAL_APIC_TIMER_COUNT                0x390
#define LOCAL_APIC_TIMER_DIVIDE               0x3E0


#define DELIVERY_MODE_FIXED                   0x0
#define DELIVERY_MODE_LOWEST_PRIORITY         0x1
#define DELIVERY_MODE_SMI                     0x2
#define DELIVERY_MODE_REMOTE_READ             0x3
#define DELIVERY_MODE_NMI                     0x4
#define DELIVERY_MODE_INIT                    0x5
#define DELIVERY_MODE_SIPI                    0x6




#define IA32_PG_P                   1u
#define IA32_PG_RW                  (1u << 1)
#define IA32_PG_USR                 (1u << 2)
#define IA32_PG_WT                  (1u << 3)
#define IA32_PG_CD                  (1u << 4)
#define IA32_PG_A                   (1u << 5)
#define IA32_PG_D                   (1u << 6)
#define IA32_PG_PS                  (1u << 7)
#define IA32_PG_G                   (1u << 8)
#define IA32_PG_AVL1                (7u << 9)
#define IA32_PG_PAT_2M              (1u << 12)
#define IA32_PG_PAT_4K              IA32_PG_PS
#define IA32_PG_AVL2                (0x7ffull << 52)

#define IA32_CPUID_SS               0x08000000

#define IA32_MTRRCAP                0xfe

#define SMM_DEFAULT_SMBASE          0x30000
#define SMM_HANDLER_OFFSET          0x8000
#define SMM_PSD_OFFSET              0xfb00
#define SMM_CPU_STATE_OFFSET        0xfc00

#define SMM_CPU_INTERVAL            SIZE_2KB

#pragma pack (1)

typedef struct {
  UINT64                            Signature;              // Offset 0x00
  UINT16                            Reserved1;              // Offset 0x08
  UINT16                            Reserved2;              // Offset 0x0A
  UINT16                            Reserved3;              // Offset 0x0C
  UINT16                            SmmCs;                  // Offset 0x0E
  UINT16                            SmmDs;                  // Offset 0x10
  UINT16                            SmmSs;                  // Offset 0x12
  UINT16                            SmmOtherSegment;        // Offset 0x14
  UINT16                            Reserved4;              // Offset 0x16
  UINT64                            Reserved5;              // Offset 0x18
  UINT64                            Reserved6;              // Offset 0x20
  UINT64                            Reserved7;              // Offset 0x28
  UINT64                            SmmGdtPtr;              // Offset 0x30
  UINT32                            SmmGdtSize;             // Offset 0x38
  UINT32                            Reserved8;              // Offset 0x3C
  UINT64                            Reserved9;              // Offset 0x40
  UINT64                            Reserved10;             // Offset 0x48
  UINT16                            Reserved11;             // Offset 0x50
  UINT16                            Reserved12;             // Offset 0x52
  UINT32                            Reserved13;             // Offset 0x54
  UINT64                            MtrrBaseMaskPtr;        // Offset 0x58
} PROCESSOR_SMM_DESCRIPTOR;

///
///
///
#define SOCKET_LGA_775_SMM_MIN_REV_ID_IOMISC  0x30004
#define SOCKET_LGA_775_SMM_MIN_REV_ID_x64     0x30006

///
/// SMM Save State IOMisc related defines
///
//
// I/O Length BitMap
//
#define  SMM_IO_LENGTH_BYTE             0x01
#define  SMM_IO_LENGTH_WORD             0x02
#define  SMM_IO_LENGTH_DWORD            0x04

//
// I/O Instruction Type BitMap
//
#define  SMM_IO_TYPE_IN_IMMEDIATE       0x9
#define  SMM_IO_TYPE_IN_DX              0x1
#define  SMM_IO_TYPE_OUT_IMMEDIATE      0x8
#define  SMM_IO_TYPE_OUT_DX             0x0
#define  SMM_IO_TYPE_INS                0x3
#define  SMM_IO_TYPE_OUTS               0x2
#define  SMM_IO_TYPE_REP_INS            0x7 
#define  SMM_IO_TYPE_REP_OUTS           0x6

typedef union {
  struct {
    UINT32  SmiFlag:1;
    UINT32  Length:3;
    UINT32  Type:4;
    UINT32  Reserved1:8;
    UINT32  Port:16;
  } Bits;
  UINT32  Uint32;
} SOCKET_LGA_775_SMM_CPU_STATE_IOMISC;

#pragma pack ()

#endif   
