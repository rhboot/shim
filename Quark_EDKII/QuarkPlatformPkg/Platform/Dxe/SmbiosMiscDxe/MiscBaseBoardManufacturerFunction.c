/** @file
  Base Board Information boot time changes.
  Misc. subclass type 4.
  SMBIOS type 2.

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


#include "CommonHeader.h"
#include "SmbiosMisc.h"

/**
  This function makes boot time changes to the contents of the
  MiscBaseBoardManufacturer (Type 2).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.  

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscBaseBoardManufacturer)
{
  CHAR8                           *OptionalStrStart;
  UINTN                           ManuStrLen;
  UINTN                           ProductStrLen;
  UINTN                           VerStrLen;
  UINTN                           AssertTagStrLen;
  UINTN                           SerialNumStrLen;
  UINTN                           ChassisStrLen;
  EFI_STATUS                      Status;
  EFI_STRING                      Manufacturer;
  EFI_STRING                      Product;
  EFI_STRING                      Version;
  EFI_STRING                      SerialNumber;
  EFI_STRING                      AssertTag;
  EFI_STRING                      Chassis;
  STRING_REF                      TokenToGet;
  STRING_REF                        TokenToUpdate;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  SMBIOS_TABLE_TYPE2              *SmbiosRecord;
  EFI_MISC_BASE_BOARD_MANUFACTURER   *ForType2InputData;
  EFI_PLATFORM_TYPE_PROTOCOL      *PlatformType;

  ForType2InputData = (EFI_MISC_BASE_BOARD_MANUFACTURER *)RecordData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_MANUFACTURER);
  Manufacturer = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  ManuStrLen = StrLen(Manufacturer);
  if (ManuStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->LocateProtocol (&gEfiPlatformTypeProtocolGuid, NULL, (VOID **) &PlatformType);
  ASSERT_EFI_ERROR (Status);

  if (StrLen (PlatformType->TypeStringPtr) > 0) {     
    TokenToUpdate = STRING_TOKEN (STR_MISC_BASE_BOARD_PRODUCT_NAME);
    HiiSetString (mHiiHandle, TokenToUpdate, PlatformType->TypeStringPtr, NULL);
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_PRODUCT_NAME);
  Product = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  ProductStrLen = StrLen(PlatformType->TypeStringPtr);
  if (ProductStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }
  StrCpy(PlatformType->TypeStringPtr, Product);

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_VERSION);
  Version = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  VerStrLen = StrLen(Version);
  if (VerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_SERIAL_NUMBER);
  SerialNumber = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  SerialNumStrLen = StrLen(SerialNumber);
  if (SerialNumStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_ASSET_TAG);
  AssertTag = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  AssertTagStrLen = StrLen(AssertTag);
  if (AssertTagStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_CHASSIS_LOCATION);
  Chassis = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  ChassisStrLen = StrLen(Chassis);
  if (ChassisStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }


  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE3) + ManuStrLen + 1 + ProductStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1 + ChassisStrLen +1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE3) + ManuStrLen + 1 + ProductStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1 + ChassisStrLen +1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE2);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;  
  //
  // Manu will be the 1st optional string following the formatted structure.
  // 
  SmbiosRecord->Manufacturer = 1;  
  //
  // ProductName will be the 2st optional string following the formatted structure.
  // 
  SmbiosRecord->ProductName  = 2;  
  //
  // Version will be the 3rd optional string following the formatted structure.
  //
  SmbiosRecord->Version = 3;  
  //
  // SerialNumber will be the 4th optional string following the formatted structure.
  //
  SmbiosRecord->SerialNumber = 4;  
  //
  // AssertTag will be the 5th optional string following the formatted structure.
  //
  SmbiosRecord->AssetTag = 5;  

  //
  // LocationInChassis will be the 6th optional string following the formatted structure.
  //
  SmbiosRecord->LocationInChassis = 6;  
  SmbiosRecord->FeatureFlag = (*(BASE_BOARD_FEATURE_FLAGS*)&(ForType2InputData->BaseBoardFeatureFlags));
  SmbiosRecord->ChassisHandle  = 0;
  SmbiosRecord->BoardType      = (UINT8)ForType2InputData->BaseBoardType;
  SmbiosRecord->NumberOfContainedObjectHandles = 0;
  
  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  //
  // Since we fill NumberOfContainedObjectHandles = 0 for simple, just after this filed to fill string
  //
  //OptionalStrStart -= 2;
  UnicodeStrToAsciiStr(Manufacturer, OptionalStrStart);
  UnicodeStrToAsciiStr(Product, OptionalStrStart + ManuStrLen + 1);
  UnicodeStrToAsciiStr(Version, OptionalStrStart + ManuStrLen + 1 + ProductStrLen + 1);
  UnicodeStrToAsciiStr(SerialNumber, OptionalStrStart + ManuStrLen + 1 + ProductStrLen + 1 + VerStrLen + 1);
  UnicodeStrToAsciiStr(AssertTag, OptionalStrStart + ManuStrLen + 1 + ProductStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1);
  UnicodeStrToAsciiStr(Chassis, OptionalStrStart + ManuStrLen + 1 + ProductStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1);

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
