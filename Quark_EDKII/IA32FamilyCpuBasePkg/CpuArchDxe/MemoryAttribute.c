/** @file

  Code for memory attribute setting

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

  Module Name:  MemoryAttribute.c

**/

#include "Cpu.h"

FIXED_MTRR    mFixedMtrrTable[] = {
  {
    MTRR_LIB_IA32_MTRR_FIX64K_00000,
    0,
    0x10000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX16K_80000,
    0x80000,
    0x4000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX16K_A0000,
    0xA0000,
    0x4000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_C0000,
    0xC0000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_C8000,
    0xC8000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_D0000,
    0xD0000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_D8000,
    0xD8000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_E0000,
    0xE0000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_E8000,
    0xE8000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_F0000,
    0xF0000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_F8000,
    0xF8000,
    0x1000
  },
};

/**
  Gets GCD Mem Space type from MTRR Type.

  This function gets GCD Mem Space type from MTRR Type

  @param  MtrrAttributes  MTRR memory type

  @return GCD Mem Space type

**/
UINT64
GetMemorySpaceAttributeFromMtrrType (
  IN UINT8                MtrrAttributes
  )
{
  switch (MtrrAttributes) {
  case MTRR_CACHE_UNCACHEABLE:
    return EFI_MEMORY_UC;
  case MTRR_CACHE_WRITE_COMBINING:
    return EFI_MEMORY_WC;
  case MTRR_CACHE_WRITE_THROUGH:
    return EFI_MEMORY_WT;
  case MTRR_CACHE_WRITE_PROTECTED:
    return EFI_MEMORY_WP;
  case MTRR_CACHE_WRITE_BACK:
    return EFI_MEMORY_WB;
  default:
    return 0;
  }
}

/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

**/
VOID
InitializeMtrrMask (
  VOID
  )
{
  UINT32                              RegEax;
  UINT8                               PhysicalAddressBits;

  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);

  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
  
    PhysicalAddressBits = (UINT8) RegEax;

    mValidMtrrBitsMask    = LShiftU64 (1, PhysicalAddressBits) - 1;
    mValidMtrrAddressMask = mValidMtrrBitsMask & 0xfffffffffffff000ULL;
  } else {
    mValidMtrrBitsMask    = MTRR_LIB_MSR_VALID_MASK;
    mValidMtrrAddressMask = MTRR_LIB_CACHE_VALID_ADDRESS;
  }
}

