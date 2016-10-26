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

  PlatformHelperLib.h

Abstract:

  PlatformHelperLib function prototype definitions.

--*/

#ifndef __PLATFORM_HELPER_LIB_H__
#define __PLATFORM_HELPER_LIB_H__

#include "Platform.h"
#include "PlatformData.h"

//
// Function prototypes for routines exported by this library.
//

/**
  The PlatformCalculateCrc32 routine.

  @param   Data                   The buffer containing the data to be processed.
  @param   DataSize               The size of data to be processed.
  @param   CrcOut                 A pointer to the caller allocated UINT32 that
                                  on return contains the CRC32 checksum of Data.

  @retval  EFI_SUCCESS            Calculation is successful.
  @retval  EFI_INVALID_PARAMETER  Data / CrcOut = NULL, or DataSize = 0.
  @retval  EFI_NOT_FOUND          Firmware volume file not found.

**/
EFI_STATUS
EFIAPI
PlatformCalculateCrc32 (
  IN  UINT8                             *Data,
  IN  UINTN                             DataSize,
  IN OUT UINT32                         *CrcOut
  );

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
  );

/**
  Read 8bit character from debug stream.

  Block until character is read.

  @return 8bit character read from debug stream.

**/
CHAR8
EFIAPI
PlatformDebugPortGetChar8 (
  VOID
  );

/**
  Return platform type string given platform type enum.

  @param  PlatformType  Executing platform type.

  ASSERT if invalid platform type enum.
  ASSERT if EFI_PLATFORM_TYPE_NAME_TABLE_DEFINITION has no entries.
  ASSERT if EFI_PLATFORM_TYPE_NAME_TABLE_DEFINITION has no string for type.

  @return string for platform type enum.

**/
CHAR16 *
EFIAPI
PlatformTypeString (
  IN CONST EFI_PLATFORM_TYPE              Type
  );

/**
  Return if platform type value is supported.

  @param  PlatformType  Executing platform type.

  @retval  TRUE    If type within range and not reserved.
  @retval  FALSE   if type is not supported.

**/
BOOLEAN
EFIAPI
PlatformIsSupportedPlatformType (
  IN CONST EFI_PLATFORM_TYPE              Type
  );

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
  );

/**
  Lock legacy SPI static configuration information.

  Function will assert if unable to lock config.

**/
VOID
EFIAPI
PlatformFlashLockConfig (
  VOID
  );

/**
  Lock regions and config of SPI flash given the policy for this platform.

  Function will assert if unable to lock regions or config.

  @param   PreBootPolicy    If TRUE do Pre Boot Flash Lock Policy.

**/
VOID
EFIAPI
PlatformFlashLockPolicy (
  IN CONST BOOLEAN                        PreBootPolicy
  );

/** Auto provision Secure Boot Resources.

  Provision from platform data, and only provision if PK public cert found.

  @retval  TRUE    UEFI 2.3.1 Secure Boot enabled.
  @retval  FALSE   UEFI 2.3.1 Secure Boot Disabled.

**/
BOOLEAN
EFIAPI
PlatformAutoProvisionSecureBoot (
  VOID
  );

/** Validate Secure boot variables setup correctly for this platform.

  ASSERT if problem found.
  Only validate if PcdSupportSecureBoot AND SecureBootEnabled.

**/
VOID
EFIAPI
PlatformValidateSecureBootVars (
  VOID
  );

/**
  Erase and Write to platform flash.

  Routine accesses one flash block at a time, each access consists
  of an erase followed by a write of FLASH_BLOCK_SIZE. One or both
  of DoErase & DoWrite params must be TRUE.

  Limitations:-
    CpuWriteAddress must be aligned to FLASH_BLOCK_SIZE.
    DataSize must be a multiple of FLASH_BLOCK_SIZE.

  @param   Smst                   If != NULL then InSmm and use to locate
                                  SpiProtocol.
  @param   CpuWriteAddress        Address in CPU memory map of flash region.
  @param   Data                   The buffer containing the data to be written.
  @param   DataSize               Amount of data to write.
  @param   DoErase                Earse each block.
  @param   DoWrite                Write to each block.

  @retval  EFI_SUCCESS            Operation successful.
  @retval  EFI_NOT_READY          Required resources not setup.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  Others                 Unexpected error happened.

**/
EFI_STATUS
EFIAPI
PlatformFlashEraseWrite (
  IN  VOID                              *Smst,
  IN  UINTN                             CpuWriteAddress,
  IN  UINT8                             *Data,
  IN  UINTN                             DataSize,
  IN  BOOLEAN                           DoErase,
  IN  BOOLEAN                           DoWrite
  );

