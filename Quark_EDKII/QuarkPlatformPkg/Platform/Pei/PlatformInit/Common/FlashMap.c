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

  FlashMap.c
   
Abstract:

  Build GUIDed HOBs for platform specific flash map.

--*/
#include <FlashLayout.h>

#include <Ppi/FlashMap.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Guid/FlashMapHob.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/SystemNvDataHobGuid.h>
#include <Guid/FirmwareFileSystem2.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PlatformHelperLib.h>

EFI_STATUS
EFIAPI
GetAreaInfo (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN PEI_FLASH_MAP_PPI            * This,
  IN  EFI_FLASH_AREA_TYPE         AreaType,
  IN  EFI_GUID                    *AreaTypeGuid,
  OUT UINT32                      *NumEntries,
  OUT EFI_FLASH_SUBAREA_ENTRY     **Entries
  );

EFI_GUID                            mFvBlockGuid      = EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID;

EFI_GUID                            mFfsGuid          = EFI_FIRMWARE_FILE_SYSTEM2_GUID;

EFI_GUID                            mSystemDataGuid   = EFI_SYSTEM_NV_DATA_HOB_GUID;

STATIC PEI_FLASH_MAP_PPI            mFlashMapPpi = { GetAreaInfo };

STATIC EFI_PEI_PPI_DESCRIPTOR       mPpiListFlashMap = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiFlashMapPpiGuid,
  &mFlashMapPpi
};

static EFI_FLASH_AREA_DATA          mFlashAreaData[]  = { EFI_FLASH_AREA_DATA_DEFINITION };

#define NUM_FLASH_AREA_DATA (sizeof (mFlashAreaData) / sizeof (mFlashAreaData[0]))

static EFI_HOB_FLASH_MAP_ENTRY_TYPE mFlashMapHobData[] = { EFI_HOB_FLASH_MAP_ENTRY_TYPE_DATA_DEFINITION };

#define NUM_HOB_FLASH_MAP_DATA  (sizeof (mFlashMapHobData) / sizeof (mFlashMapHobData[0]))

EFI_STATUS
PeimInstallFlashMapPpi (
  IN EFI_PEI_FILE_HANDLE       *FileHandle,
  IN CONST EFI_PEI_SERVICES           **PeiServices
  )
{
  EFI_STATUS                    Status;
  //
  // Install FlashMap PPI
  //
  Status = PeiServicesInstallPpi (&mPpiListFlashMap);
  ASSERT_EFI_ERROR (Status);  

  DEBUG ((EFI_D_INFO, "Flash Map PEIM Loaded\n"));
  return EFI_SUCCESS;
}  

