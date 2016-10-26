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

  PlatformSecureBoot.c

Abstract:

  Implementation of platform specific UEFI SecureBoot functionality.

--*/

#include <PiDxe.h>

#include <Guid/ImageAuthentication.h>
#include <Guid/ImageAuthentication.h>

#include <Protocol/Spi.h>
#include <Protocol/SecureBootHelper.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeServicesLib.h>

#include "CommonHeader.h"

STATIC EFI_GUID NullGuid = {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

//
// Routines defined in other source modules of this component.
//

//
// Routines local to this source module.
//

STATIC
VOID
DeleteAutoProvisionSecureBootItems (
  VOID
  )
{
  EFI_STATUS                        Status;
  PDAT_AREA                         *SysArea;
  PDAT_AREA                         *NewArea;
  UINT64                            SecureBootItemFilter;
  UINTN                             AreaSize;
  UINTN                             BlockCount;

  Status = PDatLibGetSystemAreaPointer (TRUE, &SysArea);

  if (Status == EFI_NOT_FOUND) {
    return;  // Allowed not to have any platform data.
  }
  ASSERT_EFI_ERROR (Status);  // Other errors not allowed.
  ASSERT (SysArea != NULL);

  //
  /// Filter out SecureBoot Items from SysArea into NewArea.
  //
  AreaSize = sizeof(PDAT_AREA) + SysArea->Header.Length;
  NewArea = (PDAT_AREA *) AllocateZeroPool (AreaSize + FLASH_BLOCK_SIZE);
  ASSERT (NewArea != NULL);
  SecureBootItemFilter = ((UINT64)
    ((PDAT_ITEM_ID_MASK(PDAT_ITEM_ID_PK)) | (PDAT_ITEM_ID_MASK(PDAT_ITEM_ID_SB_RECORD))
    ));
  Status = PDatLibCopyAreaWithFilterOut (
             NewArea,
             SysArea,
             SecureBootItemFilter
             );

  ASSERT_EFI_ERROR (Status);
  ASSERT (NewArea->Header.Length <= SysArea->Header.Length);

  //
  // Get number of Flash Blocks to Write.
  //
  AreaSize = sizeof(PDAT_AREA) + NewArea->Header.Length;
  BlockCount = (AreaSize / FLASH_BLOCK_SIZE);
  if ((AreaSize % FLASH_BLOCK_SIZE) != 0) {
    BlockCount++;
  }

  //
  // Write NewArea to Flash.
  //
  Status = PlatformFlashEraseWrite (
             NULL,
             (UINTN) PcdGet32(PcdPlatformDataBaseAddress),
             (UINT8 *) NewArea,
             BlockCount * FLASH_BLOCK_SIZE,
             TRUE,
             TRUE
             );
  ASSERT_EFI_ERROR (Status);

  FreePool (NewArea);
}

STATIC
UINT8 *
FindPkCert (
  OUT UINTN                               *CertSizePtr
  )
{
  EFI_STATUS                        Status;
  PDAT_AREA                         *Area;
  PDAT_ITEM                         *PKItem;
  EFI_GUID                          *PkX509File;
  UINT8                             *CertData;

  ASSERT (CertSizePtr != NULL);
  CertData = NULL;

  //
  // Try find Pk certificate in platform data first.
  //
  Status = PDatLibGetSystemAreaPointer (TRUE, &Area);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      DEBUG ((EFI_D_INFO, "AutoProvisionSecureBoot: System Platform Data Area Signature not found.\n"));
    } else {
      DEBUG ((EFI_D_ERROR, "AutoProvisionSecureBoot: System Platform Data Area get failed error = %r.\n", Status));
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    PKItem = PDatLibFindItem (Area, PDAT_ITEM_ID_PK, FALSE, NULL);

    if (PKItem) {
      ASSERT (PKItem->Header.Length > 0);
      *CertSizePtr = PKItem->Header.Length;
      return PKItem->Data;
    }
  }
  DEBUG ((EFI_D_INFO, "AutoProvisionSecureBoot: PK X509 Public Cert not found in PDAT\n"));

  //
  // Finally check to see if cert in Firmware volumes.
  //
  PkX509File = PcdGetPtr(PcdPkX509File);
  if (CompareGuid (&NullGuid, PkX509File) == FALSE) {
    Status = PlatformFindFvFileRawDataSection (
               NULL,
               PkX509File,
               (VOID **) &CertData,
               CertSizePtr
               );

    if (EFI_ERROR (Status)) {
      if (Status != EFI_NOT_FOUND) {
        ASSERT_EFI_ERROR (Status);
        CertData = NULL;
      }
    } else {
      ASSERT (CertData != NULL && *CertSizePtr > 0);
    }
  }

  return CertData;
}

