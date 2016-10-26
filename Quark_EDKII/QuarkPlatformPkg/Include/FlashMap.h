//
// Copyright (c) 2013 Intel Corporation.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in
// the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Intel Corporation nor the names of its
// contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef _FLASH_MAP_H_
#define _FLASH_MAP_H_

#define FLASH_REGION_BOOTROM_OVERRIDE_MAX_SIZE              0x00020000    // 128K
#define FLASH_REGION_BOOTROM_OVERRIDE_BASE                  0xFFFE0000

//
// #defines based on Fixed PCDs.
//

#define FLASH_BASE                                          (FixedPcdGet32 (PcdFlashAreaBaseAddress))
#define FLASH_SIZE                                          (FixedPcdGet32 (PcdFlashAreaSize))

#define FLASH_REGION_FV_SECPEI_RECOVERY_BASE                (FixedPcdGet32 (PcdFlashFvFixedStage1AreaBase) + FixedPcdGet32 (PcdFvSecurityHeaderSize))
#define FLASH_REGION_FV_SECPEI_RECOVERY_SIZE                (FixedPcdGet32 (PcdFlashFvFixedStage1AreaSize))
#define FLASH_REGION_FV_SECPEI_RECOVERY_OFFSET              (FLASH_REGION_FV_SECPEI_RECOVERY_BASE - FLASH_BASE)

#define FLASH_REGION_BIOS_NV_STORAGE_BASE                   (FixedPcdGet32 (PcdFlashNvStorageBase))
#define FLASH_REGION_BIOS_NV_STORAGE_SIZE                   (FixedPcdGet32 (PcdFlashNvStorageSize))

//
// Hard coded defines.
//

#define FLASH_REGION_RUNTIME_UPDATABLE_BASE                 (FixedPcdGet32 (PcdFlashNvStorageBase))
#define FLASH_REGION_RUNTIME_UPDATABLE_SIZE                 0x00010000
#define FLASH_REGION_RUNTIME_UPDATABLE_OFFSET               (FLASH_REGION_RUNTIME_UPDATABLE_BASE - FixedPcdGet32 (PcdFlashAreaBaseAddress))

#define FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_VARIABLE_STORE_BASE      FLASH_REGION_RUNTIME_UPDATABLE_BASE
#define FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_VARIABLE_STORE_SIZE      0x0000E000
#define FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_VARIABLE_STORE_OFFSET    FLASH_REGION_RUNTIME_UPDATABLE_OFFSET

#define FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_FTW_WORKING_BASE         (FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_VARIABLE_STORE_BASE + FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_VARIABLE_STORE_SIZE)
#define FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_FTW_WORKING_SIZE         0x00002000
#define FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_FTW_WORKING_OFFSET       (FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_VARIABLE_STORE_OFFSET + FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_VARIABLE_STORE_SIZE)

#define FLASH_REGION_NV_FTW_SPARE_BASE                      (FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_FTW_WORKING_BASE + FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_FTW_WORKING_SIZE)
#define FLASH_REGION_NV_FTW_SPARE_SIZE                      0x00010000
#define FLASH_REGION_NV_FTW_SPARE_OFFSET                    (FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_FTW_WORKING_OFFSET + FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_FTW_WORKING_SIZE)

#define FLASH_REGION_OEM_NV_STORAGE_BASE                    (FLASH_REGION_BIOS_NV_STORAGE_BASE + FLASH_REGION_BIOS_NV_STORAGE_SIZE)
#define FLASH_REGION_OEM_NV_STORAGE_SIZE                    0x00000000

//
// #defines based on dynamic PCDs.
// These PCDs should only be accessed after PlatformConfigPei has set them up.
//

#define FLASH_REGION_FVMAIN_BASE                            (PcdGet32 (PcdFlashFvMainBase))
#define FLASH_REGION_FVMAIN_SIZE                            (PcdGet32 (PcdFlashFvMainSize))
#define FLASH_REGION_FVMAIN_OFFSET                          (FLASH_REGION_FVMAIN_BASE - FLASH_BASE)

