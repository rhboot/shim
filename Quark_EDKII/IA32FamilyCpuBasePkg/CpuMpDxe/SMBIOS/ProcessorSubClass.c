/** @file
  Code to log processor subclass data with Smbios protocol.

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

UINTN mCpuSocketStrNumber = 1;
UINTN mCpuAssetTagStrNumber = 4;

/**
  Add Type 4 SMBIOS Record for Socket Unpopulated.
**/
VOID
AddUnpopulatedSmbiosProcessorTypeTable (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_SMBIOS_HANDLE     SmbiosHandle;
  UINTN                 TotalSize;
  SMBIOS_TABLE_TYPE4    *SmbiosRecord;
  CHAR8                 *OptionalStrStart;
  EFI_STRING_ID         Token;
  EFI_STRING            CpuSocketStr;
  UINTN                 CpuSocketStrLen;

  //
  // Get CPU Socket string, it will be updated when PcdPlatformCpuSocketNames is set.
  //
  Token = STRING_TOKEN (STR_UNKNOWN);
  CpuSocketStr = HiiGetPackageString (&gEfiCallerIdGuid, Token ,NULL);
  ASSERT (CpuSocketStr != NULL);
  CpuSocketStrLen = StrLen (CpuSocketStr);
  ASSERT (CpuSocketStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Report Processor Information to Type 4 SMBIOS Record.
  //

  TotalSize = sizeof (SMBIOS_TABLE_TYPE4) + CpuSocketStrLen + 1 + 1;
  SmbiosRecord = AllocatePool (TotalSize);
  ASSERT (SmbiosRecord != NULL);
  ZeroMem (SmbiosRecord, TotalSize);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
  SmbiosRecord->Hdr.Length = (UINT8) sizeof (SMBIOS_TABLE_TYPE4);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;
  //
  // Socket will be the 1st optional string following the formatted structure.
  //
  SmbiosRecord->Socket = (UINT8) mCpuSocketStrNumber;
  SmbiosRecord->ProcessorType = CentralProcessor;

  //
  // Just indicate CPU Socket Unpopulated.
  // CPU_PROCESSOR_STATUS_DATA.SocketPopulated: 1- CPU Socket populated and 0 - CPU Socket Unpopulated
  //
  SmbiosRecord->Status = 0;

  SmbiosRecord->MaxSpeed = (UINT16) PcdGet32 (PcdPlatformCpuMaxCoreFrequency);
  SmbiosRecord->ProcessorUpgrade = ProcessorUpgradeSocketLGA775;

  SmbiosRecord->L1CacheHandle = 0xFFFF;
  SmbiosRecord->L2CacheHandle = 0xFFFF;
  SmbiosRecord->L3CacheHandle = 0xFFFF;

  OptionalStrStart = (CHAR8 *) (SmbiosRecord + 1);
  UnicodeStrToAsciiStr (CpuSocketStr, OptionalStrStart);
  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = mSmbios->Add (mSmbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
  FreePool (SmbiosRecord);
  FreePool (CpuSocketStr);
  ASSERT_EFI_ERROR (Status);
}

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
  )
{
  EFI_STATUS                    Status;
  EFI_SMBIOS_HANDLE             SmbiosHandle;
  UINTN                         TotalSize;
  EFI_STRING_ID                 Token;
  CHAR8                         *OptionalStrStart;
  EFI_STRING                    CpuManuStr;
  EFI_STRING                    CpuVerStr;
  EFI_STRING                    CpuSocketStr;
  EFI_STRING                    CpuAssetTagStr;
  UINTN                         CpuManuStrLen;
  UINTN                         CpuVerStrLen;
  UINTN                         CpuSocketStrLen;
  UINTN                         CpuAssetTagStrLen;
  SMBIOS_TABLE_TYPE4            *SmbiosRecord;
  EFI_CPUID_REGISTER            *CpuidRegister;
  UINT16                        ProcessorVoltage;
  CPU_PROCESSOR_VERSION_INFORMATION     Version;
  CPU_PROCESSOR_STATUS_DATA     ProcessorStatus;
  CPU_PROCESSOR_CHARACTERISTICS_DATA    ProcessorCharacteristics;
  UINT16                        PackageThreadCount;
  UINT16                        CoreThreadCount;
  UINT8                         CoreCount;

  CoreCount = 0;

  //
  // Get CPU Socket string, it will be updated when PcdPlatformCpuSocketNames is set.
  //
  Token = STRING_TOKEN (STR_UNKNOWN);
  CpuSocketStr = HiiGetPackageString (&gEfiCallerIdGuid, Token ,NULL);
  ASSERT (CpuSocketStr != NULL);
  CpuSocketStrLen = StrLen (CpuSocketStr);
  ASSERT (CpuSocketStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Get CPU Manufacture string.
  //
  Token = GetProcessorManufacturer (ProcessorNumber);
  CpuManuStr = HiiGetPackageString (&gEfiCallerIdGuid, Token, NULL);
  ASSERT (CpuManuStr != NULL);
  CpuManuStrLen = StrLen (CpuManuStr);
  ASSERT (CpuManuStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Get CPU Version string.
  //
  GetProcessorVersion (ProcessorNumber, &Version);
  if (Version.StringValid) {
    Token = HiiSetString (mStringHandle, 0, Version.BrandString, NULL);
    if (Token == 0) {
      Token = Version.StringRef;
    }
  } else {
    Token = Version.StringRef;
  }
  CpuVerStr = HiiGetPackageString (&gEfiCallerIdGuid, Token, NULL);
  ASSERT (CpuVerStr != NULL);
  CpuVerStrLen = StrLen (CpuVerStr);
  ASSERT (CpuVerStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Get CPU Asset Tag string, it will be updated when PcdPlatformCpuAssetTags is set.
  //
  Token = STRING_TOKEN (STR_UNKNOWN);
  CpuAssetTagStr = HiiGetPackageString (&gEfiCallerIdGuid, Token ,NULL);
  ASSERT (CpuAssetTagStr != NULL);
  CpuAssetTagStrLen = StrLen (CpuAssetTagStr);
  ASSERT (CpuAssetTagStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Get CPU core count.
  //
  if (GetNumberOfCpuidLeafs (ProcessorNumber, BasicCpuidLeaf) > EFI_CPUID_CORE_TOPOLOGY) {
    CpuidRegister = GetExtendedTopologyEnumerationCpuidLeafs (ProcessorNumber, 1);
    PackageThreadCount = (UINT16) (CpuidRegister->RegEbx);
    CpuidRegister = GetExtendedTopologyEnumerationCpuidLeafs (ProcessorNumber, 0);
    CoreThreadCount = (UINT16) (CpuidRegister->RegEbx);
    CoreCount = (UINT8) (PackageThreadCount / CoreThreadCount);
  }

  //
  // Report Processor Information to Type 4 SMBIOS Record.
  //

  TotalSize = sizeof (SMBIOS_TABLE_TYPE4) + CpuSocketStrLen + 1 + CpuManuStrLen + 1 + CpuVerStrLen + 1 + CpuAssetTagStrLen + 1 + 1;
  SmbiosRecord = AllocatePool (TotalSize);
  ASSERT (SmbiosRecord != NULL);
  ZeroMem (SmbiosRecord, TotalSize);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
  SmbiosRecord->Hdr.Length = (UINT8) sizeof (SMBIOS_TABLE_TYPE4);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;
  //
  // Socket will be the 1st optional string following the formatted structure.
  //
  SmbiosRecord->Socket = (UINT8) mCpuSocketStrNumber;
  SmbiosRecord->ProcessorType = CentralProcessor;
  SmbiosRecord->ProcessorFamily = (UINT8) GetProcessorFamily (ProcessorNumber); 
  //
  // Manu will be the 2nd optional string following the formatted structure.
  //
  SmbiosRecord->ProcessorManufacture = 2;   

  CpuidRegister = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_VERSION_INFO);
  ASSERT (CpuidRegister != NULL);
  *(UINT32 *) &SmbiosRecord->ProcessorId.Signature = CpuidRegister->RegEax;
  *(UINT32 *) &SmbiosRecord->ProcessorId.FeatureFlags = CpuidRegister->RegEdx,

  //
  // Version will be the 3rd optional string following the formatted structure.
  // 
  SmbiosRecord->ProcessorVersion = 3;   

  ProcessorVoltage = GetProcessorVoltage (ProcessorNumber); // mV unit
  ProcessorVoltage = (UINT16) ((ProcessorVoltage * 10) / 1000);
  *(UINT8 *) &SmbiosRecord->Voltage = (UINT8) ProcessorVoltage;
  SmbiosRecord->Voltage.ProcessorVoltageIndicateLegacy = 1;

  SmbiosRecord->ExternalClock = (UINT16) (GET_CPU_MISC_DATA (ProcessorNumber, IntendedFsbFrequency));
  SmbiosRecord->MaxSpeed = (UINT16) PcdGet32 (PcdPlatformCpuMaxCoreFrequency);
  SmbiosRecord->CurrentSpeed = (UINT16) (GET_CPU_MISC_DATA (ProcessorNumber, IntendedFsbFrequency) * GET_CPU_MISC_DATA (ProcessorNumber, MaxCoreToBusRatio));

  ProcessorStatus.CpuStatus = 1;        // CPU Enabled
  ProcessorStatus.Reserved1 = 0;
  ProcessorStatus.SocketPopulated = 1;  // CPU Socket Populated
  ProcessorStatus.Reserved2 = 0;
  CopyMem (&SmbiosRecord->Status, &ProcessorStatus, 1);

  SmbiosRecord->ProcessorUpgrade = ProcessorUpgradeSocketLGA775;

  SmbiosRecord->L1CacheHandle = L1CacheHandle;
  SmbiosRecord->L2CacheHandle = L2CacheHandle;
  SmbiosRecord->L3CacheHandle = L3CacheHandle;

  //
  // AssetTag will be the 4th optional string following the formatted structure.
  //
  SmbiosRecord->AssetTag = (UINT8) mCpuAssetTagStrNumber;

  SmbiosRecord->CoreCount = CoreCount;

  ProcessorCharacteristics.Reserved = 0;
  ProcessorCharacteristics.Capable64Bit = 1; // 64-bit Capable
  ProcessorCharacteristics.Unknown = 0;
  ProcessorCharacteristics.Reserved2 = 0;
  CopyMem (&SmbiosRecord->ProcessorCharacteristics, &ProcessorCharacteristics, 2);

  OptionalStrStart = (CHAR8 *) (SmbiosRecord + 1);
  UnicodeStrToAsciiStr (CpuSocketStr, OptionalStrStart);
  UnicodeStrToAsciiStr (CpuManuStr, OptionalStrStart + CpuSocketStrLen + 1);
  UnicodeStrToAsciiStr (CpuVerStr, OptionalStrStart + CpuSocketStrLen + 1 + CpuManuStrLen + 1);
  UnicodeStrToAsciiStr (CpuAssetTagStr, OptionalStrStart + CpuSocketStrLen + 1 + CpuManuStrLen + 1 + CpuVerStrLen + 1);
  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = mSmbios->Add (mSmbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);
  FreePool (SmbiosRecord);
  FreePool (CpuSocketStr);
  FreePool (CpuManuStr);
  FreePool (CpuVerStr);
  FreePool (CpuAssetTagStr);
  ASSERT_EFI_ERROR (Status);
}

/**
  Get Type 4 SMBIOS Record table.

  @param[in, out]   SmbiosHandle    On entry, points to the previous handle of the SMBIOS record. On exit, points to the
                                    next SMBIOS record handle. If it is zero on entry, then the first SMBIOS record
                                    handle will be returned. If it returns zero on exit, then there are no more SMBIOS records.
  @param[out]       Record          Returned pointer to record buffer .

**/
VOID
GetSmbiosProcessorTypeTable (
  IN OUT EFI_SMBIOS_HANDLE      *SmbiosHandle,
  OUT EFI_SMBIOS_TABLE_HEADER   *Record OPTIONAL
  )
{
  EFI_STATUS                 Status;
  EFI_SMBIOS_TYPE            RecordType;
  EFI_SMBIOS_TABLE_HEADER    *Buffer;

  RecordType = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
  do {
    Status = mSmbios->GetNext (
                        mSmbios,
                        SmbiosHandle,
                        &RecordType,
                        &Buffer, 
                        NULL
                        );
    if (!EFI_ERROR(Status)) {
      if (Record != NULL) {
        Record = Buffer;
      }
      return;
    }
  } while (!EFI_ERROR(Status));

}

/**
  Notification function when PcdPlatformCpuSocketCount is set.

  @param[in]        CallBackGuid    The PCD token GUID being set.
  @param[in]        CallBackToken   The PCD token number being set.
  @param[in, out]   TokenData       A pointer to the token data being set.
  @param[in]        TokenDataSize   The size, in bytes, of the data being set.

**/
VOID
EFIAPI
CallbackOnPcdPlatformCpuSocketCount (
  IN        CONST GUID      *CallBackGuid, OPTIONAL
  IN        UINTN           CallBackToken,
  IN  OUT   VOID            *TokenData,
  IN        UINTN           TokenDataSize
  )
{
  UINT32 CpuSocketCount;
  UINT32 SocketIndex;

  CpuSocketCount = 0;
  if (CallBackToken == PcdToken (PcdPlatformCpuSocketCount)) {
    if (TokenDataSize == sizeof (UINT32)) {
      CpuSocketCount = ReadUnaligned32 (TokenData);
    }
  }

  DEBUG ((EFI_D_INFO, "Callback: PcdPlatformCpuSocketCount is set\n"));

  if (CpuSocketCount <= mPopulatedSocketCount) {
    return;
  }

  //
  // Add Type 4 SMBIOS Record for Socket Unpopulated.
  //
  for (SocketIndex = mPopulatedSocketCount; SocketIndex < CpuSocketCount; SocketIndex++) {
    AddUnpopulatedSmbiosProcessorTypeTable ();
  }

}

/**
  Notification function when PcdPlatformCpuSocketNames is set.

  @param[in]        CallBackGuid    The PCD token GUID being set.
  @param[in]        CallBackToken   The PCD token number being set.
  @param[in, out]   TokenData       A pointer to the token data being set.
  @param[in]        TokenDataSize   The size, in bytes, of the data being set.

**/
VOID
EFIAPI
CallbackOnPcdPlatformCpuSocketNames (
  IN        CONST GUID      *CallBackGuid, OPTIONAL
  IN        UINTN           CallBackToken,
  IN  OUT   VOID            *TokenData,
  IN        UINTN           TokenDataSize
  )
{
  CHAR16            **CpuSocketNames;
  CHAR8             *CpuSocketStr;
  UINTN             CpuSocketStrLen;
  UINT32            SocketIndex;
  EFI_SMBIOS_HANDLE SmbiosHandle;
  UINT32            CpuSocketCount;

  CpuSocketNames = NULL;
  if (CallBackToken == PcdToken (PcdPlatformCpuSocketNames)) {
    if (TokenDataSize == sizeof (UINT64)) {
      CpuSocketNames = (CHAR16 **) (UINTN) ReadUnaligned64 (TokenData);
    }
  }

  DEBUG ((EFI_D_INFO, "Callback: PcdPlatformCpuSocketNames is set\n"));

  if (CpuSocketNames == NULL) {
    return;
  }

  CpuSocketCount = PcdGet32 (PcdPlatformCpuSocketCount);
  if (CpuSocketCount < mPopulatedSocketCount) {
    CpuSocketCount = mPopulatedSocketCount;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SocketIndex = 0;

  //
  // Update CPU Socket string for Socket Populated.
  //
  do {
    GetSmbiosProcessorTypeTable (&SmbiosHandle, NULL);
    if (SmbiosHandle == SMBIOS_HANDLE_PI_RESERVED) {
      return;
    }
    CpuSocketStrLen = StrLen (CpuSocketNames[SocketIndex]);
    ASSERT (CpuSocketStrLen <= SMBIOS_STRING_MAX_LENGTH);
    CpuSocketStr = AllocatePool (CpuSocketStrLen + 1);
    UnicodeStrToAsciiStr (CpuSocketNames[SocketIndex], CpuSocketStr);
    mSmbios->UpdateString (mSmbios, &SmbiosHandle, &mCpuSocketStrNumber, CpuSocketStr);
    FreePool (CpuSocketStr);
  } while ((++SocketIndex) < CpuSocketCount);

}

/**
  Notification function when PcdPlatformCpuAssetTags is set.

  @param[in]        CallBackGuid    The PCD token GUID being set.
  @param[in]        CallBackToken   The PCD token number being set.
  @param[in, out]   TokenData       A pointer to the token data being set.
  @param[in]        TokenDataSize   The size, in bytes, of the data being set.

**/
VOID
EFIAPI
CallbackOnPcdPlatformCpuAssetTags (
  IN        CONST GUID      *CallBackGuid, OPTIONAL
  IN        UINTN           CallBackToken,
  IN  OUT   VOID            *TokenData,
  IN        UINTN           TokenDataSize
  )
{
  CHAR16            **CpuAssetTags;
  CHAR8             *CpuAssetTagStr;
  UINTN             CpuAssetTagStrLen;
  UINT32            SocketIndex;
  EFI_SMBIOS_HANDLE SmbiosHandle;

  CpuAssetTags = NULL;
  if (CallBackToken == PcdToken (PcdPlatformCpuAssetTags)) {
    if (TokenDataSize == sizeof (UINT64)) {
      CpuAssetTags = (CHAR16 **) (UINTN) ReadUnaligned64 (TokenData);
    }
  }

  DEBUG ((EFI_D_INFO, "Callback: PcdPlatformCpuAssetTags is set\n"));

  if (CpuAssetTags == NULL) {
    return;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SocketIndex = 0;

  //
  // Update CPU Asset Tag string for Socket Populated.
  //
  do {
    GetSmbiosProcessorTypeTable (&SmbiosHandle, NULL);
    if (SmbiosHandle == SMBIOS_HANDLE_PI_RESERVED) {
      return;
    }
    CpuAssetTagStrLen = StrLen (CpuAssetTags[SocketIndex]);
    ASSERT (CpuAssetTagStrLen <= SMBIOS_STRING_MAX_LENGTH);
    CpuAssetTagStr = AllocatePool (CpuAssetTagStrLen + 1);
    UnicodeStrToAsciiStr (CpuAssetTags[SocketIndex], CpuAssetTagStr);
    mSmbios->UpdateString (mSmbios, &SmbiosHandle, &mCpuAssetTagStrNumber, CpuAssetTagStr);
    FreePool (CpuAssetTagStr);
  } while ((++SocketIndex) < mPopulatedSocketCount);

}

/**
  Register notification functions for Pcds related to Smbios Processor Type.
**/
VOID
SmbiosProcessorTypeTableCallback (
  VOID
  )
{
  UINT32 CpuSocketCount;
  UINT64 CpuSocketNames;
  UINT64 CpuAssetTags;

  CpuSocketCount = PcdGet32 (PcdPlatformCpuSocketCount);
  CpuSocketNames = PcdGet64 (PcdPlatformCpuSocketNames);
  CpuAssetTags = PcdGet64 (PcdPlatformCpuAssetTags);

  //
  // PcdPlatformCpuSocketCount, PcdPlatformCpuSocketNames and PcdPlatformCpuAssetTags
  // have default value 0 in *.dec, check whether they have been set with valid value,
  // if yes, process them directly, or no, register notification functions for them. 
  //
  if (CpuSocketCount <= mPopulatedSocketCount) {
    LibPcdCallbackOnSet (NULL, PcdToken (PcdPlatformCpuSocketCount), CallbackOnPcdPlatformCpuSocketCount);
  } else {
    CallbackOnPcdPlatformCpuSocketCount (NULL, PcdToken (PcdPlatformCpuSocketCount), &CpuSocketCount, sizeof (UINT32));
  }
  if (CpuSocketNames == 0) {
    LibPcdCallbackOnSet (NULL, PcdToken (PcdPlatformCpuSocketNames), CallbackOnPcdPlatformCpuSocketNames);
  } else {
    CallbackOnPcdPlatformCpuSocketNames (NULL, PcdToken (PcdPlatformCpuSocketNames), &CpuSocketNames, sizeof (UINT64));
  }
  if (CpuAssetTags == 0) {
    LibPcdCallbackOnSet (NULL, PcdToken (PcdPlatformCpuAssetTags), CallbackOnPcdPlatformCpuAssetTags);
  } else {
    CallbackOnPcdPlatformCpuAssetTags (NULL, PcdToken (PcdPlatformCpuAssetTags), &CpuAssetTags, sizeof (UINT64));
  }
}

