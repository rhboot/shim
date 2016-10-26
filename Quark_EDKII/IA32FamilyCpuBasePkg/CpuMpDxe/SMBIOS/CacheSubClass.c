/** @file
  Code to record cache subclass data with Smbios protocol.

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

#include "Processor.h"
#include "Cache.h"
#include <Library/PrintLib.h>

CPU_CACHE_CONVERTER mCacheConverter[] = {
  {
    1,
    0x06,
    8,
    CacheAssociativity4Way,
    CacheTypeInstruction
  },
  {
    1,
    0x08,
    16,
    CacheAssociativity4Way,
    CacheTypeInstruction
  },
  {
    1,
    0x0A,
    8,
    CacheAssociativity2Way,
    CacheTypeData
  },
  {
    1,
    0x0C,
    16,
    CacheAssociativity4Way,
    CacheTypeData
  },
  {
    3,
    0x22,
    512,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    3,
    0x23,
    1024,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    3,
    0x25,
    2048,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    3,
    0x29,
    4096,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    1,
    0x2C,
    32,
    CacheAssociativity8Way,
    CacheTypeData
  },
  {
    1,
    0x30,
    32,
    CacheAssociativity8Way,
    CacheTypeInstruction
  },
  {
    2,
    0x39,
    128,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    2,
    0x3B,
    128,
    CacheAssociativity2Way,
    CacheTypeUnified
  },
  {
    2,
    0x3C,
    256,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    2,
    0x41,
    128,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    2,
    0x42,
    256,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    2,
    0x43,
    512,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    2,
    0x44,
    1024,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    2,
    0x45,
    2048,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    2,
    0x49,
    4096,
    CacheAssociativity16Way,
    CacheTypeUnified
  },
  {
    1,
    0x60,
    16,
    CacheAssociativity8Way,
    CacheTypeData
  },
  {
    1,
    0x66,
    8,
    CacheAssociativity4Way,
    CacheTypeData
  },
  {
    1,
    0x67,
    16,
    CacheAssociativity4Way,
    CacheTypeData
  },
  {
    1,
    0x68,
    32,
    CacheAssociativity4Way,
    CacheTypeData
  },
  {
    2,
    0x78,
    1024,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    2,
    0x79,
    128,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    2,
    0x7A,
    256,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    2,
    0x7B,
    512,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    2,
    0x7C,
    1024,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    2,
    0x7D,
    2048,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    2,
    0x7F,
    512,
    CacheAssociativity2Way,
    CacheTypeUnified
  },
  {
    2,
    0x82,
    256,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    2,
    0x83,
    512,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    2,
    0x84,
    1024,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    2,
    0x85,
    2048,
    CacheAssociativity8Way,
    CacheTypeUnified
  },
  {
    2,
    0x86,
    512,
    CacheAssociativity4Way,
    CacheTypeUnified
  },
  {
    2,
    0x87,
    1024,
    CacheAssociativity8Way,
    CacheTypeUnified
  }
};

/**
  Get cache data from CPUID EAX = 2.
  
  @param[in]    ProcessorNumber     Processor number of specified processor.
  @param[out]   CacheData           Pointer to the cache data gotten from CPUID EAX = 2.

**/
VOID
GetCacheDataFromCpuid2 (
  IN UINTN              ProcessorNumber,
  OUT CPU_CACHE_DATA    *CacheData
  )
{
  UINT8                     CacheLevel;
  EFI_CPUID_REGISTER        *CacheInformation;
  UINTN                     CacheDescriptorNum;
  UINT32                    RegPointer[4];
  UINT8                     RegIndex;
  UINT32                    RegValue;
  UINT8                     ByteIndex;
  UINT8                     Descriptor;
  UINTN                     DescriptorIndex;

  DEBUG ((EFI_D_INFO, "Get cache data from CPUID EAX = 2\n"));

  CacheDescriptorNum = (UINTN) (sizeof (mCacheConverter) / sizeof (mCacheConverter[0]));
  CacheInformation = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_CACHE_INFO);
  ASSERT (CacheInformation != NULL);

  CopyMem (RegPointer, CacheInformation, sizeof (EFI_CPUID_REGISTER));
  RegPointer[0] &= 0xFFFFFF00;

  for (RegIndex = 0; RegIndex < 4; RegIndex++) {
    RegValue = RegPointer[RegIndex];
    //
    // The most significant bit (bit 31) of each register indicates whether the register
    // contains valid information (set to 0) or is reserved (set to 1).
    //
    if ((RegValue & BIT31) != 0) {
      continue;
    }

    for (ByteIndex = 0; ByteIndex < 4; ByteIndex++) {
      Descriptor = (UINT8) ((RegValue >> (ByteIndex * 8)) & 0xFF);
      for (DescriptorIndex = 0; DescriptorIndex < CacheDescriptorNum; DescriptorIndex++) {
        if (mCacheConverter[DescriptorIndex].CacheDescriptor == Descriptor) {
          CacheLevel = mCacheConverter[DescriptorIndex].CacheLevel; // 1 based
          ASSERT (CacheLevel >= 1 && CacheLevel <= CPU_CACHE_LMAX);
          CacheData[CacheLevel - 1].CacheSizeinKB = (UINT16) (CacheData[CacheLevel - 1].CacheSizeinKB + mCacheConverter[DescriptorIndex].CacheSizeinKB);
          CacheData[CacheLevel - 1].SystemCacheType = mCacheConverter[DescriptorIndex].SystemCacheType;
          CacheData[CacheLevel - 1].Associativity = mCacheConverter[DescriptorIndex].Associativity;
        }
      }   
    }
  }
}