#define FLASH_REGION_FV_SECPEI_NORMAL_BASE                  (PcdGet32 (PcdFlashFvRecoveryBase))
#define FLASH_REGION_FV_SECPEI_NORMAL_SIZE                  (PcdGet32 (PcdFlashFvRecoverySize))
#define FLASH_REGION_FV_SECPEI_NORMAL_OFFSET                (FLASH_REGION_FV_SECPEI_NORMAL_BASE - FLASH_BASE)

#define FLASH_REGION_FV_SECPEI_BASE                         FLASH_REGION_FV_SECPEI_NORMAL_BASE
#define FLASH_REGION_FV_SECPEI_SIZE                         FLASH_REGION_FV_SECPEI_NORMAL_SIZE
#define FLASH_REGION_FV_SECPEI_OFFSET                       FLASH_REGION_FV_SECPEI_NORMAL_OFFSET

#define EFI_FLASH_AREA_DATA_DEFINITION \
  /* FVMAIN region - Base/Size are Dynamic so determined at run time.*/\
  {\
    0,\
    0,\
    EFI_FLASH_AREA_FV | EFI_FLASH_AREA_MEMMAPPED_FV,\
    EFI_FLASH_AREA_MAIN_BIOS,\
    0, 0, 0,\
    { 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },\
  },\
  /* RUNTIME_UPDATABLE region */\
  {\
    FLASH_REGION_RUNTIME_UPDATABLE_BASE,\
    FLASH_REGION_RUNTIME_UPDATABLE_SIZE,\
    EFI_FLASH_AREA_FV | EFI_FLASH_AREA_MEMMAPPED_FV,\
    EFI_FLASH_AREA_GUID_DEFINED,\
    0, 0, 0,\
    EFI_SYSTEM_NV_DATA_HOB_GUID, \
  },\
  /* NV_FTW_SPARE region */\
  {\
    FLASH_REGION_NV_FTW_SPARE_BASE,\
    FLASH_REGION_NV_FTW_SPARE_SIZE,\
    EFI_FLASH_AREA_FV | EFI_FLASH_AREA_MEMMAPPED_FV,\
    EFI_FLASH_AREA_FTW_BACKUP,\
    0, 0, 0,\
    { 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },\
  },\
  /* FV_SECPEI region - Base/Size are Dynamic so determined at run time.*/\
  {\
    0,\
    0,\
    EFI_FLASH_AREA_FV | EFI_FLASH_AREA_MEMMAPPED_FV,\
    EFI_FLASH_AREA_RECOVERY_BIOS,\
    0, 0, 0,\
    { 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },\
  },\


//
// EFI_HOB_FLASH_MAP_ENTRY_TYPE definition
//
#define EFI_HOB_FLASH_MAP_ENTRY_TYPE_DATA_DEFINITION \
  /* RUNTIME_UPDATABLE.NV_VARIABLE_STORE Subregion */\
  {\
    EFI_HOB_TYPE_GUID_EXTENSION,\
    sizeof (EFI_HOB_FLASH_MAP_ENTRY_TYPE ),\
    0,\
    EFI_FLASH_MAP_HOB_GUID, \
    0, 0, 0,\
    EFI_FLASH_AREA_EFI_VARIABLES,\
    { 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },\
    1,\
    {\
      EFI_FLASH_AREA_SUBFV | EFI_FLASH_AREA_MEMMAPPED_FV,\
      0,\
      FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_VARIABLE_STORE_BASE,\
      FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_VARIABLE_STORE_SIZE,\
      { 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },\
    },\
  }, \
  /* RUNTIME_UPDATABLE.NV_FTW_WORKING Subregion */\
  {\
    EFI_HOB_TYPE_GUID_EXTENSION,\
    sizeof (EFI_HOB_FLASH_MAP_ENTRY_TYPE ),\
    0,\
    EFI_FLASH_MAP_HOB_GUID, \
    0, 0, 0,\
    EFI_FLASH_AREA_FTW_STATE,\
    { 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },\
    1,\
    {\
      EFI_FLASH_AREA_SUBFV | EFI_FLASH_AREA_MEMMAPPED_FV,\
      0,\
      FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_FTW_WORKING_BASE,\
      FLASH_REGION_RUNTIME_UPDATABLE_SUBREGION_NV_FTW_WORKING_SIZE,\
      { 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },\
    },\
  },

#endif // #ifndef _FLASH_MAP_H_

