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

  FvbInfo.c

Abstract:

  Defines data structure that is the volume header found.These data is intent
  to decouple FVB driver with FV header.

--*/

#include <PiDxe.h>
#include "FwBlockService.h"

//
// Signatures to trigger fixup.
//
#define PLATFORM_FVBOOT_SIGNATURE         SIGNATURE_32 ('P', 'F', 'V', 'B')
#define PLATFORM_FVMAIN_SIGNATURE         SIGNATURE_32 ('P', 'F', 'V', 'M')

EFI_FVB_MEDIA_INFO  mPlatformFvbMediaInfo[] = {
  //
  // SECPEI BIOS FVB
  //
  {
    0,
#if (PI_SPECIFICATION_VERSION < 0x00010000)    
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_FIRMWARE_FILE_SYSTEM_GUID,
      0,
      PLATFORM_FVBOOT_SIGNATURE,
      EFI_FVB_READ_ENABLED_CAP | EFI_FVB_READ_STATUS | EFI_FVB_WRITE_ENABLED_CAP | EFI_FVB_WRITE_STATUS,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[3]
      1,  // Revision
      {
        0,
        0,
      }
    },
#else
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_FIRMWARE_FILE_SYSTEM2_GUID,
      0,
      PLATFORM_FVBOOT_SIGNATURE,
      EFI_FVB_READ_ENABLED_CAP | EFI_FVB_READ_STATUS | EFI_FVB_WRITE_ENABLED_CAP | EFI_FVB_WRITE_STATUS,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[3]
      2,  // Revision
      {
        0,
        0,
      }
    },
#endif
    {
      0,
      0
    }
  },
  //
  // RUNTIME UPDATABLE (NV) FV
  //
  {
    FLASH_REGION_RUNTIME_UPDATABLE_BASE,
#if (PI_SPECIFICATION_VERSION < 0x00010000)    
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_SYSTEM_NV_DATA_FV_GUID,
      (FLASH_REGION_RUNTIME_UPDATABLE_SIZE + FLASH_REGION_NV_FTW_SPARE_SIZE),
      EFI_FVH_SIGNATURE,
      EFI_FVB_READ_ENABLED_CAP | EFI_FVB_READ_STATUS | EFI_FVB_WRITE_ENABLED_CAP | EFI_FVB_WRITE_STATUS,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[3]
      1,  // Revision
      {
        ((FLASH_REGION_RUNTIME_UPDATABLE_SIZE + FLASH_REGION_NV_FTW_SPARE_SIZE) / FLASH_BLOCK_SIZE),
        FLASH_BLOCK_SIZE,
      }
    },
#else
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_SYSTEM_NV_DATA_FV_GUID,
      (FLASH_REGION_RUNTIME_UPDATABLE_SIZE + FLASH_REGION_NV_FTW_SPARE_SIZE),
      EFI_FVH_SIGNATURE,
      EFI_FVB_READ_ENABLED_CAP | EFI_FVB_READ_STATUS | EFI_FVB_WRITE_ENABLED_CAP | EFI_FVB_WRITE_STATUS,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[3]
      2,  // Revision
      {
        ((FLASH_REGION_RUNTIME_UPDATABLE_SIZE + FLASH_REGION_NV_FTW_SPARE_SIZE) / FLASH_BLOCK_SIZE),
        FLASH_BLOCK_SIZE,
      }
    },
#endif    
    {
      0,
      0
    }
  },
  //
  // Main BIOS FVB, Dynamic Base/Size values, fixed up at run time.
  //
  {
    0,
#if (PI_SPECIFICATION_VERSION < 0x00010000)    
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_FIRMWARE_FILE_SYSTEM_GUID,
      0,
      PLATFORM_FVMAIN_SIGNATURE,
      EFI_FVB_READ_ENABLED_CAP | EFI_FVB_READ_STATUS | EFI_FVB_WRITE_ENABLED_CAP | EFI_FVB_WRITE_STATUS,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[3]
      1,  // Revision
      {
        0,
        0,
      }
    },
