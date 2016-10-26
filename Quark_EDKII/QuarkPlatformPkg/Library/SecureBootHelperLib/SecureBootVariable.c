/** @file

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

  SecureBootVariable.c

Abstract:

  Secure boot variable services for Secure Boot Helper Protocol.

--*/

#include "CommonHeader.h"

//
// Routines local to this source module.
//

STATIC
UINTN
GetSecureBootCertVarStats (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  OUT UINT32                              *AttrPtr,
  OUT UINTN                               *TotalCertCountPtr,
  OUT UINTN                               *X509CertCountPtr OPTIONAL,
  OUT UINTN                               *Rsa2048CertCountPtr OPTIONAL,
  OUT UINTN                               *OtherCertCountPtr OPTIONAL,
  OUT UINTN                               *Sha256CountPtr OPTIONAL,
  OUT UINTN                               *Sha1CountPtr OPTIONAL
  )
{
  VOID                              *FindContext;
  EFI_SIGNATURE_LIST                *CertList;
  EFI_SIGNATURE_DATA                *CertData;
  FIND_CERT_STATS                   VarStats;

  *AttrPtr = 0;
  *TotalCertCountPtr = 0;
  if (X509CertCountPtr) {
    *X509CertCountPtr = 0;
  }
  if (Rsa2048CertCountPtr) {
    *Rsa2048CertCountPtr = 0;
  }
  if (OtherCertCountPtr) {
    *OtherCertCountPtr = 0;
  }
  if (Sha256CountPtr) {
    *Sha256CountPtr = 0;
  }
  if (Sha1CountPtr) {
    *Sha1CountPtr = 0;
  }

  FindContext = FindFirstCert (
                  This,
                  VariableName,
                  VendorGuid,
                  NULL,
                  &CertList,
                  &CertData,
                  &VarStats
                  );

  if (FindContext == NULL) {
    return 0;
  }
  ASSERT (VarStats.VariableSize > 0);
  *AttrPtr = VarStats.VariableAttributes;

  do {
    *TotalCertCountPtr = *TotalCertCountPtr + 1;
    if (CompareGuid (&gEfiCertSha256Guid, &CertList->SignatureType)) {
      if (Sha256CountPtr) {
        *Sha256CountPtr = *Sha256CountPtr + 1;
      }
    } else if (CompareGuid (&gEfiCertRsa2048Guid, &CertList->SignatureType)) {
      if (Rsa2048CertCountPtr) {
        *Rsa2048CertCountPtr = *Rsa2048CertCountPtr + 1;
      }
    } else if (CompareGuid (&gEfiCertX509Guid, &CertList->SignatureType)) {
      if (X509CertCountPtr) {
        *X509CertCountPtr = *X509CertCountPtr + 1;
      }
    } else if (CompareGuid (&gEfiCertSha1Guid, &CertList->SignatureType)) {
      if (Sha1CountPtr) {
        *Sha1CountPtr = *Sha1CountPtr + 1;
      }
    } else {
      if (OtherCertCountPtr) {
        *OtherCertCountPtr = *OtherCertCountPtr + 1;
      }
    }

    CertList = FindNextCert (
                 FindContext,
                 NULL,
                 CertList,
                 &CertData
                 );
  } while (CertList != NULL);
  FindCertClose (FindContext);
  return VarStats.VariableSize;
}

//
// Routines exported by this source module.
//

/** Check if secure boot enabled.

  @retval  TRUE    Secure Boot enabled.
  @retval  FALSE   Secure Boot disabled.

**/
BOOLEAN
EFIAPI
IsSecureBootEnabledWork (
  VOID
  )
{
  UINT8                             *SecureBootEnable;
  BOOLEAN                           Result;

  GetVariable2 (EFI_SECURE_BOOT_ENABLE_NAME, &gEfiSecureBootEnableDisableGuid, (VOID**)&SecureBootEnable, NULL);
  if (SecureBootEnable == NULL) {
    DEBUG ((EFI_D_INFO, "SecureBootHelper:IsSecureBootEnabled %s not found\n", EFI_SECURE_BOOT_ENABLE_NAME));
    return FALSE;
  }

  Result = (*SecureBootEnable == SECURE_BOOT_ENABLE);
  FreePool (SecureBootEnable);

  return Result;
}

/** Check if secure boot enabled.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  TRUE    Secure Boot enabled.
  @retval  FALSE   Secure Boot disabled.

**/
BOOLEAN
EFIAPI
IsSecureBootEnabled (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;

  ASSERT (This != NULL);
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  ASSERT (PrivateData->Signature == SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE);

  return IsSecureBootEnabledWork ();;
}

/** Check if secure boot mode is in custom mode.

  @retval  TRUE    Secure Boot mode is in custom mode.
  @retval  FALSE   Secure Boot mode is NOT in custom mode.

**/
BOOLEAN
EFIAPI
IsSecureBootInCustomModeWork (
  VOID
  )
{
  UINT8                             *Mode;
  BOOLEAN                           Result;

  GetVariable2 (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid, (VOID**)&Mode, NULL);
  if (Mode == NULL) {
    return FALSE;
  }

  Result = (*Mode == CUSTOM_SECURE_BOOT_MODE);
  FreePool (Mode);
  return Result;
}

/** Check if secure boot mode is in custom mode.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  TRUE    Secure Boot mode is in custom mode.
  @retval  FALSE   Secure Boot mode is NOT in custom mode.

**/
BOOLEAN
EFIAPI
IsSecureBootInCustomMode (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;

  ASSERT (This != NULL);
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  ASSERT (PrivateData->Signature == SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE);

  return IsSecureBootInCustomModeWork ();
}

/** Set Secure Boot Mode to Custom Mode.

  To be called when for SecureBoot in Setup or when the platform
  is operating by a physically present user..

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  EFI_SUCCESS            Set success.
  @retval  EFI_ACCESS_DENIED      Can't set mode if Secure Boot enabled.
  @retval  EFI_INVALID_PARAMETER  This pointer is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @return  Other                  Unexpected error - unable to verify.

**/
EFI_STATUS
EFIAPI
SetupSetSecureBootCustomMode (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;
  UINT8                             Mode;

  //
  // Verify This pointer.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  Mode = CUSTOM_SECURE_BOOT_MODE;
  return gRT->SetVariable (
                EFI_CUSTOM_MODE_NAME,
                &gEfiCustomModeEnableGuid,
                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                sizeof (UINT8),
                &Mode
                );
}

