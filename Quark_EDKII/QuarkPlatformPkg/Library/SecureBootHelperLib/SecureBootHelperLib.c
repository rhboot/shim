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

  SecureBootHelperLib.c

Abstract:

  This component produces the secure boot helper protocol.

--*/

#include "CommonHeader.h"

//
// Handle for the secure boot helper protocol.
//
EFI_HANDLE                  mSecureBootHelperProtocolHandle = NULL;

/**
  Convert a nonnegative integer to an octet string of a specified length.

  @param[in]   Integer          Pointer to the nonnegative integer to be converted.
  @param[in]   IntSizeInWords   Length of integer buffer in words.
  @param[out]  OctetString      Converted octet string of the specified length.
  @param[in]   OSSizeInBytes    Intended length of resulting octet string in bytes.

Returns:

  @retval   EFI_SUCCESS            Data conversion successfully.
  @retval   EFI_BUFFER_TOOL_SMALL  Buffer is too small for output string.

**/
EFI_STATUS
EFIAPI
MyInt2OctStr (
  IN     CONST UINTN                *Integer,
  IN     UINTN                      IntSizeInWords,
     OUT UINT8                      *OctetString,
  IN     UINTN                      OSSizeInBytes
  )
{
  CONST UINT8  *Ptr1;
  UINT8        *Ptr2;

  for (Ptr1 = (CONST UINT8 *)Integer, Ptr2 = OctetString + OSSizeInBytes - 1;
       Ptr1 < (UINT8 *)(Integer + IntSizeInWords) && Ptr2 >= OctetString;
       Ptr1++, Ptr2--) {
    *Ptr2 = *Ptr1;
  }

  for (; Ptr1 < (CONST UINT8 *)(Integer + IntSizeInWords) && *Ptr1 == 0; Ptr1++);

  if (Ptr1 < (CONST UINT8 *)(Integer + IntSizeInWords)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Ptr2 >= OctetString) {
    ZeroMem (OctetString, Ptr2 - OctetString + 1);
  }

  return EFI_SUCCESS;
}

/**
  Get the image file buffer data and buffer size by its device path.

  Access the file either from a firmware volume, from a file system interface,
  or from the load file interface.

  Allocate memory to store the found image. The caller is responsible to free memory.

  If File is NULL, then NULL is returned.
  If FileSize is NULL, then NULL is returned.
  If AuthenticationStatus is NULL, then NULL is returned.

  @param[in]       FilePath             Pointer to the device path of the file that is abstracted to
                                        the file buffer.
  @param[out]      FileSize             Pointer to the size of the abstracted file buffer.
  @param[out]      AuthenticationStatus Pointer to a caller-allocated UINT32 in which the authentication
                                        status is returned.

  @retval NULL   File is NULL, or FileSize is NULL, or the file can't be found.
  @retval other  The abstracted file buffer. The caller is responsible to free memory.
**/
VOID *
EFIAPI
MyGetFileBufferByFilePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL    *FilePath,
  OUT      UINTN                       *FileSize,
  OUT UINT32                           *AuthenticationStatus
  )
{
  VOID *FileBuffer;
  FileBuffer = NULL;

  //
  // Try to get image by FALSE boot policy for the exact boot file path.
  //
  FileBuffer = GetFileBufferByFilePath (FALSE, FilePath, FileSize, AuthenticationStatus);
  if (FileBuffer == NULL) {
    //
    // Try to get image by TRUE boot policy for the inexact boot file path.
    //
    FileBuffer = GetFileBufferByFilePath (TRUE, FilePath, FileSize, AuthenticationStatus);
  }
  return FileBuffer;
}

