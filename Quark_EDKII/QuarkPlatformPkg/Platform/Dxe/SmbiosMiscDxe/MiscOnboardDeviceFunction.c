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

  MiscOnboardDeviceFunction.c

Abstract:

  Onboard device information boot time changes.
  SMBIOS type 10.

--*/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


/**
  This function makes boot time changes to the contents of the
  MiscOnboardDevice (Type 10).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.  

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscOnboardDevice)
{
  CHAR8                         *OptionalStrStart;
  UINT8                         StatusAndType;
  UINTN                         DescriptionStrLen;
  EFI_STRING                    DeviceDescription;
  STRING_REF                    TokenToGet;
  EFI_STATUS                    Status;
  EFI_SMBIOS_HANDLE             SmbiosHandle;
  SMBIOS_TABLE_TYPE10           *SmbiosRecord;
  EFI_MISC_ONBOARD_DEVICE       *ForType10InputData;

  ForType10InputData = (EFI_MISC_ONBOARD_DEVICE *)RecordData;
  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TokenToGet = 0;
  switch (ForType10InputData->OnBoardDeviceDescription) {
    case STR_MISC_ONBOARD_DEVICE_VIDEO:
      TokenToGet = STRING_TOKEN (STR_MISC_ONBOARD_DEVICE_VIDEO);      
      break;
    case STR_MISC_ONBOARD_DEVICE_AUDIO:
      TokenToGet = STRING_TOKEN (STR_MISC_ONBOARD_DEVICE_AUDIO);
      break;
  }
  
  DeviceDescription = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  DescriptionStrLen = StrLen(DeviceDescription);
  if (DescriptionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE10) + DescriptionStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE10) + DescriptionStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_ONBOARD_DEVICE_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE10);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;

  //
  // Status & Type: Bit 7 Devicen Status, Bits 6:0 Type of Device
  //
  StatusAndType = (UINT8) ForType10InputData->OnBoardDeviceStatus.DeviceType;
  if (ForType10InputData->OnBoardDeviceStatus.DeviceEnabled != 0) {
    StatusAndType |= 0x80;
  } else {
    StatusAndType &= 0x7F;
  }

  SmbiosRecord->Device[0].DeviceType = StatusAndType;
  SmbiosRecord->Device[0].DescriptionString = 1;
  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(DeviceDescription, OptionalStrStart);

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