STATIC
VOID
ProvisionFVRecords (
  IN SECUREBOOT_HELPER_PROTOCOL           *SecureBootHelperProtocol
  )
{
  EFI_STATUS                        Status;
  EFI_GUID                          *KekX509File;
  EFI_GUID                          *KekRsa2048File;
  UINT8                             *CertData;
  UINTN                             CertSize;

  DEBUG ((EFI_D_INFO, ">>ProvisionFVRecords\n"));
  KekX509File = PcdGetPtr(PcdKekX509File);
  if (CompareGuid (&NullGuid, KekX509File) == FALSE) {
    Status = PlatformFindFvFileRawDataSection (
               NULL,
               KekX509File,
               (VOID **) &CertData,
               &CertSize
               );

    if (EFI_ERROR (Status)) {
      if (Status != EFI_NOT_FOUND) {
        ASSERT_EFI_ERROR (Status);
      }
    } else {
      ASSERT (CertData != NULL && CertSize > 0);
      DEBUG ((EFI_D_INFO, "Enroll X509 Cert into KEK 0x%04x:%g\n", CertSize, KekX509File));
      Status = SecureBootHelperProtocol->SetupEnrollX509 (
                 SecureBootHelperProtocol,
                 KEKStore,
                 NULL,
                 CertData,
                 CertSize,
                 KekX509File
                 );
      ASSERT_EFI_ERROR (Status);
    }
  }

  KekRsa2048File = PcdGetPtr(PcdKekRsa2048File);
  if (CompareGuid (&NullGuid, KekRsa2048File) == FALSE) {
    Status = PlatformFindFvFileRawDataSection (
               NULL,
               KekRsa2048File,
               (VOID **) &CertData,
               &CertSize
               );

    if (EFI_ERROR (Status)) {
      if (Status != EFI_NOT_FOUND) {
        ASSERT_EFI_ERROR (Status);
      }
    } else {
      ASSERT (CertData != NULL && CertSize > 0);
      DEBUG ((EFI_D_INFO, "Enroll Rsa2048 storing file (*.pbk) into KEK database 0x%04x:%g\n", CertSize, KekRsa2048File));
      Status = SecureBootHelperProtocol->SetupEnrollKekRsa2048 (
                 SecureBootHelperProtocol,
                 NULL,
                 CertData,
                 CertSize,
                 KekRsa2048File
                 );
      ASSERT_EFI_ERROR (Status);
    }
  }
  DEBUG ((EFI_D_INFO, "<<ProvisionFVRecords\n"));
}