EFI_STATUS
PeimInitializeFlashMap (
  IN EFI_PEI_FILE_HANDLE       *FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  Build GUID HOBs for platform specific flash map.
  
Arguments:

  FfsHeader     Pointer this FFS file header.
  PeiServices   General purpose services available to every PEIM.
    
Returns:

  EFI_SUCCESS   Guid HOBs for platform flash map is built.
  Otherwise     Failed to build the Guid HOB data.

--*/
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  EFI_FLASH_AREA_HOB_DATA       FlashHobData;
  EFI_HOB_FLASH_MAP_ENTRY_TYPE  *Hob;

  //
  // Build flash area entries as GUIDed HOBs.
  //

  for (Index = 0; Index < NUM_FLASH_AREA_DATA; Index++) {
    SetMem (&FlashHobData, sizeof (EFI_FLASH_AREA_HOB_DATA), 0);

    FlashHobData.AreaType               = mFlashAreaData[Index].AreaType;
    FlashHobData.NumberOfEntries        = 1;
    FlashHobData.SubAreaData.Attributes = mFlashAreaData[Index].Attributes;

    //
    // Default to table values, maybe overridden in following switch statement.
    //
    FlashHobData.SubAreaData.Base       = (EFI_PHYSICAL_ADDRESS) (UINTN) mFlashAreaData[Index].Base;
    FlashHobData.SubAreaData.Length     = (EFI_PHYSICAL_ADDRESS) (UINTN) mFlashAreaData[Index].Length;

    switch (FlashHobData.AreaType) {
    case EFI_FLASH_AREA_RECOVERY_BIOS:
    case EFI_FLASH_AREA_MAIN_BIOS:
      if ((FlashHobData.AreaType == EFI_FLASH_AREA_RECOVERY_BIOS)) {
        //
        // Setup Stage1 which comes from Dynamic PCDs.
        //
        FlashHobData.SubAreaData.Base       = (EFI_PHYSICAL_ADDRESS) FLASH_REGION_FV_SECPEI_NORMAL_BASE;
        FlashHobData.SubAreaData.Length     = (EFI_PHYSICAL_ADDRESS) FLASH_REGION_FV_SECPEI_NORMAL_SIZE;
      } else {
        //
        // Setup FvMain which comes from Dynamic PCDs.
        //
        FlashHobData.SubAreaData.Base       = (EFI_PHYSICAL_ADDRESS) FLASH_REGION_FVMAIN_BASE;
        FlashHobData.SubAreaData.Length     = (EFI_PHYSICAL_ADDRESS) FLASH_REGION_FVMAIN_SIZE;
      }
      if (PlatformIsBootWithRecoveryStage1 ()) {
        DEBUG (
          (DEBUG_INFO, "FlashMap - Do not create FV Hobs for FV:0x%08x since recovery boot.\n",
          FlashHobData.SubAreaData.Base
          ));
        continue;
      }

      CopyMem (
        &FlashHobData.AreaTypeGuid,
        &mFfsGuid,
        sizeof (EFI_GUID)
        );
      CopyMem (
        &FlashHobData.SubAreaData.FileSystem,
        &mFvBlockGuid,
        sizeof (EFI_GUID)
        );
      break;

    case EFI_FLASH_AREA_GUID_DEFINED:
      CopyMem (
        &FlashHobData.AreaTypeGuid,
        &mSystemDataGuid,
        sizeof (EFI_GUID)
        );
      CopyMem (
        &FlashHobData.SubAreaData.FileSystem,
        &mFvBlockGuid,
        sizeof (EFI_GUID)
        );
      break;

    default:
      break;
    }

    Hob = BuildGuidDataHob (
            &gEfiFlashMapHobGuid,
            &FlashHobData,
            sizeof (EFI_FLASH_AREA_HOB_DATA)
            );
    ASSERT (Hob);
  }

  for (Index = 0; Index < NUM_HOB_FLASH_MAP_DATA; Index++) {
    Status = PeiServicesCreateHob (
              EFI_HOB_TYPE_GUID_EXTENSION,
              (UINT16) (sizeof (EFI_HOB_FLASH_MAP_ENTRY_TYPE)),
              &Hob
              );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    CopyMem (
      Hob,
      &mFlashMapHobData[Index],
      sizeof (EFI_HOB_FLASH_MAP_ENTRY_TYPE)
      );
    if (mFlashMapHobData[Index].AreaType == EFI_FLASH_AREA_EFI_VARIABLES) {
      Hob->Entries[0].Base    = Hob->Entries[0].Base + EFI_RUNTIME_UPDATABLE_FV_HEADER_LENGTH;
      Hob->Entries[0].Length  = Hob->Entries[0].Length - EFI_RUNTIME_UPDATABLE_FV_HEADER_LENGTH;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetAreaInfo (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN PEI_FLASH_MAP_PPI            * This,
  IN  EFI_FLASH_AREA_TYPE         AreaType,
  IN  EFI_GUID                    *AreaTypeGuid,
  OUT UINT32                      *NumEntries,
  OUT EFI_FLASH_SUBAREA_ENTRY     **Entries
  )
/*++

Routine Description:

  Get data from the platform specific flash area map.
  
Arguments:

  PeiServices   General purpose services available to every PEIM.
  AreaType      Flash map area type.
  AreaTypeGuid  Guid for the flash map area type.
  NumEntries    On return, filled with the number of sub-areas with the same type.
  Entries       On return, filled with entry pointer to the sub-areas.
    
Returns:

  EFI_SUCCESS   The type of area exists in the flash map and data is returned.
  EFI_NOT_FOUND The type of area does not exist in the flash map.

--*/
{
  EFI_PEI_HOB_POINTERS          Hob;
  EFI_HOB_FLASH_MAP_ENTRY_TYPE  *FlashMapEntry;

  Hob.Raw = GetHobList ();
  while (Hob.Raw != NULL) {
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_GUID_EXTENSION, Hob.Raw);
    if (CompareGuid (&Hob.Guid->Name, &gEfiFlashMapHobGuid)) {
      FlashMapEntry = (EFI_HOB_FLASH_MAP_ENTRY_TYPE *) Hob.Raw;
      ASSERT (FlashMapEntry != NULL);
      if ((AreaType != FlashMapEntry->AreaType) ||
          ((AreaType == EFI_FLASH_AREA_GUID_DEFINED) && !CompareGuid (AreaTypeGuid, &FlashMapEntry->AreaTypeGuid))
          ) {
        continue;
      }

      *NumEntries = FlashMapEntry->NumEntries;
      *Entries    = FlashMapEntry->Entries;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
