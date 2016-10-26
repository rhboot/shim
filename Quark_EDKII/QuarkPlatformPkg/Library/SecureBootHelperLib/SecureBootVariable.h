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

  SecureBootVariable.h

Abstract:

  Header file for SecureBootVariable.c.

--*/

#ifndef __SECUREBOOT_HELPER_VARIABLE_HEADER_H_
#define __SECUREBOOT_HELPER_VARIABLE_HEADER_H_

//
// Cryptograhpic Key Information
//
#pragma pack(1)
typedef struct _CPL_KEY_INFO {
  UINT32        KeyLengthInBits;    // Key Length In Bits
  UINT32        BlockSize;          // Operation Block Size in Bytes
  UINT32        CipherBlockSize;    // Output Cipher Block Size in Bytes
  UINT32        KeyType;            // Key Type
  UINT32        CipherMode;         // Cipher Mode for Symmetric Algorithm
  UINT32        Flags;              // Additional Key Property Flags
} CPL_KEY_INFO;
#pragma pack()

BOOLEAN
EFIAPI
IsSecureBootEnabledWork (
  VOID
  );

BOOLEAN
EFIAPI
IsSecureBootEnabled (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

BOOLEAN
EFIAPI
IsSecureBootInCustomModeWork (
  VOID
  );

BOOLEAN
EFIAPI
IsSecureBootInCustomMode (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

EFI_STATUS
EFIAPI
SetupSetSecureBootCustomMode (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

EFI_STATUS
EFIAPI
SetupSetSecureBootDefaultMode (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

EFI_STATUS
EFIAPI
SetupCreateTimeBasedAuthVarPayload (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN OUT UINTN                            *DataSize,
  IN OUT UINT8                            **Data
  );

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
  );

EFI_STATUS
EFIAPI
SetupDeleteCert (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN CONST EFI_GUID                       *SignatureOwner
  );

EFI_STATUS
EFIAPI
SetupAppendCert (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CHAR16                               *VariableName,
  IN EFI_GUID                             *VendorGuid,
  IN CONST EFI_GUID                       *CertType,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL,
  IN CONST UINTN                          SigDataSize,
  IN CONST UINT8                          *SigData
  );

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
  );

EFI_SIGNATURE_LIST *
EFIAPI
FindNextCert (
  IN VOID                                 *FindContext,
  IN FIND_CERT_FILTER                     *Filter,
  IN EFI_SIGNATURE_LIST                   *CurrCertList,
  OUT EFI_SIGNATURE_DATA                  **CertData
  );

VOID
EFIAPI
FindCertClose (
  IN VOID                                 *FindContext
  );

BOOLEAN
EFIAPI
IsPkEnrolled (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  OUT UINTN                               *PkVarSize OPTIONAL,
  OUT UINT32                              *PkVarAttr OPTIONAL
  );

EFI_STATUS
EFIAPI
SetupEnrollX509 (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST SECUREBOOT_STORE_TYPE          StoreType,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST VOID                           *SourceBuffer OPTIONAL,
  IN UINTN                                SourceSize,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  );

EFI_STATUS
EFIAPI
SetupEnrollKekRsa2048 (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST VOID                           *SourceBuffer OPTIONAL,
  IN UINTN                                SourceSize,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  );

EFI_STATUS
EFIAPI
SetupEnrollImageSignature (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST SECUREBOOT_STORE_TYPE          StoreType,
  IN CONST EFI_DEVICE_PATH_PROTOCOL       *FileDevicePath,
  IN CONST VOID                           *SourceBuffer OPTIONAL,
  IN UINTN                                SourceSize,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  );

EFI_STATUS
EFIAPI
SetupDeleteStoreRecord (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST SECUREBOOT_STORE_TYPE          StoreType,
  IN CONST BOOLEAN                        CompleteStore,
  IN CONST EFI_GUID                       *SignatureOwner OPTIONAL
  );

EFI_STATUS
EFIAPI
GetSecureBootVarStats (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  OUT SECUREBOOT_VARIABLE_STATS           *Stats
  );

EFI_STATUS
EFIAPI
SetupCreateTimeBasedAuthVar (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN  CHAR16                              *VariableName,
  IN  EFI_GUID                            *VendorGuid,
  IN CONST BOOLEAN                        LeaveIfExists
  );

EFI_STATUS
EFIAPI
SetupCreateEmptyDbDbxIfNotExist (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  );

#endif  // __SECUREBOOT_HELPER_VARIABLE_HEADER_H_