STATIC
VOID
ProvisionPlatformDataRecords (
  IN SECUREBOOT_HELPER_PROTOCOL           *SecureBootHelperProtocol
  )
{
  EFI_STATUS                        Status;
  PDAT_ITEM                         *CurrItem;
  PDAT_LIB_FINDCONTEXT              PDatFindContext;
  SECUREBOOT_STORE_TYPE             StoreType;
  EFI_GUID                          *SignatureOwner;
  PDAT_SB_PAYLOAD_HEADER            *SbHeader;
  UINT8                             *CertData;
  UINTN                             CertSize;

  DEBUG ((EFI_D_INFO, ">>ProvisionPlatformDataRecords\n"));

  CurrItem = PDatLibFindFirstWithFilter (
    NULL,
    PDAT_ITEM_ID_MASK (PDAT_ITEM_ID_SB_RECORD),
    &PDatFindContext,
    NULL
    );

  if (CurrItem == NULL) {
    return;  // No records do nothing.
  }
  DEBUG ((EFI_D_INFO, "ProvisionPlatformDataRecords: Items of id '0x%04x' found to provision\n", (UINTN) PDAT_ITEM_ID_SB_RECORD ));
  do {
    ASSERT (CurrItem->Header.Length > (sizeof(SbHeader) + 1));
    SbHeader = (PDAT_SB_PAYLOAD_HEADER *) CurrItem->Data;
    StoreType = (SECUREBOOT_STORE_TYPE) SbHeader->StoreType;
    CertData  = &CurrItem->Data[sizeof(SbHeader)];
    CertSize = CurrItem->Header.Length - sizeof(SbHeader);
    if ((SbHeader->Flags & PDAT_SB_FLAG_HAVE_GUID) != 0) {
      ASSERT (CurrItem->Header.Length > (sizeof(PDAT_SB_PAYLOAD_HEADER) + sizeof(EFI_GUID) + 1));
      SignatureOwner = (EFI_GUID *) CertData;
      CertData += sizeof(EFI_GUID);
      CertSize -= sizeof(EFI_GUID);
    } else {
      SignatureOwner = NULL;
    }
    DEBUG ((
      EFI_D_INFO, "SbRec TotLen '0x%04x' Store '%02x' Type '%02x' Flags '0x%04x' GUID '%g'\n",
      CurrItem->Header.Length,
      StoreType,
      SbHeader->CertType,
      SbHeader->Flags,
      SignatureOwner
      ));

    if (SbHeader->CertType == PDAT_SB_CERT_TYPE_X509) {
      ASSERT ((StoreType == DBStore) || (StoreType == KEKStore) || (StoreType == DBXStore));
      Status = SecureBootHelperProtocol->SetupEnrollX509 (
                 SecureBootHelperProtocol,
                 StoreType,
                 NULL,
                 CertData,
                 CertSize,
                 SignatureOwner
                 );
    } else if (SbHeader->CertType == PDAT_SB_CERT_TYPE_SHA256) {
      ASSERT ((StoreType == DBStore) || (StoreType == DBXStore));
      Status = SecureBootHelperProtocol->SetupEnrollImageSignature (
                 SecureBootHelperProtocol,
                 StoreType,
                 NULL,
                 CertData,
                 CertSize,
                 SignatureOwner
                 );
    } else if (SbHeader->CertType == PDAT_SB_CERT_TYPE_RSA2048) {
      ASSERT (StoreType == KEKStore);
      Status = SecureBootHelperProtocol->SetupEnrollKekRsa2048 (
                 SecureBootHelperProtocol,
                 NULL,
                 CertData,
                 CertSize,
                 SignatureOwner
                 );
    } else {
      DEBUG ((EFI_D_ERROR, "ProvisionPlatformDataRecords: Unsupported Cert Type = '%d'.\n", (UINTN) SbHeader->CertType));
      Status = EFI_UNSUPPORTED;
    }
    ASSERT_EFI_ERROR (Status);

    CurrItem = PDatLibFindNextWithFilter (
      PDAT_ITEM_ID_MASK (PDAT_ITEM_ID_SB_RECORD),
      &PDatFindContext,
      NULL
      );
  } while (CurrItem != NULL);

  DEBUG ((EFI_D_INFO, "<<ProvisionPlatformDataRecords\n"));
}

//
// Routines exported by this source module.
//

