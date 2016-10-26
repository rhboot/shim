/** @file
  Include file for record processor subclass data with Smbios protocol.

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

#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

#include "MpCommon.h"

//
// This is the string tool generated data representing our strings.
//
extern UINT8                    CpuMpDxeStrings[];
extern EFI_SMBIOS_PROTOCOL      *mSmbios;
extern EFI_HII_HANDLE           mStringHandle;
extern UINT32                   mPopulatedSocketCount;

//
// This constant defines the maximum length of the CPU brand string. According to the
// IA manual, the brand string is in EAX through EDX (thus 16 bytes) after executing
// the CPUID instructions with EAX as 80000002, 80000003, 80000004.
//
#define MAXIMUM_CPU_BRAND_STRING_LENGTH 48

typedef struct {
  BOOLEAN           StringValid;
  CHAR16            BrandString[MAXIMUM_CPU_BRAND_STRING_LENGTH + 1];
  EFI_STRING_ID     StringRef;
} CPU_PROCESSOR_VERSION_INFORMATION;

//
// It is defined for SMBIOS_TABLE_TYPE4.Status.
//
typedef struct {
  UINT8 CpuStatus       :3; // Indicates the status of the processor.
  UINT8 Reserved1       :3; // Reserved for future use. Should be set to zero.
  UINT8 SocketPopulated :1; // Indicates if the processor socket is populated or not.
  UINT8 Reserved2       :1; // Reserved for future use. Should be set to zero.
} CPU_PROCESSOR_STATUS_DATA;

//
// It is defined for SMBIOS_TABLE_TYPE4.ProcessorCharacteristics.
//
typedef struct {
  UINT16 Reserved       :1;
  UINT16 Unknown        :1;
  UINT16 Capable64Bit   :1;
  UINT16 Reserved2      :13;
} CPU_PROCESSOR_CHARACTERISTICS_DATA;

/**
  Add Processor Information to Type 4 SMBIOS Record for Socket Populated.

  @param[in]    ProcessorNumber     Processor number of specified processor.
  @param[in]    L1CacheHandle       The handle of the L1 Cache SMBIOS record.
  @param[in]    L2CacheHandle       The handle of the L2 Cache SMBIOS record.
  @param[in]    L3CacheHandle       The handle of the L3 Cache SMBIOS record.

**/
VOID
AddSmbiosProcessorTypeTable (
  IN UINTN              ProcessorNumber,
  IN EFI_SMBIOS_HANDLE  L1CacheHandle,
  IN EFI_SMBIOS_HANDLE  L2CacheHandle,
  IN EFI_SMBIOS_HANDLE  L3CacheHandle
  );

/**
  Register notification functions for Pcds related to Smbios Processor Type.
**/
VOID
SmbiosProcessorTypeTableCallback (
  VOID
  );

/**
  Returns the processor voltage of the processor installed in the system.

  @param    ProcessorNumber     Processor number of specified processor.

  @return   Processor Voltage in mV

**/
UINT16
GetProcessorVoltage (
  IN UINTN  ProcessorNumber
  );

/**
  Returns the procesor version string token installed in the system.

  @param    ProcessorNumber  Processor number of specified processor.
  @param    Version          Pointer to the output processor version.

**/
VOID
GetProcessorVersion (
  IN UINTN                                  ProcessorNumber,
  OUT CPU_PROCESSOR_VERSION_INFORMATION     *Version
  );

/**
  Returns the processor family of the processor installed in the system.

  @param    ProcessorNumber     Processor number of specified processor.

  @return   Processor Family

**/
PROCESSOR_FAMILY_DATA
GetProcessorFamily (
  IN UINTN  ProcessorNumber
  );

/**
  Returns the procesor manufaturer string token installed in the system.

  @param    ProcessorNumber     Processor number of specified processor.

  @return   Processor Manufacturer string token.

**/
EFI_STRING_ID
GetProcessorManufacturer (
  IN UINTN  ProcessorNumber
  );

/**
  Checks if processor is Intel or not.

  @param    ProcessorNumber     Processor number of specified processor.

  @return   TRUE                Intel Processor.
  @return   FALSE               Not Intel Processor.

**/
BOOLEAN
IsIntelProcessor (
  IN UINTN  ProcessorNumber
  );

#endif