/**
  Get cache data from CPUID EAX = 4.

  CPUID EAX = 4 is Deterministic Cache Parameters Leaf.
  
  @param[in]    ProcessorNumber     Processor number of specified processor.
  @param[out]   CacheData           Pointer to the cache data gotten from CPUID EAX = 4.

**/
VOID
GetCacheDataFromCpuid4 (
  IN UINTN              ProcessorNumber,
  OUT CPU_CACHE_DATA    *CacheData
  )
{
  EFI_CPUID_REGISTER            *CpuidRegisters;
  UINT8                         Index;
  UINT8                         NumberOfDeterministicCacheParameters;
  UINT32                        Ways;
  UINT32                        Partitions;
  UINT32                        LineSize;
  UINT32                        Sets;
  CACHE_TYPE_DATA               SystemCacheType;
  CACHE_ASSOCIATIVITY_DATA      Associativity;
  UINT8                         CacheLevel;

  DEBUG ((EFI_D_INFO, "Get cache data from CPUID EAX = 4\n"));

  NumberOfDeterministicCacheParameters = (UINT8) GetNumberOfCpuidLeafs (ProcessorNumber, DeterministicCacheParametersCpuidLeafs);

  for (Index = 0; Index < NumberOfDeterministicCacheParameters; Index++) {
    CpuidRegisters = GetDeterministicCacheParametersCpuidLeaf (ProcessorNumber, Index);

    if ((CpuidRegisters->RegEax & CPU_CACHE_TYPE_MASK) == 0) {
      //break;
      continue;
    }

    switch (CpuidRegisters->RegEax & CPU_CACHE_TYPE_MASK) {
      case 1:
        SystemCacheType = CacheTypeData;
        break;
      case 2:
        SystemCacheType = CacheTypeInstruction;
        break;
      case 3:
        SystemCacheType = CacheTypeUnified;
        break;
      default:
        SystemCacheType = CacheTypeUnknown;
    }

    Ways = ((CpuidRegisters->RegEbx & CPU_CACHE_WAYS_MASK) >> CPU_CACHE_WAYS_SHIFT) + 1;
    Partitions = ((CpuidRegisters->RegEbx & CPU_CACHE_PARTITIONS_MASK) >> CPU_CACHE_PARTITIONS_SHIFT) + 1;
    LineSize = (CpuidRegisters->RegEbx & CPU_CACHE_LINESIZE_MASK) + 1;
    Sets = CpuidRegisters->RegEcx + 1;

    switch (Ways) {
      case 2:
        Associativity = CacheAssociativity2Way;
        break;
      case 4:
        Associativity = CacheAssociativity4Way;
        break;
      case 8:
        Associativity = CacheAssociativity8Way;
        break;
      case 12:
        Associativity = CacheAssociativity12Way;
        break;
      case 16:
        Associativity = CacheAssociativity16Way;
        break;
      case 24:
        Associativity = CacheAssociativity24Way;
        break;
      case 32:
        Associativity = CacheAssociativity32Way;
        break;
      case 48:
        Associativity = CacheAssociativity48Way;
        break;
      case 64:
        Associativity = CacheAssociativity64Way;
        break;
      default:
        Associativity = CacheAssociativityFully;
        break;
    }

    CacheLevel = (UINT8) ((CpuidRegisters->RegEax & CPU_CACHE_LEVEL_MASK) >> CPU_CACHE_LEVEL_SHIFT); // 1 based
    ASSERT (CacheLevel >= 1 && CacheLevel <= CPU_CACHE_LMAX);
    CacheData[CacheLevel - 1].CacheSizeinKB = (UINT16) (CacheData[CacheLevel - 1].CacheSizeinKB + (Ways * Partitions * LineSize * Sets) / 1024);
    CacheData[CacheLevel - 1].SystemCacheType = SystemCacheType;
    CacheData[CacheLevel - 1].Associativity = Associativity;
  }

}

