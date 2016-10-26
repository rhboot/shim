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

  MiscChassisManufacturerFunction.c
  
Abstract: 

  Chassis manufacturer information boot time changes.
  SMBIOS type 3.

--*/


#include "CommonHeader.h"

#include "SmbiosMisc.h"

/**
  This function makes boot time changes to the contents of the
  MiscChassisManufacturer (Type 3).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.  

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscChassisManufacturer)
{
  CHAR8                           *OptionalStrStart;
  UINTN                           ManuStrLen;
  UINTN                           VerStrLen;
  UINTN                           AssertTagStrLen;
  UINTN                           SerialNumStrLen;
  EFI_STATUS                      Status;
  CHAR16                          Manufacturer[SMBIOS_STRING_MAX_LENGTH];
  CHAR16                          Version[SMBIOS_STRING_MAX_LENGTH];
  CHAR16                          SerialNumber[SMBIOS_STRING_MAX_LENGTH];
  CHAR16                          AssertTag[SMBIOS_STRING_MAX_LENGTH];  
  EFI_STRING                      ManufacturerPtr;
  EFI_STRING                      VersionPtr;
  EFI_STRING                      SerialNumberPtr;
  EFI_STRING                      AssertTagPtr;
  STRING_REF                      TokenToGet;
  STRING_REF                      TokenToUpdate;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  SMBIOS_TABLE_TYPE3              *SmbiosRecord;
  EFI_MISC_CHASSIS_MANUFACTURER   *ForType3InputData;

  ForType3InputData = (EFI_MISC_CHASSIS_MANUFACTURER *)RecordData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update strings from PCD
  //
  AsciiStrToUnicodeStr (PcdGetPtr(PcdSMBIOSChassisManufacturer), Manufacturer);
  if (StrLen (Manufacturer) > 0) {     
    TokenToUpdate = STRING_TOKEN (STR_MISC_CHASSIS_MANUFACTURER);
    HiiSetString (mHiiHandle, TokenToUpdate, Manufacturer, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_MANUFACTURER);
  ManufacturerPtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  ManuStrLen = StrLen(ManufacturerPtr);
  if (ManuStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  AsciiStrToUnicodeStr (PcdGetPtr(PcdSMBIOSChassisVersion), Version);
  if (StrLen (Version) > 0) {     
    TokenToUpdate = STRING_TOKEN (STR_MISC_CHASSIS_VERSION);
    HiiSetString (mHiiHandle, TokenToUpdate, Version, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_VERSION);
  VersionPtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  VerStrLen = StrLen(VersionPtr);
  if (VerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }
  
  AsciiStrToUnicodeStr (PcdGetPtr(PcdSMBIOSChassisSerialNumber), SerialNumber);
  if (StrLen (SerialNumber) > 0) {     
    TokenToUpdate = STRING_TOKEN (STR_MISC_CHASSIS_SERIAL_NUMBER);
    HiiSetString (mHiiHandle, TokenToUpdate, SerialNumber, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_SERIAL_NUMBER);
  SerialNumberPtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  SerialNumStrLen = StrLen(SerialNumberPtr);
  if (SerialNumStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  AsciiStrToUnicodeStr (PcdGetPtr(PcdSMBIOSChassisAssetTag), AssertTag);
  if (StrLen (AssertTag) > 0) {     
    TokenToUpdate = STRING_TOKEN (STR_MISC_CHASSIS_ASSET_TAG);
    HiiSetString (mHiiHandle, TokenToUpdate, AssertTag, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_ASSET_TAG);
  AssertTagPtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  AssertTagStrLen = StrLen(AssertTagPtr);
  if (AssertTagStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE3) + ManuStrLen + 1  + VerStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE3) + ManuStrLen + 1  + VerStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE3);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;  
  //
  // Manu will be the 1st optional string following the formatted structure.
  // 
  SmbiosRecord->Manufacturer = 1;  
  SmbiosRecord->Type = PcdGet8 (PcdSMBIOSChassisType);
  //
  // Version will be the 2nd optional string following the formatted structure.
  //
  SmbiosRecord->Version = 2;  
  //
  // SerialNumber will be the 3rd optional string following the formatted structure.
  //
  SmbiosRecord->SerialNumber = 3;  
  //
  // AssertTag will be the 4th optional string following the formatted structure.
  //
  SmbiosRecord->AssetTag = 4;  
  SmbiosRecord->BootupState = PcdGet8 (PcdSMBIOSChassisBootupState);
  SmbiosRecord->PowerSupplyState = PcdGet8 (PcdSMBIOSChassisPowerSupplyState);
  SmbiosRecord->ThermalState = (UINT8)ForType3InputData->ChassisThermalState;
  SmbiosRecord->SecurityStatus = PcdGet8 (PcdSMBIOSChassisSecurityState);
  *(UINT32 *)&SmbiosRecord->OemDefined = PcdGet32 (PcdSMBIOSChassisOemDefined);
  SmbiosRecord->Height = PcdGet8 (PcdSMBIOSChassisHeight);                
  SmbiosRecord->NumberofPowerCords = PcdGet8 (PcdSMBIOSChassisNumberPowerCords);
  SmbiosRecord->ContainedElementCount = PcdGet8 (PcdSMBIOSChassisElementCount);
  SmbiosRecord->ContainedElementRecordLength = PcdGet8 (PcdSMBIOSChassisElementRecordLength);

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(ManufacturerPtr, OptionalStrStart);
  UnicodeStrToAsciiStr(VersionPtr, OptionalStrStart + ManuStrLen + 1);
  UnicodeStrToAsciiStr(SerialNumberPtr, OptionalStrStart + ManuStrLen + 1 + VerStrLen + 1);
  UnicodeStrToAsciiStr(AssertTagPtr, OptionalStrStart + ManuStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1);

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios-> Add(
                      Smbios, 
                      NULL,
                      &SmbiosHandle, 
                      (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord
                      );

  FreePool(SmbiosRecord);
  return Status;
}