/**
  Searches memory descriptors covered by given memory range.

  This function searches into the Gcd Memory Space for descriptors (from StartIndex
  to EndIndex) that contains the memory range specified by BaseAddress
  and Length.

  @param  MemorySpaceMap       Gcd Memory Space Map as array.
  @param  NumberOfDescriptors  Number of descriptors in map.
  @param  BaseAddress          BaseAddress for the requested range.
  @param  Length               Length for the requested range.
  @param  StartIndex           Start index into the Gcd Memory Space Map.
  @param  EndIndex             End index into the Gcd Memory Space Map.

  @retval EFI_SUCCESS          Search successfully.
  @retval EFI_NOT_FOUND        The requested descriptors does not exist.

**/
EFI_STATUS
SearchGcdMemorySpaces (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap,
  IN UINTN                               NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN UINT64                              Length,
  OUT UINTN                              *StartIndex,
  OUT UINTN                              *EndIndex
  )
{
  UINTN           Index;

  *StartIndex = 0;
  *EndIndex   = 0;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (BaseAddress >= MemorySpaceMap[Index].BaseAddress &&
        BaseAddress < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) {
      *StartIndex = Index;
    }
    if (BaseAddress + Length - 1 >= MemorySpaceMap[Index].BaseAddress &&
        BaseAddress + Length - 1 < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) {
      *EndIndex = Index;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

/**
  Sets the attributes for a specified range in Gcd Memory Space Map.

  This function sets the attributes for a specified range in Gcd Memory Space Map.

  @param  MemorySpaceMap       Gcd Memory Space Map as array
  @param  NumberOfDescriptors  Number of descriptors in map
  @param  BaseAddress          BaseAddress for the range
  @param  Length               Length for the range
  @param  Attributes           Attributes to set

  @retval EFI_SUCCESS          Memory attributes set successfully
  @retval EFI_NOT_FOUND        The specified range does not exist in Gcd Memory Space

**/
EFI_STATUS
SetGcdMemorySpaceAttributes (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap,
  IN UINTN                               NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN UINT64                              Length,
  IN UINT64                              Attributes
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  UINTN                 StartIndex;
  UINTN                 EndIndex;
  EFI_PHYSICAL_ADDRESS  RegionStart;
  UINT64                RegionLength;

  //
  // Get all memory descriptors covered by the memory range
  //
  Status = SearchGcdMemorySpaces (
             MemorySpaceMap,
             NumberOfDescriptors,
             BaseAddress,
             Length,
             &StartIndex,
             &EndIndex
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Go through all related descriptors and set attributes accordingly
  //
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
      continue;
    }
    //
    // Calculate the start and end address of the overlapping range
    //
    if (BaseAddress >= MemorySpaceMap[Index].BaseAddress) {
      RegionStart = BaseAddress;
    } else {
      RegionStart = MemorySpaceMap[Index].BaseAddress;
    }
    if (BaseAddress + Length - 1 < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) {
      RegionLength = BaseAddress + Length - RegionStart;
    } else {
      RegionLength = MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - RegionStart;
    }
    //
    // Set memory attributes according to MTRR attribute and the original attribute of descriptor
    //
    gDS->SetMemorySpaceAttributes (
           RegionStart,
           RegionLength,
           (MemorySpaceMap[Index].Attributes & ~EFI_MEMORY_CACHETYPE_MASK) | (MemorySpaceMap[Index].Capabilities & Attributes)
           );
  }

  return EFI_SUCCESS;
}

/**
  Refreshes the GCD Memory Space attributes according to MTRRs.
  
  This function refreshes the GCD Memory Space attributes according to MTRRs.

**/
VOID
RefreshGcdMemoryAttributes (
  VOID
  )
{
  EFI_STATUS                          Status;
  UINTN                               Index;
  UINTN                               SubIndex;
  UINT64                              RegValue;
  EFI_PHYSICAL_ADDRESS                BaseAddress;
  UINT64                              Length;
  UINT64                              Attributes;
  UINT64                              CurrentAttributes;
  UINT8                               MtrrType;
  UINTN                               NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap;
  UINT64                              DefaultAttributes;
  VARIABLE_MTRR                       VariableMtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
  MTRR_FIXED_SETTINGS                 MtrrFixedSettings;
  UINTN                               FirmwareVariableMtrrCount;
  UINT8                               DefaultMemoryType;

  FirmwareVariableMtrrCount = GetFirmwareVariableMtrrCount ();

  mIsFlushingGCD = TRUE;
  MemorySpaceMap = NULL;

  //
  // Initialize the valid bits mask and valid address mask for MTRRs
  //
  InitializeMtrrMask ();

  //
  // Get the memory attribute of variable MTRRs
  //
  MtrrGetMemoryAttributeInVariableMtrr (
    mValidMtrrBitsMask,
    mValidMtrrAddressMask,
    VariableMtrr
    );

  //
  // Get the memory space map from GCD
  //
  Status = gDS->GetMemorySpaceMap (
                  &NumberOfDescriptors,
                  &MemorySpaceMap
                  );
  ASSERT_EFI_ERROR (Status);

  DefaultMemoryType = (UINT8) MtrrGetDefaultMemoryType ();
  DefaultAttributes = GetMemorySpaceAttributeFromMtrrType (DefaultMemoryType);

  //
  // Set default attributes to all spaces.
  //
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
      continue;
    }
    gDS->SetMemorySpaceAttributes (
           MemorySpaceMap[Index].BaseAddress,
           MemorySpaceMap[Index].Length,
           (MemorySpaceMap[Index].Attributes & ~EFI_MEMORY_CACHETYPE_MASK) |
           (MemorySpaceMap[Index].Capabilities & DefaultAttributes)
           );
  }
  
  //
  // Go for variable MTRRs with WB attribute
  //
  for (Index = 0; Index < FirmwareVariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid &&
        VariableMtrr[Index].Type == MTRR_CACHE_WRITE_BACK) {
      SetGcdMemorySpaceAttributes (
        MemorySpaceMap,
        NumberOfDescriptors,
        VariableMtrr[Index].BaseAddress,
        VariableMtrr[Index].Length,
        EFI_MEMORY_WB
        );
    }
  }
  //
  // Go for variable MTRRs with the attribute except for WB and UC attributes
  //
  for (Index = 0; Index < FirmwareVariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid &&
        VariableMtrr[Index].Type != MTRR_CACHE_WRITE_BACK &&
        VariableMtrr[Index].Type != MTRR_CACHE_UNCACHEABLE) {
      Attributes = GetMemorySpaceAttributeFromMtrrType ((UINT8) VariableMtrr[Index].Type);
      SetGcdMemorySpaceAttributes (
        MemorySpaceMap,
        NumberOfDescriptors,
        VariableMtrr[Index].BaseAddress,
        VariableMtrr[Index].Length,
        Attributes
        );
    }
  }
  //
  // Go for variable MTRRs with UC attribute
  //
  for (Index = 0; Index < FirmwareVariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid &&
        VariableMtrr[Index].Type == MTRR_CACHE_UNCACHEABLE) {
      SetGcdMemorySpaceAttributes (
        MemorySpaceMap,
        NumberOfDescriptors,
        VariableMtrr[Index].BaseAddress,
        VariableMtrr[Index].Length,
        EFI_MEMORY_UC
        );
    }
  }

  //
  // Go for fixed MTRRs
  //
  Attributes  = 0;
  BaseAddress = 0;
  Length      = 0;
  MtrrGetFixedMtrr (&MtrrFixedSettings);
  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
    RegValue = MtrrFixedSettings.Mtrr[Index];
    //
    // Check for continuous fixed MTRR sections
    //
    for (SubIndex = 0; SubIndex < 8; SubIndex++) {
      MtrrType = (UINT8) RShiftU64 (RegValue, SubIndex * 8);
      CurrentAttributes = GetMemorySpaceAttributeFromMtrrType (MtrrType);
      if (Length == 0) {
        //
        // A new MTRR attribute begins
        //
        Attributes = CurrentAttributes;
      } else {
        //
        // If fixed MTRR attribute changed, then set memory attribute for previous atrribute
        //
        if (CurrentAttributes != Attributes) {
          SetGcdMemorySpaceAttributes (
            MemorySpaceMap,
            NumberOfDescriptors,
            BaseAddress,
            Length,
            Attributes
            );
          BaseAddress = mFixedMtrrTable[Index].BaseAddress + mFixedMtrrTable[Index].Length * SubIndex;
          Length = 0;
          Attributes = CurrentAttributes;
        }
      }
      Length += mFixedMtrrTable[Index].Length;
    }
  }
  //
  // Handle the last fixed MTRR region
  //
  SetGcdMemorySpaceAttributes (
    MemorySpaceMap,
    NumberOfDescriptors,
    BaseAddress,
    Length,
    Attributes
    );

  //
  // Free memory space map allocated by GCD service GetMemorySpaceMap ()
  //
  if (MemorySpaceMap != NULL) {
    FreePool (MemorySpaceMap);
  }

  mIsFlushingGCD = FALSE;
}