/**
  Those capsules supported by the firmwares.

  @param  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS       Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
PlatformSupportCapsuleImage (
  IN VOID *CapsuleHeader
  );

/** Check if System booted with recovery Boot Stage1 image.

  @retval  TRUE    If system booted with recovery Boot Stage1 image.
  @retval  FALSE   If system booted with normal stage1 image.

**/
BOOLEAN
EFIAPI
PlatformIsBootWithRecoveryStage1 (
  VOID
  );

/**
  Clear SPI Protect registers.

  @retval EFI_SUCESS         SPI protect registers cleared.
  @retval EFI_ACCESS_DENIED  Unable to clear SPI protect registers.
**/

EFI_STATUS
EFIAPI
PlatformClearSpiProtect (
  VOID
  );

/**
  Determine if an SPI address range is protected.

  @param  SpiBaseAddress  Base of SPI range.
  @param  Length          Length of SPI range.

  @retval TRUE       Range is protected.
  @retval FALSE      Range is not protected.
**/
BOOLEAN
EFIAPI
PlatformIsSpiRangeProtected (
  IN CONST UINT32                         SpiBaseAddress,
  IN CONST UINT32                         Length
  );

/**
  Set Legacy GPIO Level

  @param  LevelRegOffset      GPIO level register Offset from GPIO Base Address.
  @param  GpioNum             GPIO bit to change.
  @param  HighLevel           If TRUE set GPIO High else Set GPIO low.

**/
VOID
EFIAPI
PlatformLegacyGpioSetLevel (
  IN CONST UINT32       LevelRegOffset,
  IN CONST UINT32       GpioNum,
  IN CONST BOOLEAN      HighLevel
  );

/**
  Get Legacy GPIO Level

  @param  LevelRegOffset      GPIO level register Offset from GPIO Base Address.
  @param  GpioNum             GPIO bit to check.

  @retval TRUE       If bit is SET.
  @retval FALSE      If bit is CLEAR.

**/
BOOLEAN
EFIAPI
PlatformLegacyGpioGetLevel (
  IN CONST UINT32       LevelRegOffset,
  IN CONST UINT32       GpioNum
  );

/**
  Set the direction of Pcal9555 IO Expander GPIO pin.

  @param  Pcal9555SlaveAddr  I2c Slave address of Pcal9555 Io Expander.
  @param  GpioNum            Gpio direction to configure - values 0-7 for Port0
                             and 8-15 for Port1.
  @param  CfgAsInput         If TRUE set pin direction as input else set as output.

**/
VOID
EFIAPI
PlatformPcal9555GpioSetDir (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum,
  IN CONST BOOLEAN                        CfgAsInput
  );

/**
  Set the level of Pcal9555 IO Expander GPIO high or low.

  @param  Pcal9555SlaveAddr  I2c Slave address of Pcal9555 Io Expander.
  @param  GpioNum            Gpio to change values 0-7 for Port0 and 8-15
                             for Port1.
  @param  HighLevel          If TRUE set pin high else set pin low.

**/
VOID
EFIAPI
PlatformPcal9555GpioSetLevel (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum,
  IN CONST BOOLEAN                        HighLevel
  );

/**

  Enable pull-up/pull-down resistors of Pcal9555 GPIOs.

  @param  Pcal9555SlaveAddr  I2c Slave address of Pcal9555 Io Expander.
  @param  GpioNum            Gpio to change values 0-7 for Port0 and 8-15
                             for Port1.

**/
VOID
EFIAPI
PlatformPcal9555GpioEnablePull (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum
  );

/**

  Disable pull-up/pull-down resistors of Pcal9555 GPIOs.

  @param  Pcal9555SlaveAddr  I2c Slave address of Pcal9555 Io Expander.
  @param  GpioNum            Gpio to change values 0-7 for Port0 and 8-15
                             for Port1.

**/
VOID
EFIAPI
PlatformPcal9555GpioDisablePull (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum
  );

/**
  Init platform LEDs into known state.

  @param   PlatformType     Executing platform type.

  @retval  EFI_SUCCESS      Operation success.

**/
EFI_STATUS
EFIAPI
PlatformLedInit (
  IN CONST EFI_PLATFORM_TYPE              Type
  );

/**
  Turn on or off platform flash update LED.

  @param   PlatformType     Executing platform type.
  @param   TurnOn           If TRUE turn on else turn off.

  @retval  EFI_SUCCESS      Operation success.

**/
EFI_STATUS
EFIAPI
PlatformFlashUpdateLed (
  IN CONST EFI_PLATFORM_TYPE              Type,
  IN CONST BOOLEAN                        TurnOn
  );

#endif // #ifndef __PLATFORM_HELPER_LIB_H__