/** Auto provision Secure Boot Resources.

  Provision from platform data, and only provision if PK public cert found.

  @retval  TRUE    UEFI 2.3.1 Secure Boot enabled.
  @retval  FALSE   UEFI 2.3.1 Secure Boot Disabled.

**/
BOOLEAN
EFIAPI
PlatformAutoProvisionSecureBoot (
  VOID
  )
{
  EFI_STATUS                        Status;
  SECUREBOOT_HELPER_PROTOCOL        *SecureBootHelperProtocol;
  UINT8                             *PkX509Data;
  UINTN                             PkX509DataSize;
  BOOLEAN                           SecureBootEnabled;

  SecureBootEnabled = FALSE;

  if(FeaturePcdGet (PcdSupportSecureBoot)) {
    Status = gBS->LocateProtocol(
               &gSecureBootHelperProtocolGuid,
               NULL,
               (VOID**) &SecureBootHelperProtocol
               );
    ASSERT_EFI_ERROR (Status);
    SecureBootEnabled = SecureBootHelperProtocol->IsSecureBootEnabled (
                                                    SecureBootHelperProtocol
                                                    );
    if (SecureBootEnabled) {
      DEBUG ((EFI_D_INFO, "AutoProvisionSecureBoot: System provisioned, nothing to do\n"));
    } else {
      PkX509Data = FindPkCert (&PkX509DataSize);
      if (PkX509Data == NULL) {
        DEBUG ((EFI_D_INFO, "AutoProvisionSecureBoot: PK X509 Public Cert not found, do nothing\n"));
      } else {

        //
        // Provision auto provisiong records stored in platform data and FVs.
        //
        ProvisionPlatformDataRecords (SecureBootHelperProtocol);
        ProvisionFVRecords (SecureBootHelperProtocol);

        //
        // Before enrolling PK insure Db & Dbx exist (if they do not exist
        // create a auth. var with a null cert).
        //
        Status = SecureBootHelperProtocol->SetupCreateEmptyDbDbxIfNotExist (
                   SecureBootHelperProtocol
                   );
        ASSERT_EFI_ERROR (Status);

        //
        // Now enable secure boot by enrolling PK public cert.
        //
        Status = SecureBootHelperProtocol->SetupEnrollX509 (
                   SecureBootHelperProtocol,
                   PKStore,
                   NULL,
                   PkX509Data,
                   PkX509DataSize,
                   NULL
                   );
        ASSERT_EFI_ERROR (Status);

        SecureBootEnabled = SecureBootHelperProtocol->IsSecureBootEnabled (
                                                        SecureBootHelperProtocol
                                                        );
        ASSERT (SecureBootEnabled);

        PlatformValidateSecureBootVars ();

        DeleteAutoProvisionSecureBootItems ();
      }
    }
  }
  return SecureBootEnabled;
}

/** Validate Secure boot variables setup correctly for this platform.

  ASSERT if problem found.
  Only validate if PcdSupportSecureBoot AND SecureBootEnabled.

**/
VOID
EFIAPI
PlatformValidateSecureBootVars (
  VOID
  )
{
  EFI_STATUS                        Status;
  SECUREBOOT_VARIABLE_STATS         Stats;
  BOOLEAN                           SecureBootEnabled;
  SECUREBOOT_HELPER_PROTOCOL        *SecureBootHelperProtocol;

  //
  // Only validate if PcdSupportSecureBoot AND SecureBootEnabled.
  //
  if(FeaturePcdGet (PcdSupportSecureBoot)) {
    Status = gBS->LocateProtocol(
               &gSecureBootHelperProtocolGuid,
               NULL,
               (VOID**) &SecureBootHelperProtocol
               );
    ASSERT_EFI_ERROR (Status);
    SecureBootEnabled = SecureBootHelperProtocol->IsSecureBootEnabled (
                                                    SecureBootHelperProtocol
                                                    );
    if (SecureBootEnabled) {
      Status = SecureBootHelperProtocol->GetSecureBootVarStats (
                                           SecureBootHelperProtocol,
                                           &Stats
                                           );
      ASSERT_EFI_ERROR (Status);

      //
      // For this platform all cert vars need to be enrolled with
      // EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute and
      // KEK must have a X509 or Rsa2048 cert.
      //
      DEBUG (
        (EFI_D_INFO, "ValidateSecureBoot: Enrolled pk:kek:db:dbx %d:%d:%d:%d\n",
        Stats.PkEnrolled,
        Stats.KekEnrolled,
        Stats.DbEnrolled,
        Stats.DbxEnrolled
        ));
      ASSERT (Stats.PkEnrolled && Stats.KekEnrolled && Stats.DbEnrolled && Stats.DbxEnrolled);
      DEBUG (
        (EFI_D_INFO, "ValidateSecureBoot: Attributes pk:kek:db:dbx 0x%08x:0x%08x:0x%08x:0x%08x\n",
        Stats.PkAttributes,
        Stats.KekAttributes,
        Stats.DbAttributes,
        Stats.DbxAttributes
        ));
      ASSERT (
        ((Stats.PkAttributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) &&
        ((Stats.KekAttributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) &&
        ((Stats.DbAttributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) &&
        ((Stats.DbxAttributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0)
        );
      DEBUG (
        (EFI_D_INFO, "ValidateSecureBoot: KekCertCounts X509:Rsa2048 %d:%d\n",
        Stats.KekX509CertCount,
        Stats.KekRsa2048CertCount
        ));
      ASSERT (Stats.KekX509CertCount > 0 || Stats.KekRsa2048CertCount > 0);
    }
  }
}