/** Check if platform is operating by a physically present user.

  This function provides a platform-specific method to detect whether the platform
  is operating by a physically present user.

  Programmatic changing of platform security policy (such as disable Secure Boot,
  or switch between Standard/Custom Secure Boot mode) MUST NOT be possible during
  Boot Services or after exiting EFI Boot Services. Only a physically present user
  is allowed to perform these operations.

  NOTE THAT: This function cannot depend on any EFI Variable Service since they are
  not available when this function is called in AuthenticateVariable driver.

  @param[in]       This  Pointer to SECUREBOOT_HELPER_PROTOCOL located by caller.

  @retval  TRUE    The platform is operated by a physically present user.
  @retval  FALSE   The platform is NOT operated by a physically present user.

**/
BOOLEAN
EFIAPI
IsUserPhysicalPresent (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This
  )
{
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;
  ASSERT (This != NULL);
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  ASSERT (PrivateData->Signature == SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE);

  return UserPhysicalPresent ();
}

/**
  Installs Secure Boot Helper Protocol.

  @param  ImageHandle  The image handle of this driver.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS   Install the Secure Boot Helper Protocol successfully.

**/
EFI_STATUS
EFIAPI
SecureBootHelperInitialize (
  IN EFI_HANDLE                           ImageHandle,
  IN EFI_SYSTEM_TABLE                     *SystemTable
  )
{
  EFI_STATUS                              Status;
  SECUREBOOT_HELPER_PRIVATE_DATA          *PrivateData;

  //
  // Create & Init a private data structure.
  //
  PrivateData = AllocateZeroPool (sizeof (SECUREBOOT_HELPER_PRIVATE_DATA));
  if (PrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  PrivateData->ImageHandle = ImageHandle;
  PrivateData->Signature = SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE;
  PrivateData->SBHP.IsUserPhysicalPresent = IsUserPhysicalPresent;
  PrivateData->SBHP.HashMultipleBuffers = HashMultipleBuffers;
  PrivateData->SBHP.LoadPeImage = LoadPeImage;
  PrivateData->SBHP.IsSecureBootEnabled = IsSecureBootEnabled;
  PrivateData->SBHP.IsSecureBootInCustomMode = IsSecureBootInCustomMode;
  PrivateData->SBHP.SetupSetSecureBootCustomMode = SetupSetSecureBootCustomMode;
  PrivateData->SBHP.SetupSetSecureBootDefaultMode = SetupSetSecureBootDefaultMode;
  PrivateData->SBHP.SetupCreateTimeBasedAuthVarPayload = SetupCreateTimeBasedAuthVarPayload;
  PrivateData->SBHP.SetupSetTimeBasedAuthVariable = SetupSetTimeBasedAuthVariable;
  PrivateData->SBHP.SetupDeleteCert = SetupDeleteCert;
  PrivateData->SBHP.SetupAppendCert = SetupAppendCert;
  PrivateData->SBHP.FindFirstCert = FindFirstCert;
  PrivateData->SBHP.FindNextCert = FindNextCert;
  PrivateData->SBHP.FindCertClose = FindCertClose;
  PrivateData->SBHP.IsPkEnrolled = IsPkEnrolled;
  PrivateData->SBHP.SetupEnrollX509 = SetupEnrollX509;
  PrivateData->SBHP.SetupEnrollKekRsa2048 = SetupEnrollKekRsa2048;
  PrivateData->SBHP.SetupEnrollImageSignature = SetupEnrollImageSignature;
  PrivateData->SBHP.SetupDeleteStoreRecord = SetupDeleteStoreRecord;
  PrivateData->SBHP.GetImageFromMask = GetImageFromMask;
  PrivateData->SBHP.GetSecureBootVarStats = GetSecureBootVarStats;
  PrivateData->SBHP.SetupCreateTimeBasedAuthVar = SetupCreateTimeBasedAuthVar;
  PrivateData->SBHP.SetupCreateEmptyDbDbxIfNotExist = SetupCreateEmptyDbDbxIfNotExist;

  //
  // Make sure the Secure Boot Helper DXE Protocol is not already installed
  // in the system.
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gSecureBootHelperProtocolGuid);

  //
  // Install the SecureBoot Helper Protocol onto a new handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSecureBootHelperProtocolHandle,
                  &gSecureBootHelperProtocolGuid,
                  &PrivateData->SBHP,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
