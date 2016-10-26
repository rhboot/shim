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

  PlatformHelperPei.c

Abstract:

  Implementation of Helper routines for PEI enviroment.

--*/

#include <PiPei.h>

#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>

#include "CommonHeader.h"

//
// Routines defined in other source modules of this component.
//

//
// Routines local to this source module.
//

//
// Routines exported by this source module.
//

/**
  Find pointer to RAW data in Firmware volume file.

  @param   FvNameGuid       Firmware volume to search. If == NULL search all.
  @param   FileNameGuid     Firmware volume file to search for.
  @param   SectionData      Pointer to RAW data section of found file.
  @param   SectionDataSize  Pointer to UNITN to get size of RAW data.

  @retval  EFI_SUCCESS            Raw Data found.
  @retval  EFI_INVALID_PARAMETER  FileNameGuid == NULL.
  @retval  EFI_NOT_FOUND          Firmware volume file not found.
  @retval  EFI_UNSUPPORTED        Unsupported in current enviroment (PEI or DXE).

**/
EFI_STATUS
EFIAPI
PlatformFindFvFileRawDataSection (
  IN CONST EFI_GUID                 *FvNameGuid OPTIONAL,
  IN CONST EFI_GUID                 *FileNameGuid,
  OUT VOID                          **SectionData,
  OUT UINTN                         *SectionDataSize
  )
{
  EFI_STATUS                        Status;
  UINTN                             Instance;
  EFI_PEI_FV_HANDLE                 VolumeHandle;
  EFI_PEI_FILE_HANDLE               FileHandle;
  EFI_SECTION_TYPE                  SearchType;
  EFI_FV_INFO                       VolumeInfo;
  EFI_FV_FILE_INFO                  FileInfo;
  CONST EFI_PEI_SERVICES            **PeiServices;

  if (FileNameGuid == NULL || SectionData == NULL || SectionDataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *SectionData = NULL;
  *SectionDataSize = 0;

  PeiServices = GetPeiServicesTablePointer ();
  SearchType = EFI_SECTION_RAW;
  for (Instance = 0; !EFI_ERROR(((*PeiServices)->FfsFindNextVolume (PeiServices, Instance, &VolumeHandle))); Instance++) {
    if (FvNameGuid != NULL) {
      Status = (*PeiServices)->FfsGetVolumeInfo (VolumeHandle, &VolumeInfo);
      if (EFI_ERROR (Status)) {
        continue;
      }
      if (!CompareGuid (FvNameGuid, &VolumeInfo.FvName)) {
        continue;
      }
    }
    Status = (*PeiServices)->FfsFindFileByName (FileNameGuid, VolumeHandle, &FileHandle);
    if (!EFI_ERROR (Status)) {
      Status = (*PeiServices)->FfsGetFileInfo (FileHandle, &FileInfo);
      if (EFI_ERROR (Status)) {
        continue;
      }
      if (IS_SECTION2(FileInfo.Buffer)) {
        *SectionDataSize = SECTION2_SIZE(FileInfo.Buffer) - sizeof(EFI_COMMON_SECTION_HEADER2);
      } else {
        *SectionDataSize = SECTION_SIZE(FileInfo.Buffer) - sizeof(EFI_COMMON_SECTION_HEADER);
      }
      Status = (*PeiServices)->FfsFindSectionData (PeiServices, SearchType, FileHandle, SectionData);
      if (!EFI_ERROR (Status)) {
        return Status;
      }
    }
  }
  return EFI_NOT_FOUND;
}

/**
  Find free spi protect register and write to it to protect a flash region.

  @param   DirectValue      Value to directly write to register.
                            if DirectValue == 0 the use Base & Length below.
  @param   BaseAddress      Base address of region in Flash Memory Map.
  @param   Length           Length of region to protect.

  @retval  EFI_SUCCESS      Free spi protect register found & written.
  @retval  EFI_NOT_FOUND    Free Spi protect register not found.
  @retval  EFI_DEVICE_ERROR Unable to write to spi protect register.
**/
EFI_STATUS
EFIAPI
PlatformWriteFirstFreeSpiProtect (
  IN CONST UINT32                         DirectValue,
  IN CONST UINT32                         BaseAddress,
  IN CONST UINT32                         Length
  )
{
  return WriteFirstFreeSpiProtect (
           QNC_RCRB_BASE,
           DirectValue,
           BaseAddress,
           Length,
           NULL
           );
}

/**
  Lock legacy SPI static configuration information.

  Function will assert if unable to lock config.

**/
VOID
EFIAPI
PlatformFlashLockConfig (
  VOID
  )
{
  ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
}

/**
  Lock regions and config of SPI flash given the policy for this platform.

  Function will assert if unable to lock regions or config.

  @param   PreBootPolicy    If TRUE do Pre Boot Flash Lock Policy.

**/
VOID
EFIAPI
PlatformFlashLockPolicy (
  IN CONST BOOLEAN                        PreBootPolicy
  )
{
  ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
}

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
  ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
  return FALSE;
}

/** Check if System booted with recovery Boot Stage1 image.

  @retval  TRUE    If system booted with recovery Boot Stage1 image.
  @retval  FALSE   If system booted with normal stage1 image.

**/
BOOLEAN
EFIAPI
PlatformIsBootWithRecoveryStage1 (
  VOID
  )
{
  BOOLEAN                           IsRecoveryBoot;
  QUARK_EDKII_STAGE1_HEADER         *Edk2ImageHeader;

  Edk2ImageHeader = (QUARK_EDKII_STAGE1_HEADER *) (PcdGet32 (PcdEsramStage1Base) + PcdGet32 (PcdFvSecurityHeaderSize));
  switch ((UINT8)Edk2ImageHeader->ImageIndex & QUARK_STAGE1_IMAGE_TYPE_MASK) {
  case QUARK_STAGE1_RECOVERY_IMAGE_TYPE:
    //
    // Recovery Boot
    //
    IsRecoveryBoot = TRUE;
    break;
  default:
    //
    // Normal Boot
    //
    IsRecoveryBoot = FALSE;
    break;
  }

  return IsRecoveryBoot;
}