/** Set Secure Boot Mode to default security Mode.

  To be called when for SecureBoot in Setup/Custom mode.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  EFI_SUCCESS            Set success.
  @retval  EFI_ACCESS_DENIED      Can't set mode if Secure Boot enabled.
  @retval  EFI_INVALID_PARAMETER  This pointer is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @return  Other                  Unexpected error - unable to verify.

**/
EFI_STATUS
EFIAPI
SetupSetSecureBootDefaultMode (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;
  UINT8                             Mode;

  //
  // Verify This pointer.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  Mode = STANDARD_SECURE_BOOT_MODE;
  return gRT->SetVariable (
                EFI_CUSTOM_MODE_NAME,
                &gEfiCustomModeEnableGuid,
                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                sizeof (UINT8),
                &Mode
                );
}

/**
  Create time based authenticated variable playload for SecureBoot Setup Mode.

  Also work in SecureBoot Custom Mode.

  @param[in]       This            Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in, out]  DataSize        On input, the size of Data buffer in bytes.
                                   On output, the size of data returned in Data
                                   buffer in bytes.
  @param[in, out]  Data            On input, Pointer to data buffer to be wrapped or 
                                   pointer to NULL to wrap an empty payload.
                                   On output, Pointer to the new payload date buffer allocated from pool,
                                   it's caller's responsibility to free the memory when finish using it. 

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resourses to create time based payload.
  @retval EFI_INVALID_PARAMETER    The parameter is invalid.
  @retval Others                   Unexpected error happens.

**/
EFI_STATUS
EFIAPI
SetupCreateTimeBasedAuthVarPayload (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN OUT UINTN                            *DataSize,
  IN OUT UINT8                            **Data
  )
{
  EFI_STATUS                        Status;
  UINT8                             *NewData;
  UINT8                             *Payload;
  UINTN                             PayloadSize;
  EFI_VARIABLE_AUTHENTICATION_2     *DescriptorData;
  UINTN                             DescriptorSize;
  EFI_TIME                          Time;
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;

  //
  // Verify This pointer.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  Payload     = *Data;
  PayloadSize = *DataSize;

  DescriptorSize    = OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo) + OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  NewData = (UINT8*) AllocateZeroPool (DescriptorSize + PayloadSize);
  if (NewData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Payload != NULL) && (PayloadSize != 0)) {
    CopyMem (NewData + DescriptorSize, Payload, PayloadSize);
  }

  //
  // In Setup mode or Custom mode, the variable does not need to be signed but the
  // parameters to the SetVariable() call still need to be prepared as authenticated
  // variable. So we create EFI_VARIABLE_AUTHENTICATED_2 descriptor without certificate
  // data in it.
  //

  DescriptorData = (EFI_VARIABLE_AUTHENTICATION_2 *) (NewData);

  Status = gRT->GetTime (&Time, NULL);
  if (EFI_ERROR (Status)) {  // Don't fail if no time.
    ZeroMem (&Time, sizeof (EFI_TIME));
    DEBUG ((EFI_D_ERROR, "SecureBootHelper:CreateAuthVarPayload Auth Var With Zero Time\n"));
  } else {
    Time.Pad1       = 0;
    Time.Nanosecond = 0;
    Time.TimeZone   = 0;
    Time.Daylight   = 0;
    Time.Pad2       = 0;
  }
  CopyMem (&DescriptorData->TimeStamp, &Time, sizeof (EFI_TIME));

  DescriptorData->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  DescriptorData->AuthInfo.Hdr.wRevision        = 0x0200;
  DescriptorData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&DescriptorData->AuthInfo.CertType, &gEfiCertPkcs7Guid);

  *DataSize = DescriptorSize + PayloadSize;
  *Data     = NewData;
  return EFI_SUCCESS;
}

/**  Set authenticated variable for SecureBoot in Setup or Custom mode.

  This code sets an authenticated variable in storage blocks
  (Volatile or Non-Volatile).

  @param[in] This            Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in] VariableName    Name of Variable to set.
  @param[in] VendorGuid      Variable vendor GUID.
  @param[in] Attr            Attribute value of the variable found
  @param[in] DataSize        Size of Data found. If size is less than the
                             data, this value contains the required size.
  @param[in] Data            Data pointer.
  @param[in] AppendIfExists  If TRUE then force append of data if var exists.

  @retval    EFI_INVALID_PARAMETER        Invalid parameter.
  @retval    EFI_SUCCESS                  Set successfully.
  @retval    EFI_OUT_OF_RESOURCES         Resource not enough to set variable.
  @retval    EFI_NOT_FOUND                Not found.
  @retval    EFI_WRITE_PROTECTED          Variable is read-only.

**/
EFI_STATUS
EFIAPI
SetupSetTimeBasedAuthVariable (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN UINT32                               Attr,
  IN UINTN                                DataSize,
  IN UINT8                                *Data,
  IN BOOLEAN                              AppendIfExists
  )
{
  EFI_STATUS                         Status;
  IN UINT8                           *DescAndData;
  IN UINTN                           DescAndDataSize;
  UINTN                              VarSize;

  DescAndData = Data;
  DescAndDataSize = DataSize;

  Status = SetupCreateTimeBasedAuthVarPayload (This, &DescAndDataSize, &DescAndData);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SecureBootHelper:CreateAuthVarPayload Failed %r", Status));
  } else {
    //
    // Check if Auth variable has been already existed.
    // If AppendIfExists, use EFI_VARIABLE_APPEND_WRITE attribute to append the
    // new signature data to original variable
    //
    VarSize = 0;

    Status = gRT->GetVariable(
                    VariableName,
                    VendorGuid,
                    NULL,
                    &VarSize,
                    NULL
                    );
    if (Status == EFI_BUFFER_TOO_SMALL || Status == EFI_NOT_FOUND) {
      if (Status == EFI_BUFFER_TOO_SMALL && AppendIfExists) {
        Attr |= EFI_VARIABLE_APPEND_WRITE;
      }
      //
      // Set the variable.
      //
      Status = gRT->SetVariable(
                      VariableName,
                      VendorGuid,
                      Attr,
                      DescAndDataSize,
                      DescAndData
                      );
      DEBUG ((EFI_D_INFO, "SecureBootHelper:SetAuthVar %r '%s':0x%08x:%g\n", Status, VariableName, Attr, VendorGuid));
    }
  }
  if (DescAndData != Data) {
    FreePool (DescAndData);
  }
  return Status;
}