/**
  Add Type 7 SMBIOS Record for Cache Information.
  
  @param[in]    ProcessorNumber     Processor number of specified processor.
  @param[out]   L1CacheHandle       Pointer to the handle of the L1 Cache SMBIOS record.
  @param[out]   L2CacheHandle       Pointer to the handle of the L2 Cache SMBIOS record.
  @param[out]   L3CacheHandle       Pointer to the handle of the L3 Cache SMBIOS record.

**/
VOID
AddSmbiosCacheTypeTable (
  IN UINTN              ProcessorNumber,
  OUT EFI_SMBIOS_HANDLE *L1CacheHandle,
  OUT EFI_SMBIOS_HANDLE *L2CacheHandle,
  OUT EFI_SMBIOS_HANDLE *L3CacheHandle
  )
{
  EFI_STATUS                    Status;
  SMBIOS_TABLE_TYPE7            *SmbiosRecord;
  EFI_SMBIOS_HANDLE             SmbiosHandle;
  UINT8                         CacheLevel;
  CPU_CACHE_DATA                CacheData[CPU_CACHE_LMAX];
  CHAR8                         *OptionalStrStart;
  UINTN                         StringBufferSize;
  UINTN                         CacheSocketStrLen;
  EFI_STRING                    CacheSocketStr;
  CACHE_SRAM_TYPE_DATA          CacheSramType;
  CPU_CACHE_CONFIGURATION_DATA  CacheConfig;

  ZeroMem (CacheData, CPU_CACHE_LMAX * sizeof (CPU_CACHE_DATA));

  //
  // Check whether the CPU supports CPUID EAX = 4, if yes, get cache data from CPUID EAX = 4,
  // or no, get cache data from CPUID EAX = 2 to be compatible with the earlier CPU.
  //
  if (GetNumberOfCpuidLeafs (ProcessorNumber, BasicCpuidLeaf) > 4 ) {
    GetCacheDataFromCpuid4 (ProcessorNumber, CacheData);
  } else {
    GetCacheDataFromCpuid2 (ProcessorNumber, CacheData);
  }

  //
  // Now cache data has been ready.
  //
  for (CacheLevel = 0; CacheLevel < CPU_CACHE_LMAX; CacheLevel++) {
    //
    // NO smbios record for zero-sized cache.
    //
    if (CacheData[CacheLevel].CacheSizeinKB == 0) {
      continue;
    }

    DEBUG ((
      EFI_D_INFO,
      "CacheData: CacheLevel = 0x%x CacheSizeinKB = 0x%xKB SystemCacheType = 0x%x Associativity = 0x%x\n",
      CacheLevel + 1,
      CacheData[CacheLevel].CacheSizeinKB,
      CacheData[CacheLevel].SystemCacheType,
      CacheData[CacheLevel].Associativity
      ));


    StringBufferSize = sizeof (CHAR16) * SMBIOS_STRING_MAX_LENGTH;
    CacheSocketStr = AllocateZeroPool (StringBufferSize);
    ASSERT (CacheSocketStr != NULL);
    CacheSocketStrLen = UnicodeSPrint (CacheSocketStr, StringBufferSize, L"L%x-Cache", CacheLevel + 1);
    ASSERT (CacheSocketStrLen <= SMBIOS_STRING_MAX_LENGTH);

    //
    // Report Cache Information to Type 7 SMBIOS Record.
    //

    SmbiosRecord = AllocatePool (sizeof (SMBIOS_TABLE_TYPE7) + CacheSocketStrLen + 1 + 1);
    ASSERT (SmbiosRecord != NULL);
    ZeroMem (SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE7) + CacheSocketStrLen + 1 + 1);

    SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_CACHE_INFORMATION;
    SmbiosRecord->Hdr.Length = (UINT8) sizeof (SMBIOS_TABLE_TYPE7);
    //
    // Make handle chosen by smbios protocol.add automatically.
    //
    SmbiosRecord->Hdr.Handle = 0;
    //
    // Socket will be the 1st optional string following the formatted structure.
    //
    SmbiosRecord->SocketDesignation = 1;

    //
    // Cache Level - 1 through 8, e.g. an L1 cache would use value 000b and an L3 cache would use 010b.
    //
    CacheConfig.Level = CacheLevel;
    CacheConfig.Socketed = 0;           // Not Socketed
    CacheConfig.Reserved2 = 0;
    CacheConfig.Location = 0;           // Internal Cache
    CacheConfig.Enable = 1;             // Cache enabled
    CacheConfig.OperationalMode = 1;    // Write Back
    CacheConfig.Reserved1 = 0;
    CopyMem (&SmbiosRecord->CacheConfiguration, &CacheConfig, 2);
    //
    // Only 1K granularity assumed here.
    //
    SmbiosRecord->MaximumCacheSize = CacheData[CacheLevel].CacheSizeinKB;
    SmbiosRecord->InstalledSize = CacheData[CacheLevel].CacheSizeinKB;

    ZeroMem (&CacheSramType, sizeof (CACHE_SRAM_TYPE_DATA));
    CacheSramType.Synchronous = 1;
    CopyMem (&SmbiosRecord->SupportedSRAMType, &CacheSramType, 2);
    CopyMem (&SmbiosRecord->CurrentSRAMType, &CacheSramType, 2);

    SmbiosRecord->CacheSpeed = 0;
    SmbiosRecord->ErrorCorrectionType = CacheErrorSingleBit;
    SmbiosRecord->SystemCacheType = (UINT8) CacheData[CacheLevel].SystemCacheType;
    SmbiosRecord->Associativity = (UINT8) CacheData[CacheLevel].Associativity;

    OptionalStrStart = (CHAR8 *) (SmbiosRecord + 1);
    UnicodeStrToAsciiStr (CacheSocketStr, OptionalStrStart);

    // 
    // Now we have got the full smbios record, call smbios protocol to add this record.
    //
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status = mSmbios->Add (mSmbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*) SmbiosRecord);

    //
    // Record L1/L2/L3 Cache Smbios Handle, Type 4 SMBIOS Record needs it.
    //
    if (CacheLevel + 1 == CPU_CACHE_L1) {
      *L1CacheHandle = SmbiosHandle;
    } else if (CacheLevel + 1 == CPU_CACHE_L2) {
      *L2CacheHandle = SmbiosHandle;
    } else if (CacheLevel + 1 == CPU_CACHE_L3) {
      *L3CacheHandle  = SmbiosHandle;
    }
    FreePool (SmbiosRecord);
    FreePool (CacheSocketStr);
    ASSERT_EFI_ERROR (Status);
  } 
}

