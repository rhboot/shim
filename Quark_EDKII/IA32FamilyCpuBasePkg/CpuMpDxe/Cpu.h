/** @file

  Include file for CPU DXE Module

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

  Module Name:  Cpu.h

**/

#ifndef _CPU_DXE_H_
#define _CPU_DXE_H_

#include "MpCommon.h"

#define PLATFORM_DESKTOP           0
#define PLATFORM_MOBILE            1
#define PLATFORM_SERVER            2

#define EFI_CPUID_CORE_TOPOLOGY    0x0B

//
// The definitions below follow the naming rules.
//   Definitions beginning with "B_" are bits within registers
//   Definitions beginning with "N_" are the bit position
//   Definitions with "_CPUID_" are CPUID bit fields
//   Definitions with "_MSR_" are MSR bit fields
//   Definitions with "N_*_START" are the bit start position
//   Definitions with "N_*_STOP" are the bit stop position
//   Definitions with "B_*_MASK" are the bit mask for the register values
//

//
// Bit definitions for CPUID EAX = 1
//
// ECX
#define N_CPUID_MONITOR_MWAIT_SUPPORT               3
#define N_CPUID_VMX_SUPPORT                         5
#define N_CPUID_SMX_SUPPORT                         6
#define N_CPUID_EIST_SUPPORT                        7
#define N_CPUID_TM2_SUPPORT                         8
#define N_CPUID_DCA_SUPPORT                         18
#define N_CPUID_X2APIC_SUPPORT                      21
#define N_CPUID_AESNI_SUPPORT                       25
// EDX
#define N_CPUID_MCE_SUPPORT                         7
#define N_CPUID_MCA_SUPPORT                         14
#define N_CPUID_TM_SUPPORT                          29
#define N_CPUID_PBE_SUPPORT                         31

//
// Bit definitions for CPUID EAX = 80000001h
//
// EDX
#define N_CPUID_XD_BIT_AVAILABLE                    20

//
// Bit definitions for MSR_IA32_APIC_BASE (ECX = 1Bh)
//
#define N_MSR_BSP_FLAG                              8
#define B_MSR_ENABLE_X2APIC_MODE                    BIT10
#define N_MSR_ENABLE_X2APIC_MODE                    10
#define N_MSR_APIC_GLOBAL_ENABLE                    11

//
// Bit definitions for MSR_IA32_MISC_ENABLE (ECX = 1A0h)
//
#define N_MSR_LIMIT_CPUID_MAXVAL                    22
#define N_MSR_XD_BIT_DISABLE                        34

extern EFI_PHYSICAL_ADDRESS        mStartupVector;

extern BOOLEAN                     mRestoreSettingAfterInit;
extern UINT8                       mPlatformType;
extern ACPI_CPU_DATA               *mAcpiCpuData;

/**
  Collects basic processor data for calling processor.

  This function collects basic processor data for calling processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
CollectBasicProcessorData (
  IN UINTN  ProcessorNumber
  );

/**
  Early MP Initialization.
  
  This function does early MP initialization, including MTRR sync and first time microcode load.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
EarlyMpInit (
  IN UINTN  ProcessorNumber
  );

/**
  Collects data from all logical processors.

  This function collects data from all logical processors

**/
VOID
DataCollectionPhase (
  VOID
  );

/**
  Produces register table according to output of Data Collection phase.
  
  This function produces register table according to output of Data Collection phase.

**/
VOID
AnalysisPhase (
  VOID
  );

/**
  Programs processor registers according to register tables.

  This function programs processor registers according to register tables.

**/
VOID
SettingPhase (
  VOID
  );

/**
  Produces Pre-SMM-Init Register Tables for all processors.

  This function produces Pre-SMM-Init Register Tables for all processors.

**/
VOID
ProducePreSmmInitRegisterTable (
  VOID
  );

/**
  Wakes up APs for the first time to count their number and collect BIST data.

  This function wakes up APs for the first time to count their number and collect BIST data.

**/
VOID
WakeupAPAndCollectBist (
  VOID
  );

/**
  Configures all logical processors with three-phase architecture.
  
  This function configures all logical processors with three-phase architecture.

**/
VOID
ProcessorConfigurationuration (
  VOID
  );

/**
  Select least-feature processor as BSP.
  
  This function selects least-feature processor as BSP.

**/
VOID
SelectLfpAsBsp (
  VOID
  );

/**
  Add SMBIOS Processor Type and Cache Type tables for the CPU.
**/
VOID
AddCpuSmbiosTables (
  VOID
  );

/**
  Prepare ACPI NVS memory below 4G memory for use of S3 resume.
  
  This function allocates ACPI NVS memory below 4G memory for use of S3 resume,
  and saves data into the memory region.

  @param  Context   The Context save the info.
  
**/
VOID
SaveCpuS3Data (
  VOID    *Context
  );

/**
  Programs registers for the calling processor.

  This function programs registers for the calling processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
SetProcessorRegister (
  IN UINTN  ProcessorNumber
  );

/**
  Programs registers before SMM initialization for the calling processor.

  This function programs registers before SMM initialization for the calling processor.

  @param  ProcessorNumber    Handle number of specified logical processor.

**/
VOID
SetPreSmmInitProcessorRegister (
  IN UINTN  ProcessorNumber
  );

/**
  Label of start of AP machine check handler.

  This is just a label of start of AP machine check handler.

**/
VOID
EFIAPI
ApMachineCheckHandler (
  VOID
  );

/**
  Label of end of AP machine check handler.

  This is just a label of end of AP machine check handler.

**/
VOID
EFIAPI
ApMachineCheckHandlerEnd (
  VOID
  );

/**
  This function gets Package ID/Core ID/Thread ID of the processor.

  The algorithm below assumes the target system has symmetry across physical package boundaries
  with respect to the number of logical processors per package, number of cores per package.

  @param  InitialApicId  Initial APIC ID for determing processor topology.
  @param  Location       Pointer to EFI_CPU_PHYSICAL_LOCATION structure.
  @param  ThreadIdBits   Number of bits occupied by Thread ID portion.
  @param  CoreIdBits     Number of bits occupied by Core ID portion.

**/
VOID
ExtractProcessorLocation (
  IN  UINT32                    InitialApicId,
  OUT EFI_CPU_PHYSICAL_LOCATION *Location,
  OUT UINTN                     *ThreadIdBits,
  OUT UINTN                     *CoreIdBits
  );

#endif
