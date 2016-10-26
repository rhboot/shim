/** @file
  Code to retrieve processor sublcass data.

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

#include "Cpu.h"
#include "Processor.h"

/**
  Returns the procesor version string token installed in the system.

  @param    ProcessorNumber  Processor number of specified processor.
  @param    Version          Pointer to the output processor version.

**/
VOID
GetProcessorVersion (
  IN UINTN                                  ProcessorNumber,
  OUT CPU_PROCESSOR_VERSION_INFORMATION     *Version
  )
{
  CHAR16                BrandIdString[MAXIMUM_CPU_BRAND_STRING_LENGTH + 1];
  EFI_CPUID_REGISTER    *CpuBrandString;
  UINT8                 Index;

  //
  // Create the string using Brand ID String.
  //
  Version->StringValid = FALSE;

  if (IsIntelProcessor (ProcessorNumber)) {
    Version->StringRef = STRING_TOKEN (STR_INTEL_GENUINE_PROCESSOR);

    CpuBrandString = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_BRAND_STRING1);
    ASSERT (CpuBrandString != NULL);

    //
    // Check if Brand ID String is supported or filled up
    //
    if (CpuBrandString->RegEax != 0) {
      AsciiStrToUnicodeStr ((CHAR8 *) CpuBrandString, (CHAR16 *) &BrandIdString[0]);

      CpuBrandString = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_BRAND_STRING2);
      ASSERT (CpuBrandString != NULL);
      AsciiStrToUnicodeStr ((CHAR8 *) CpuBrandString, (CHAR16 *) &BrandIdString[16]);

      CpuBrandString = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_BRAND_STRING3);
      ASSERT (CpuBrandString != NULL);
      AsciiStrToUnicodeStr ((CHAR8 *) CpuBrandString, (CHAR16 *) &BrandIdString[32]);

      //
      // Remove preceeding spaces
      //
      Index = 0;
      while (((Index < MAXIMUM_CPU_BRAND_STRING_LENGTH) && (BrandIdString[Index] == 0x20)) != 0) {
        Index++;
      }

      ASSERT (Index <= MAXIMUM_CPU_BRAND_STRING_LENGTH);
      CopyMem (
        Version->BrandString,
        &BrandIdString[Index],
        (MAXIMUM_CPU_BRAND_STRING_LENGTH - Index) * sizeof (CHAR16)
        );
      Version->BrandString[MAXIMUM_CPU_BRAND_STRING_LENGTH - Index] = 0;
      Version->StringValid = TRUE;
    }
  } else {
    Version->StringRef = STRING_TOKEN (STR_UNKNOWN);
  }
}

/**
  Returns the procesor manufaturer string token installed in the system.

  @param    ProcessorNumber     Processor number of specified processor.

  @return   Processor Manufacturer string token.

**/
EFI_STRING_ID
GetProcessorManufacturer (
  IN UINTN  ProcessorNumber
  )
{
  if (IsIntelProcessor (ProcessorNumber)) {
    return STRING_TOKEN (STR_INTEL_CORPORATION);
  } else {
    return STRING_TOKEN (STR_UNKNOWN);
  }
}

/**
  Checks if processor is Intel or not.

  @param    ProcessorNumber     Processor number of specified processor.

  @return   TRUE                Intel Processor.
  @return   FALSE               Not Intel Processor.

**/
BOOLEAN
IsIntelProcessor (
  IN UINTN  ProcessorNumber
  )
{
  EFI_CPUID_REGISTER  *Reg;

  Reg = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_SIGNATURE);
  ASSERT (Reg != NULL);

  //
  // After CPUID(0), check if EBX contians 'uneG', ECX contains 'letn', and EDX contains 'Ieni'
  //
  if ((Reg->RegEbx != 0x756e6547) || (Reg->RegEcx != 0x6c65746e) || (Reg->RegEdx != 0x49656e69)) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Returns the processor family of the processor installed in the system.

  @param    ProcessorNumber     Processor number of specified processor.

  @return   Processor Family

**/
PROCESSOR_FAMILY_DATA
GetProcessorFamily (
  IN UINTN  ProcessorNumber
  )
{
  UINT32  FamilyId;
  UINT32  ModelId;

  if (IsIntelProcessor (ProcessorNumber)) {

    GetProcessorVersionInfo (ProcessorNumber, &FamilyId, &ModelId, NULL, NULL);

    return ProcessorFamilyPentium;
  }

  return ProcessorFamilyUnknown;
}

/**
  Returns the processor voltage of the processor installed in the system.

  @param    ProcessorNumber     Processor number of specified processor.

  @return   Processor Voltage in mV

**/
UINT16
GetProcessorVoltage (
  IN UINTN  ProcessorNumber
  )
{
  UINT16             VoltageInmV;
  EFI_CPUID_REGISTER *Reg;

  Reg = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_VERSION_INFO);
  ASSERT (Reg != NULL);

  if ((Reg->RegEax >> 8 & 0x3F) == 0xF) {
    VoltageInmV = 3000;
  } else {
    VoltageInmV = 1600;
  }

  return VoltageInmV;
}

