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

  SMIFlashDxe.h

Abstract:

 SMI driver to update BIOS capsule.

--*/

#ifndef _SMI_FLASH_DXE_H_
#define _SMI_FLASH_DXE_H_

#include <PiSmm.h>
#include <Guid/QuarkCapsuleGuid.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/Spi.h>
#include <Protocol/SmmFirmwareVolumeBlock.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/PlatformHelperLib.h>
#include <Library/PlatformDataLib.h>

#include <FlashLayout.h>

#include "AccessViolationHandler.h"

#define SMI_FLASH_UPDATE_MAX_STATES		5
#define MAX_IMAGES_PER_CAPSULE        20

//
// All the legacy SPI flash images that the SPI update mechanism is aware of.
// The update mechansim will only consider images in the below list
//
#define EDKII_BOOTROM_OVERRIDE_DEFAULT_BASE_ADDRESS     0xFFFE0000
#define EDKII_KEY_MODULE_DEFAULT_BASE_ADDRESS           0xFFFD8000
#define EDKII_SVN_DEFAULT_BASE_ADDRESS                  0xFFFD0000
#define EDKII_RECOVERY_IMAGE1_DEFAULT_BASE_ADDRESS      FixedPcdGet32 (PcdFlashFvFixedStage1AreaBase)
#define EDKII_OEM_AREA_DEFAULT_BASE_ADDRESS             0xFFF50000
#define EDKII_NVRAM_DEFAULT_BASE_ADDRESS                FLASH_REGION_BIOS_NV_STORAGE_BASE
#define EDKII_PLATFORM_DATA_DEFAULT_BASE_ADDRESS        FixedPcdGet32 (PcdPlatformDataBaseAddress)
#define EDKII_MFH_DEFAULT_BASE_ADDRESS                  FixedPcdGet32 (PcdFlashNvMfh)
#define EDKII_RMU_DEFAULT_BASE_ADDRESS                  0xFFF00000
#define EDKII_BOOT_STAGE1_IMAGE1_DEFAULT_BASE_ADDRESS   0xFFEC0000
#define EDKII_BOOT_STAGE1_IMAGE2_DEFAULT_BASE_ADDRESS   0xFFE80000
#define EDKII_BOOT_STAGE2_COMPACT_DEFAULT_BASE_ADDRESS  0xFFD00000
#define EDKII_BIOS_SECTION_DEFAULT_BASE_ADDRESS         0xFFD00000
#define EDKII_OS_DEFAULT_BASE_ADDRESS                   0xFF800000

#define EDKII_BOOTROM_OVERRIDE_MAX_SIZE                 0x00020000    // 128K
#define EDKII_KEY_MODULE_MAX_SIZE                       0x00008000    // 32K
#define EDKII_SVN_MAX_SIZE                              0x00008000    // 32K
#define EDKII_RECOVERY_IMAGE1_MAX_SIZE                  FLASH_REGION_FV_SECPEI_RECOVERY_SIZE
#define EDKII_OEM_AREA_MAX_SIZE                         0x00040000    // 256K
#define EDKII_NVRAM_MAX_SIZE                            FLASH_REGION_BIOS_NV_STORAGE_SIZE
#define EDKII_PLATFORM_DATA_MAX_SIZE                    (FixedPcdGet32 (PcdPlatformDataMaxLen))
#define EDKII_MFH_MAX_SIZE                              0x00008000    // 32K
#define EDKII_RMU_MAX_SIZE                              0x00008000    // 32K
#define EDKII_BOOT_STAGE1_IMAGE1_MAX_SIZE               0x00040000    // 256K
#define EDKII_BOOT_STAGE1_IMAGE2_MAX_SIZE               0x00040000    // 256K
#define EDKII_BOOT_STAGE2_COMPACT_MAX_SIZE              0x00180000    // 1.5M
#define EDKII_BIOS_SECTION_MAX_SIZE                     0x00300000    // 3M
#define EDKII_OS_MAX_SIZE                               0x00500000    // 5M

#define SPI_FLASH_4M                                    0x400000
#define SPI_FLASH_8M                                    0x800000

//
// Capsule Header Flags defined by Capsule GUID
//
#define PD_UPDATE_MAC                     0x00000001

#define SPI_ERASE_SECTOR_SIZE            FLASH_BLOCK_SIZE //This is the chipset requirement

#pragma pack(1)

typedef struct {
  UINT32	    SpiImageDefaultBaseAddress;
  UINT32	    SpiImageMaxSize;
  BOOLEAN	    FixedAddress;
  BOOLEAN	    Updatable;
  BOOLEAN	    AnyBlockUpdate;
  BOOLEAN	    Present;
} SPI_IMAGE_PRESENT;

//
// Periodic SMI functionality
//
typedef
EFI_STATUS
(EFIAPI *NEXT_STATE_POINTER) (
  VOID
  );

typedef struct _NEXT_STATE {
  NEXT_STATE_POINTER     NextStatePointer;
} NEXT_STATE;

typedef enum {
  SMI_FLASH_UPDATE_START,
  SMI_FLASH_UPDATE_IN_PROGRESS,
  SMI_FLASH_UPDATE_STOP
} SMI_FLASH_STATE_MANAGER;

typedef struct {
  UINT32    TargetAddr;
  UINT32    Size;
  UINT32    SourceOffset;
  UINT32    Reserved;
} UPDATE_HINT;

typedef struct {
  EFI_SPI_PROTOCOL      *SmmSpiProtocol;  
  UINT32    SourceAddr;
  UINT32    TargetAddr;
  UINTN     BlockSize;
  BOOLEAN   IsValid;
  BOOLEAN   UpdateStatus;
} STATE_MACHINE_PRIVATE_DATA;

typedef enum {
  CHALLENGE_CAPSULE_STATE = 0,	// 0
  PARSE_CAPSULE_STATE,		// 1
  ERASE_BLOCK_STATE,		  // 2
  WRITE_BLOCK_STATE		    // 3
} SMI_FLASH_STATE_ENUM;		// 4

#pragma pack()

#endif