/** Delete entry from secure boot database for SecureBoot in Setup/Custom mode.

  @param[in]       This                Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       VariableName        The variable name of the vendor's signature database.
  @param[in]       VendorGuid          A unique identifier for the vendor.
  @param[in]       SignatureOwner      Signature GUID to delete.

  @retval  EFI_SUCCESS                 Delete cert success.
  @retval  EFI_INVALID_PARAMETER       Invalid parameter.
  @retval  EFI_NOT_FOUND               Can't find the cert item.
  @retval  EFI_OUT_OF_RESOURCES        Could not allocate needed resources.

  **/
EFI_STATUS
EFIAPI
SetupDeleteCert (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN CONST EFI_GUID                       *SignatureOwner
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;
  EFI_STATUS                        Status;
  UINTN                             DataSize;
  UINT8                             *Data;
  UINT8                             *OldData;
  UINT32                            Attr;
  UINT32                            Index;
  EFI_SIGNATURE_LIST                *CertList;
  EFI_SIGNATURE_LIST                *NewCertList;
  EFI_SIGNATURE_DATA                *Cert;
  UINTN                             CertCount;
  UINT32                            Offset;
  UINT32                            FullDeleteOffset;
  BOOLEAN                           IsItemFound;
  UINT32                            ItemDataSize;

  //
  // Verify This pointer.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  if (SignatureOwner == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Data            = NULL;
  OldData         = NULL;
  Cert            = NULL;
  Attr            = 0;

  //
  // Get original signature list data.
  //
  DataSize = 0;
  Status = gRT->GetVariable (VariableName, VendorGuid, NULL, &DataSize, NULL);
  if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
    return (Status);
  }

  OldData = (UINT8 *) AllocateZeroPool (DataSize);
  if (OldData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gRT->GetVariable (VariableName, VendorGuid, &Attr, &DataSize, OldData);
  if (!EFI_ERROR(Status)) {
    //
    // Allocate space for new variable.
    //
    Data = (UINT8*) AllocateZeroPool (DataSize);
    if (Data == NULL) {
      Status  =  EFI_OUT_OF_RESOURCES;
    }
  }

  if (Data != NULL) {
    //
    // Enumerate all signature data and erasing the target item.
    //
    IsItemFound     = FALSE;
    ItemDataSize = (UINT32) DataSize;
    CertList = (EFI_SIGNATURE_LIST *) OldData;
    Offset = 0;
    while ((ItemDataSize > 0) && (ItemDataSize >= CertList->SignatureListSize)) {
      FullDeleteOffset = Offset;  // If we empty list then delete full record.
      //
      // Copy EFI_SIGNATURE_LIST header then calculate the signature count in this list.
      //
      CopyMem (Data + Offset, CertList, (sizeof(EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize));
      NewCertList = (EFI_SIGNATURE_LIST*) (Data + Offset);
      Offset += (sizeof(EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
      Cert      = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
      CertCount  = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
      for (Index = 0; Index < CertCount; Index++) {
        if (CompareGuid (&Cert->SignatureOwner, SignatureOwner)) {
          //
          // Found it so skip it for updated variable.
          //
          NewCertList->SignatureListSize -= CertList->SignatureSize;
          IsItemFound = TRUE;
        } else {
          //
          // This item doesn't match. Copy it to the Data buffer.
          //
          CopyMem (Data + Offset, (UINT8*)(Cert), CertList->SignatureSize);
          Offset += CertList->SignatureSize;
        }
        Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
      }
      if (NewCertList->SignatureListSize <= sizeof (EFI_SIGNATURE_LIST)) {
        Offset = FullDeleteOffset;
      }
      ItemDataSize -= CertList->SignatureListSize;
      CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
    }

    if (!IsItemFound) {
      //
      // Doesn't find the signature Item!
      //
      Status = EFI_NOT_FOUND;
    } else {

      //
      // Delete the EFI_SIGNATURE_LIST header if there is no signature in the list.
      //
      ItemDataSize = Offset;
      CertList = (EFI_SIGNATURE_LIST *) Data;
      Offset = 0;
      ZeroMem (OldData, ItemDataSize);
      while ((ItemDataSize > 0) && (ItemDataSize >= CertList->SignatureListSize)) {
        CertCount  = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
        if (CertCount != 0) {
          CopyMem (OldData + Offset, (UINT8*)(CertList), CertList->SignatureListSize);
          Offset += CertList->SignatureListSize;
        }
        ItemDataSize -= CertList->SignatureListSize;
        CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
      }

      DataSize = Offset;
      if ((Attr & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
        Status = SetupCreateTimeBasedAuthVarPayload (This, &DataSize, &OldData);
        if (EFI_ERROR (Status)) {
          DEBUG ((EFI_D_ERROR, "SecureBootHelper:DeleteCert:CreateAuthVarPayload Failed %r", Status));
          DataSize = 0;
        }
      }
      if (DataSize > 0) {
        Status = gRT->SetVariable(
                        VariableName,
                        VendorGuid,
                        Attr,
                        DataSize,
                        OldData
                        );
        if (EFI_ERROR (Status)) {
          DEBUG ((EFI_D_ERROR, "SecureBootHelper:DeleteCert:SetVariable Failed %r\n", Status));
        }
      }
    }
  }
  if (Data != NULL) {
    FreePool(Data);
  }

  if (OldData != NULL) {
    FreePool(OldData);
  }

  return Status;
}

/** Append cert to KEK, DB or DBX vars for SecureBoot in Setup/Custom mode.

  @param[in] This            Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in] VariableName    Variable name of signature database.
  @param[in] VendorGuid      Variable vendor GUID.
  @param[in] CertType        Signature hash type.
  @param[in] SignatureOwner  GUID to identify signature.
  @param[in] SigDataSize     Signature data size.
  @param[in] SigData         Signature data.

  @retval    EFI_SUCCESS            The variable has been appended successfully.
  @retval    EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval    EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @retval    Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
SetupAppendCert (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN CONST EFI_GUID                       *CertType,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL,
  IN CONST UINTN                          SigDataSize OPTIONAL,
  IN CONST UINT8                          *SigData OPTIONAL
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;
  EFI_STATUS                        Status;
  EFI_SIGNATURE_LIST                *SigDBCert;
  EFI_SIGNATURE_DATA                *SigDBCertData;
  UINT8                             *SetData;
  UINTN                             SigDBSize;
  UINT32                            Attr;
  WIN_CERTIFICATE_UEFI_GUID         *GuidCertData;

  //
  // Verify params.
  //
  if (This == NULL || CertType == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (SigDataSize > 0 && SigData == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  SetData = NULL;
  GuidCertData = NULL;

  //
  // Create a new SigDB entry.
  //
  SigDBSize = sizeof(EFI_SIGNATURE_LIST)
              + sizeof(EFI_SIGNATURE_DATA) - 1
              + (UINT32) SigDataSize;

  SetData = (UINT8*) AllocateZeroPool (SigDBSize);
  if (SetData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Adjust the Certificate Database parameters.
  //
  SigDBCert = (EFI_SIGNATURE_LIST*) SetData;
  SigDBCert->SignatureListSize   = (UINT32) SigDBSize;
  SigDBCert->SignatureHeaderSize = 0;
  SigDBCert->SignatureSize       = sizeof(EFI_SIGNATURE_DATA) - 1 + (UINT32) SigDataSize;
  CopyGuid (&SigDBCert->SignatureType, CertType);

  SigDBCertData = (EFI_SIGNATURE_DATA*)((UINT8*)SigDBCert + sizeof(EFI_SIGNATURE_LIST));
  if (SignatureOwner != NULL) {
    CopyGuid (&SigDBCertData->SignatureOwner, SignatureOwner);
  }
  if (SigDataSize > 0) {
    CopyMem (SigDBCertData->SignatureData, SigData, SigDataSize);
  }
  Attr = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS
          | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;

  Status = SetupSetTimeBasedAuthVariable (
             This,
             VariableName,
             VendorGuid,
             Attr,
             SigDBSize,
             SetData,
             TRUE
             );

  if (SetData != NULL) {
    FreePool (SetData);
  }
  return Status;
}

/** Check if certificate list matchs filter criteria.

  Filter criteria are :-
    CertList->SignatureType GUID matches Filter->SignatureType.
    Payload size of CertList == Filter->SignatureDataPayloadSize.
    The SignatureOwner GUID of any certificate in list matches Filter->SignatureOwner.

  @param[in]       CertList        Certificate list to check.
  @param[in]       Filter          Filter criteria.
  @param[out]      CallerCertData  Pointer to update with address of EFI_SIGNATURE_DATA.

  @return 0 if no match or the number of certificates in CertList if a match found.
  @return Optionally update CallerCertData with address of EFI_SIGNATURE_DATA.

**/
UINTN
EFIAPI
CertMatch (
  IN EFI_SIGNATURE_LIST                   *CertList,
  IN FIND_CERT_FILTER                     *Filter OPTIONAL,
  OUT EFI_SIGNATURE_DATA                  **CallerCertData OPTIONAL
  )
{
  EFI_SIGNATURE_DATA *Cert;
  UINTN CertCount;
  UINTN Index;

  Cert =
    (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
  CertCount =
    (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;

  if (Filter) {
    if (Filter->SignatureType) {
      if (CompareGuid (&CertList->SignatureType, Filter->SignatureType) == FALSE) {
        return 0;
      }
    }
    if (Filter->SignatureDataPayloadSize != 0) {
      if (CertList->SignatureSize != sizeof(EFI_SIGNATURE_DATA) - 1 + (UINT32) Filter->SignatureDataPayloadSize) {
        return 0;
      }
    }

    if (Filter->SignatureOwner) {
      for (Index = 0; Index < CertCount; Index++) {
        if (CompareGuid (&Cert->SignatureOwner, Filter->SignatureOwner) == TRUE) {
          break;
        }
        Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
      }
      if (Index == CertCount) {
        return 0;
      }
    }
  }
  if (CallerCertData) {
    *CallerCertData = Cert;
  }
  return CertCount;
}

/** Find first cert. list in security database that matches filter criteria.

  If Filter == NULL then return first cert list in database.

  Caller must call Find Cert Close to free resources allocated by this find.

  @param[in]       This          Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       VariableName  Security database variable name.
  @param[in]       VendorGuid    Security database variable vendor GUID.
  @param[in]       Filter        Filter criteria.
  @param[out]      CertList      Pointer to update with address of EFI_SIGNATURE_LIST.
  @param[out]      CertData      Pointer to update with address of EFI_SIGNATURE_DATA.

  @retval NULL   Nothing found in database.
  @retval other  Pointer to this found context to be passed to Find Next Cert
                 and Find Cert Close.
  @return Updated CertList with address of EFI_SIGNATURE_LIST structure of the
          found certificate list.
  @return Optionally update CertData with address of EFI_SIGNATURE_DATA of the
          found certificate list.

**/
VOID *
EFIAPI
FindFirstCert (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN FIND_CERT_FILTER                     *Filter OPTIONAL,
  OUT EFI_SIGNATURE_LIST                  **CertList,
  OUT EFI_SIGNATURE_DATA                  **CertData OPTIONAL,
  OUT FIND_CERT_STATS                     *Stats OPTIONAL
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;
  EFI_STATUS                        Status;
  UINT8                             *FindContext;
  UINTN                             DataSize;
  UINTN                             AllocSize;
  EFI_SIGNATURE_LIST                *MatchList;
  UINT32                            Attr;

  if (Stats != NULL) {
    Stats->VariableSize = 0;
    Stats->VariableAttributes = 0;
  }

  //
  // Verify This pointer.
  //
  if (This == NULL) {
    return NULL;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return NULL;
  }

  MatchList = NULL;
  DataSize = 0;

  if (CertList == NULL) {
    return NULL;
  }

  Status   = gRT->GetVariable (VariableName, VendorGuid, NULL, &DataSize, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return NULL;
  }
  AllocSize = DataSize + sizeof(VOID *);
  FindContext = (UINT8 *) AllocateZeroPool (AllocSize);
  if (FindContext == NULL) {
    return NULL;
  }
  *((UINT8 **) FindContext) = FindContext + AllocSize;

  Status = gRT->GetVariable (
                  VariableName,
                  VendorGuid,
                  &Attr,
                  &DataSize,
                  (VOID *) (FindContext + sizeof(UINT8 *))
                  );

  if (!EFI_ERROR (Status)) {
    if (Stats != NULL) {
      Stats->VariableSize = DataSize;
      Stats->VariableAttributes = Attr;
    }
    MatchList = (EFI_SIGNATURE_LIST *) (FindContext + sizeof(UINT8 *));
    if (CertMatch (MatchList, Filter, CertData) == 0) {
      MatchList = FindNextCert (
                    FindContext,
                    Filter,
                    MatchList,
                    CertData);
    }
  }

  if (MatchList == NULL) {
    FreePool (FindContext);
    return NULL;
  }

  *CertList = MatchList;

  return (VOID *) FindContext;
}

/** Find next cert. list in security database that matches filter criteria.

  If Filter == NULL then return next cert list in database.

  @param[in]       FindContext   Return from previous call to Find First Cert.
  @param[in]       Filter        Filter criteria.
  @param[out]      CurrCertList  Current position in security database.
  @param[out]      CertData      Pointer to update with address of EFI_SIGNATURE_DATA.

  @retval NULL   Reached end of database.
  @retval other  Pointer to EFI_SIGNATURE_LIST structure of the
                 next certificate list.
  @return Optionally update CertData with address of EFI_SIGNATURE_DATA of the
          next certificate list.

**/
EFI_SIGNATURE_LIST *
EFIAPI
FindNextCert (
  IN VOID                                 *FindContext,
  IN FIND_CERT_FILTER                     *Filter,
  IN EFI_SIGNATURE_LIST                   *CurrCertList,
  OUT EFI_SIGNATURE_DATA                  **CertData
  )
{
  UINT8                             *Limit;
  EFI_SIGNATURE_LIST                *Next;

  if (FindContext == NULL || CurrCertList == NULL) {
    return NULL;
  }
  Limit = *((UINT8 **) FindContext);
  Next = CurrCertList;
  do {
    Next = (EFI_SIGNATURE_LIST *) (((UINT8 *) Next) + Next->SignatureListSize);
    if (((UINT8 *) Next) >= Limit) {
      return NULL;
    }
  } while (CertMatch (Next, Filter, CertData) == 0);
  return Next;
}

/** Free resources allocated by Find First Cert.

  @param[in]       FindContext   Return from previous call to Find First Cert.

**/
VOID
EFIAPI
FindCertClose (
  IN VOID                                 *FindContext
  )
{
  if (FindContext) {
    FreePool (FindContext);
  }
}

/** Check if Platform Key is enrolled.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[out]      Optional return of size of 'pk' variable.
  @param[out]      Optional return of attributes of 'pk' variable.

  @retval  TRUE    Platform Key is enrolled.
  @retval  FALSE   System has no Platform Key or Error with key var.

**/
BOOLEAN
EFIAPI
IsPkEnrolled (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  OUT UINTN                               *PkVarSize OPTIONAL,
  OUT UINT32                              *PkVarAttr OPTIONAL
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA     *PrivateData;
  EFI_STATUS                         Status;
  UINTN                              VarSize;
  UINT8                             *Data;

  ASSERT (This != NULL);
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  ASSERT (PrivateData->Signature == SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE);

  VarSize = 0;

  Status = gRT->GetVariable (
                  EFI_PLATFORM_KEY_NAME,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &VarSize,
                  NULL
                  );

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return FALSE;
  }

  if (PkVarSize) {
    *PkVarSize = VarSize;
  }

  if (PkVarAttr) {
    Data = (UINT8 *) AllocatePool (VarSize);
    if (Data == NULL) {
      return 0;
    }

    Status = gRT->GetVariable (EFI_PLATFORM_KEY_NAME, &gEfiGlobalVariableGuid, PkVarAttr, &VarSize, Data);
    FreePool (Data);
    if (EFI_ERROR(Status)) {
      return FALSE;
    }
  }
  return TRUE;
}

/**  Enroll X509 Cert file into PK, KEK, DB or DBX database.

  For use with SecureBoot in Setup/Custom mode.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       StoreType      PKStore, KEKStore, DBStore or DBXStore.
  @param[in]       FileDevicePath Device Path of X509 certificate file (.der /.cer / .crt)
                                  in format using ASN.1 DER encoding.
  @param[in]       SourceBuffer   If not NULL, a pointer to the memory location containing a copy
                                  of the certificate, overrides FileDevicePath.
  @param[in]       SourceSize     The size in bytes of SourceBuffer. Ignored if SourceBuffer is NULL.
  @param[in]       SignatureOwner Identifier for this enrollment.

  @retval  EFI_SUCCESS            Enroll success.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Resource not enough to set variable.
  @retval  EFI_WRITE_PROTECTED    Resource is read-only.
  @retval  EFI_NOT_FOUND          File not found.
  @retval  EFI_SECURITY_VIOLATION System in invalid mode for operation.
  @retval  Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
SetupEnrollX509 (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST SECUREBOOT_STORE_TYPE          StoreType,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST VOID                           *SourceBuffer OPTIONAL,
  IN UINTN                                SourceSize,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  )
{
  EFI_STATUS                        Status;
  UINT8                             *X509Data;
  UINTN                             X509Size;
  UINT32                            AStat;
  BOOLEAN                           FreeX509Data;
  UINT32                            Attr;
  UINTN                             DataSize;
  EFI_SIGNATURE_LIST                *PkCert;
  EFI_SIGNATURE_DATA                *PkCertData;

  if (SourceBuffer != NULL) {
    X509Data = (UINT8 *) SourceBuffer;
    X509Size = SourceSize;
    FreeX509Data = FALSE;
  } else {
    X509Data = MyGetFileBufferByFilePath (FileDevicePath, &X509Size, &AStat);

    if (X509Data == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    FreeX509Data = TRUE;
  }

  if (StoreType == DBStore) {
    Status = SetupAppendCert (
           This,
           EFI_IMAGE_SECURITY_DATABASE,
           &gEfiImageSecurityDatabaseGuid,
           &gEfiCertX509Guid,
           SignatureOwner,
           X509Size,
           X509Data
           );
  } else if (StoreType == DBXStore) {
    Status = SetupAppendCert (
           This,
           EFI_IMAGE_SECURITY_DATABASE1,
           &gEfiImageSecurityDatabaseGuid,
           &gEfiCertX509Guid,
           SignatureOwner,
           X509Size,
           X509Data
           );
  } else if (StoreType == KEKStore) {
    Status = SetupAppendCert (
           This,
           EFI_KEY_EXCHANGE_KEY_NAME,
           &gEfiGlobalVariableGuid,
           &gEfiCertX509Guid,
           SignatureOwner,
           X509Size,
           X509Data
           );
  } else if (StoreType == PKStore) {
    //
    // Allocate space for PK certificate list and initialize it.
    // Create PK database entry with SignatureHeaderSize equals 0.
    //
    DataSize = sizeof(EFI_SIGNATURE_LIST) + sizeof(EFI_SIGNATURE_DATA) - 1
               + X509Size;

    PkCert = (EFI_SIGNATURE_LIST*) AllocateZeroPool (DataSize);
    if (PkCert == NULL) {
      FreePool (X509Data);
      Status = EFI_OUT_OF_RESOURCES;
    } else {

      //
      // Enroll pk X509 cert.
      //
      PkCert->SignatureListSize = (UINT32) DataSize;
      PkCert->SignatureSize = (UINT32) (sizeof(EFI_SIGNATURE_DATA) - 1 + X509Size);
      PkCert->SignatureHeaderSize = 0;
      CopyGuid (&PkCert->SignatureType, &gEfiCertX509Guid);
      PkCertData = (EFI_SIGNATURE_DATA*) ((UINTN)(PkCert)
                                          + sizeof(EFI_SIGNATURE_LIST)
                                          + PkCert->SignatureHeaderSize);
      CopyGuid (&PkCertData->SignatureOwner, &gEfiGlobalVariableGuid);
      //
      // Fill the PK database with PKpub data from X509 certificate file.
      //
      CopyMem (&(PkCertData->SignatureData[0]), X509Data, X509Size);

      Attr = EFI_VARIABLE_NON_VOLATILE
              | EFI_VARIABLE_RUNTIME_ACCESS
              | EFI_VARIABLE_BOOTSERVICE_ACCESS
              | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;

      Status = SetupSetTimeBasedAuthVariable (
                 This,
                 EFI_PLATFORM_KEY_NAME,
                 &gEfiGlobalVariableGuid,
                 Attr,
                 DataSize,
                 (UINT8 *) PkCert,
                 FALSE
                 );

      FreePool (PkCert);
    }
  } else {
    Status = EFI_INVALID_PARAMETER;
  }
  if (FreeX509Data) {
    FreePool (X509Data);
  }
  DEBUG ((EFI_D_INFO, "SecureBootHelper:EnrollX509 %r Store = 0x%02x Owner = %g\n", Status, (UINTN) StoreType, SignatureOwner));

  return Status;
}

/**  Enroll Rsa2048 storing file (*.pbk) into KEK database.

  SecureBoot in Setup/Custom mode.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       FileDevicePath Device Path of Rsa2048 storing file (*.pbk).
  @param[in]       SourceBuffer   If not NULL, a pointer to the memory location containing a copy
                                  of the pbk file, overrides FileDevicePath.
  @param[in]       SourceSize     The size in bytes of SourceBuffer. Ignored if SourceBuffer is NULL.
  @param[in]       SignatureOwner Identifier for this enrollment.

  @retval  EFI_SUCCESS            Enroll success.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Resource not enough to set variable.
  @retval  EFI_WRITE_PROTECTED    Resource is read-only.
  @retval  EFI_UNSUPPORTED        Unsupported file.
  @retval  EFI_NOT_FOUND          File not found.
  @retval  EFI_SECURITY_VIOLATION System in invalid mode for operation.
  @retval  Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
SetupEnrollKekRsa2048 (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST VOID                           *SourceBuffer OPTIONAL,
  IN UINTN                                SourceSize,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  )
{
  EFI_STATUS                        Status;
  UINTN                             KeyBlobSize;
  UINT8                             *KeyBlob;
  CPL_KEY_INFO                      *KeyInfo;
  UINT32                            AStat;
  UINTN                             KeyLenInBytes;
  UINT8                             *KeyBuffer;
  BOOLEAN                           FreeBlob;

  if (SourceBuffer != NULL) {
    KeyBlob = (UINT8 *) SourceBuffer;
    KeyBlobSize = SourceSize;
    FreeBlob = FALSE;
  } else {
    KeyBlob = MyGetFileBufferByFilePath (FileDevicePath, &KeyBlobSize, &AStat);

    if (KeyBlob == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    FreeBlob = TRUE;
  }
  KeyBuffer = NULL;

  KeyInfo = (CPL_KEY_INFO *) KeyBlob;
  if (KeyInfo->KeyLengthInBits / 8 != WIN_CERT_UEFI_RSA2048_SIZE) {
    DEBUG ((EFI_D_ERROR, "SecureBootHelper:EnrollKekRsa2048 Unsupported key length, Only RSA2048 is supported.\n"));
    Status = EFI_UNSUPPORTED;
  } else {

    //
    // Convert the Public key to fix octet string format represented in RSA PKCS#1.
    // 
    KeyLenInBytes = KeyInfo->KeyLengthInBits / 8;
    KeyBuffer = AllocateZeroPool (KeyLenInBytes);
    if (KeyBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      MyInt2OctStr (
        (UINTN*) (KeyBlob + sizeof (CPL_KEY_INFO)),
        KeyLenInBytes / sizeof (UINTN),
        KeyBuffer,
        KeyLenInBytes
        );
      CopyMem(KeyBlob + sizeof(CPL_KEY_INFO), KeyBuffer, KeyLenInBytes);

      Status = SetupAppendCert (
                 This,
                 EFI_KEY_EXCHANGE_KEY_NAME,
                 &gEfiGlobalVariableGuid,
                 &gEfiCertRsa2048Guid,
                 SignatureOwner,
                 WIN_CERT_UEFI_RSA2048_SIZE,
                 KeyBlob + sizeof(CPL_KEY_INFO)
           );
    }
  }

  if (KeyBuffer) {
    FreePool (KeyBuffer);
  }

  if (FreeBlob) {
    FreePool (KeyBlob);
  }
  DEBUG ((EFI_D_INFO, "SecureBootHelper:EnrollKekRsa2048 %r Owner = %g\n", Status, SignatureOwner));
  return Status;
}

/**  Enroll Image Signature into secure boot database.

  SecureBoot in Setup/Custom mode.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       StoreType      PKStore, KEKStore, DBStore or DBXStore.
  @param[in]       FileDevicePath Device Path of image to enroll.
  @param[in]       SourceBuffer   If not NULL, a pointer to the memory location containing SHA256
                                  32 byte image digest. Assumed to match &gEfiCertSha256Guid.
  @param[in]       SourceSize     The size in bytes of SourceBuffer. Ignored if SourceBuffer is NULL.
  @param[in]       SignatureOwner Identifier for this enrollment.

  @retval  EFI_SUCCESS            Enroll success.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Resource not enough to set variable.
  @retval  EFI_WRITE_PROTECTED    Resource is read-only.
  @retval  EFI_UNSUPPORTED        Unsupported image type.
  @retval  EFI_NOT_FOUND          File not found.
  @retval  EFI_SECURITY_VIOLATION System in invalid mode for operation.
  @retval  Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
SetupEnrollImageSignature (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST SECUREBOOT_STORE_TYPE          StoreType,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST VOID                           *SourceBuffer OPTIONAL,
  IN UINTN                                SourceSize,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  )
{
  EFI_STATUS                              Status;
  SECUREBOOT_PEIMAGE_LOAD_INFO            *LoadInfo;
  CHAR16                                  *VariableName;
  EFI_GUID                                *VendorGuid;

  if (StoreType == DBStore) {
    VariableName = EFI_IMAGE_SECURITY_DATABASE;
  } else if (StoreType == DBXStore) {
    VariableName = EFI_IMAGE_SECURITY_DATABASE1;
  } else {
    return EFI_INVALID_PARAMETER;
  }
  VendorGuid = &gEfiImageSecurityDatabaseGuid;

  if (SourceBuffer != NULL) {
    Status = SetupAppendCert (
               This,
               VariableName,
               VendorGuid,
               &gEfiCertSha256Guid,
               SignatureOwner,
               SourceSize,
               (UINT8 *) SourceBuffer
               );
  } else {
    LoadInfo = LoadPeImageWork (FileDevicePath, TRUE);
    if (LoadInfo == NULL) {
      Status = EFI_NOT_FOUND;
    } else {
      if (LoadInfo->ImageDigestSize == 0) {
        Status = EFI_UNSUPPORTED;
      } else {
        Status =SetupAppendCert (
                   This,
                   VariableName,
                   VendorGuid,
                   &LoadInfo->ImageCertType,
                   SignatureOwner,
                   LoadInfo->ImageDigestSize,
                   LoadInfo->ImageDigest
                   );
      }
      FreePool (LoadInfo);
    }
  }
  DEBUG ((EFI_D_INFO, "SecureBootHelper:EnrollImageSignature %r Store = 0x%02x Owner = %g\n", Status, (UINTN) StoreType, SignatureOwner));
  return Status;
}

/**  Delete record in SecureBoot PK, KEK, DB or DBX or delete a complete store.

  SecureBoot in Setup/Custom mode.

  if CompleteStore then reset StoreType with all certs deleted.
  CompleteStore param ignored if StoreType == PKStore ie complete PK always reset.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       StoreType      PKStore, KEKStore, DBStore or DBXStore.
  @param[in]       CompleteStore  If true reset complete secure boot store (StoreType).
  @param[in]       SignatureOwner Specific record in store to delete.

  @retval  EFI_SUCCESS            Delete success.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_OUT_OF_RESOURCES   Resource not enough to do operation.
  @retval  EFI_WRITE_PROTECTED    Resource is read-only.
  @retval  EFI_SECURITY_VIOLATION System in invalid mode for operation.
  @retval  Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
SetupDeleteStoreRecord (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST SECUREBOOT_STORE_TYPE          StoreType,
  IN CONST BOOLEAN                        CompleteStore,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  )
{
  CHAR16                            *VariableName;
  EFI_GUID                          *VendorGuid;

  if (StoreType == DBStore) {
    VariableName = EFI_IMAGE_SECURITY_DATABASE;
    VendorGuid = &gEfiImageSecurityDatabaseGuid;
  } else if (StoreType == DBXStore) {
    VariableName = EFI_IMAGE_SECURITY_DATABASE1;
    VendorGuid = &gEfiImageSecurityDatabaseGuid;
  } else if (StoreType == KEKStore) {
    VariableName = EFI_KEY_EXCHANGE_KEY_NAME;
    VendorGuid = &gEfiGlobalVariableGuid;
  } else if (StoreType == PKStore) {
    return SetupCreateTimeBasedAuthVar (
             This,
             EFI_PLATFORM_KEY_NAME,
             &gEfiGlobalVariableGuid,
             FALSE
             );
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if (CompleteStore) {
    return SetupCreateTimeBasedAuthVar (
             This,
             VariableName,
             VendorGuid,
             FALSE
             );
  }
  return SetupDeleteCert (
           This,
           VariableName,
           VendorGuid,
           SignatureOwner
           );
}

/**  Get statistics for SecureBoot PK, KEK, DB and DBX variables.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[out]      Stats          Caller allocated SECUREBOOT_VARIABLE_STATS
                                  struct filled out by this routine.

  @retval  EFI_SUCCESS            Operation success with Stats filled out.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
GetSecureBootVarStats (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  OUT SECUREBOOT_VARIABLE_STATS           *Stats
  )
{
  UINTN                             LastSize;
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;

  //
  // Verify This & Stats pointers.
  //
  if (This == NULL || Stats == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }
  ZeroMem (Stats, sizeof (SECUREBOOT_VARIABLE_STATS));
  Stats->PkEnrolled = IsPkEnrolled (
                        This,
                        &Stats->MaxVarSize,
                        &Stats->PkAttributes
                        );

  LastSize = GetSecureBootCertVarStats (
               This,
               EFI_KEY_EXCHANGE_KEY_NAME,
               &gEfiGlobalVariableGuid,
               &Stats->KekAttributes,
               &Stats->KekTotalCertCount,
               &Stats->KekX509CertCount,
               &Stats->KekRsa2048CertCount,
               &Stats->KekOtherCertCount,
               NULL,
               NULL
               );
  Stats->KekEnrolled = (LastSize > 0);
  Stats->MaxVarSize = MAX(Stats->MaxVarSize, LastSize);

  LastSize = GetSecureBootCertVarStats (
               This,
               EFI_IMAGE_SECURITY_DATABASE,
               &gEfiImageSecurityDatabaseGuid,
               &Stats->DbAttributes,
               &Stats->DbTotalCertCount,
               &Stats->DbX509CertCount,
               NULL,
               &Stats->DbOtherCertCount,
               &Stats->DbSha256Count,
               &Stats->DbSha1Count
               );
  Stats->DbEnrolled = (LastSize > 0);
  Stats->MaxVarSize = MAX(Stats->MaxVarSize, LastSize);

  LastSize = GetSecureBootCertVarStats (
               This,
               EFI_IMAGE_SECURITY_DATABASE1,
               &gEfiImageSecurityDatabaseGuid,
               &Stats->DbxAttributes,
               &Stats->DbxTotalCertCount,
               &Stats->DbxX509CertCount,
               NULL,
               &Stats->DbxOtherCertCount,
               &Stats->DbxSha256Count,
               &Stats->DbxSha1Count
               );
  Stats->DbxEnrolled = (LastSize > 0);
  Stats->MaxVarSize = MAX(Stats->MaxVarSize, LastSize);

  return EFI_SUCCESS;
}

/**  Create Time based authenticated variable for SecureBoot Setup/Custom Mode.

  @param[in]       This             Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.
  @param[in]       VariableName     Name of Variable to set.
  @param[in]       VendorGuid       Variable vendor GUID.
  @param[in]       LeaveIfExists    If variable already exists do nothing.

  @retval    EFI_SUCCESS            The variable has been appended successfully.
  @retval    EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval    EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @retval    EFI_WRITE_PROTECTED    Variable is read-only.
  @retval    Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
SetupCreateTimeBasedAuthVar (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN  CHAR16                              *VariableName,
  IN  EFI_GUID                            *VendorGuid,
  IN CONST BOOLEAN                        LeaveIfExists
  )
{
  EFI_STATUS                        Status;
  VOID*                             Variable;
  UINT8                             *Data;
  UINTN                             DataSize;
  UINT32                            Attr;

  GetVariable2 (VariableName, VendorGuid, &Variable, NULL);
  if (Variable != NULL) {
    FreePool (Variable);
    if (LeaveIfExists) {
      return EFI_SUCCESS;
    }
  }

  Data     = NULL;
  DataSize = 0;
  Attr     = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS
             | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | EFI_VARIABLE_APPEND_WRITE;

  Status = SetupCreateTimeBasedAuthVarPayload (This, &DataSize, &Data);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SecureBootHelper:CreateAuthVariable:CreateAuthVarPayload failed %r", Status));
    return Status;
  }

  Status = gRT->SetVariable (
                  VariableName,
                  VendorGuid,
                  Attr,
                  DataSize,
                  Data
                  );
  if (Data != NULL) {
    FreePool (Data);
  }
  return Status;
}

/**  Create Empty DB & DBX if they do not exist.

  SecureBoot in Setup/Custom mode.

  @param[in]       This           Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval    EFI_SUCCESS            The variable has been appended successfully.
  @retval    EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval    EFI_OUT_OF_RESOURCES   Could not allocate needed resources.
  @retval    Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
SetupCreateEmptyDbDbxIfNotExist (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  )
{
  EFI_STATUS                        Status;
  CHAR16                            *VariableNames[2];
  EFI_GUID                          ZeroGuid;
  UINTN                             Index;
  UINTN                             VarSize;
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;

  //
  // Verify This pointer.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&ZeroGuid, sizeof (ZeroGuid));

  VariableNames[0] = EFI_IMAGE_SECURITY_DATABASE;
  VariableNames[1] = EFI_IMAGE_SECURITY_DATABASE1;
  Status = EFI_SUCCESS;

  for (Index=0;Index < sizeof(VariableNames) / sizeof(VariableNames[0]);Index++) {
    VarSize = 0;

    Status = gRT->GetVariable(
                    VariableNames[Index],
                    &gEfiImageSecurityDatabaseGuid,
                    NULL,
                    &VarSize,
                    NULL
                    );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Status = EFI_SUCCESS;
      continue;  // Already setup do nothing.
    } else if (Status == EFI_NOT_FOUND) {

      //
      // Create an empty version with a dummy Signature if required.
      // Use null guid for SignatureType in dummy signature so it is ignored
      // by image verifier.
      //
      Status = SetupAppendCert (
                 This,
                 VariableNames[Index],
                 &gEfiImageSecurityDatabaseGuid,
                 &ZeroGuid,
                 NULL,
                 0,
                 NULL
                 );
      ASSERT_EFI_ERROR (Status);
    } else {
      //
      // Always expect error Status if Status != EFI_BUFFER_TOO_SMALL
      // and Status != EFI_NOT_FOUND.
      //
      ASSERT (EFI_ERROR (Status));
      ASSERT (FALSE);
    }
  }

  return Status;
}