#else
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_FIRMWARE_FILE_SYSTEM2_GUID,
      0,
      PLATFORM_FVMAIN_SIGNATURE,
      EFI_FVB_READ_ENABLED_CAP | EFI_FVB_READ_STATUS | EFI_FVB_WRITE_ENABLED_CAP | EFI_FVB_WRITE_STATUS,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[3]
      2,  // Revision
      {
        0,
        0,
      }
    },
#endif
    {
      0,
      0
    }
  },
  //
  // End of the FV structure
  //
};

EFI_STATUS
GetFvbInfo (
  IN  EFI_PHYSICAL_ADDRESS         FvBaseAddress,
  OUT EFI_FIRMWARE_VOLUME_HEADER   **FvbInfo
  )
{
  UINTN Index;
  EFI_BOOT_MODE        BootMode;

  BootMode = GetBootModeHob ();

  for (Index = 0; Index < sizeof (mPlatformFvbMediaInfo) / sizeof (EFI_FVB_MEDIA_INFO); Index += 1) {

    //
    // Fixup for dynamic values.
    //
    if (mPlatformFvbMediaInfo[Index].FvbInfo.Signature == PLATFORM_FVBOOT_SIGNATURE) {
      mPlatformFvbMediaInfo[Index].BaseAddress = FLASH_REGION_FV_SECPEI_BASE;
      mPlatformFvbMediaInfo[Index].FvbInfo.FvLength = (UINT64) FLASH_REGION_FV_SECPEI_SIZE;
      mPlatformFvbMediaInfo[Index].FvbInfo.BlockMap[0].NumBlocks = (UINT64) (FLASH_REGION_FV_SECPEI_SIZE / FLASH_BLOCK_SIZE);
      mPlatformFvbMediaInfo[Index].FvbInfo.BlockMap[0].Length = FLASH_BLOCK_SIZE;
      mPlatformFvbMediaInfo[Index].FvbInfo.Signature = EFI_FVH_SIGNATURE;  // Only need to do it once.
    }
    if (mPlatformFvbMediaInfo[Index].FvbInfo.Signature == PLATFORM_FVMAIN_SIGNATURE) {
      mPlatformFvbMediaInfo[Index].BaseAddress = FLASH_REGION_FVMAIN_BASE;
      mPlatformFvbMediaInfo[Index].FvbInfo.FvLength = (UINT64) FLASH_REGION_FVMAIN_SIZE;
      mPlatformFvbMediaInfo[Index].FvbInfo.BlockMap[0].NumBlocks = (UINT64) (FLASH_REGION_FVMAIN_SIZE / FLASH_BLOCK_SIZE);
      mPlatformFvbMediaInfo[Index].FvbInfo.BlockMap[0].Length = FLASH_BLOCK_SIZE;
      mPlatformFvbMediaInfo[Index].FvbInfo.Signature = EFI_FVH_SIGNATURE;  // Only need to do it once.
    }

    if (mPlatformFvbMediaInfo[Index].BaseAddress == FvBaseAddress) {
      *FvbInfo = &mPlatformFvbMediaInfo[Index].FvbInfo;

      DEBUG ((DEBUG_ERROR, "\nBaseAddr: 0x%lx \n", FvBaseAddress));
      DEBUG ((DEBUG_ERROR, "FvLength: 0x%lx \n", (*FvbInfo)->FvLength));
      DEBUG ((DEBUG_ERROR, "HeaderLength: 0x%x \n", (*FvbInfo)->HeaderLength));
      DEBUG ((DEBUG_ERROR, "FvBlockMap[0].NumBlocks: 0x%x \n", (*FvbInfo)->BlockMap[0].NumBlocks));
      DEBUG ((DEBUG_ERROR, "FvBlockMap[0].Length: 0x%x \n", (*FvbInfo)->BlockMap[0].Length));
      DEBUG ((DEBUG_ERROR, "FvBlockMap[1].NumBlocks: 0x%x \n", (*FvbInfo)->BlockMap[1].NumBlocks));
      DEBUG ((DEBUG_ERROR, "FvBlockMap[1].Length: 0x%x \n\n", (*FvbInfo)->BlockMap[1].Length));

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
