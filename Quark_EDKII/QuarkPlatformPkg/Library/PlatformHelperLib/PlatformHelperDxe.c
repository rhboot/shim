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

  PlatformHelperDxe.c

Abstract:

  Implementation of helper routines for DXE enviroment.

--*/

#include <PiDxe.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/PlatformType.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/Spi.h>
#include <Protocol/I2CHc.h>

#include <Guid/QuarkVariableLock.h>

#include "CommonHeader.h"

//
// Global variables.
//
EFI_SPI_PROTOCOL                    *mPlatHelpSpiProtocolRef = NULL;

//
// Routines defined in other source modules of this component.
//

//
// Routines local to this component.
//

//
// Routines shared with other souce modules in this component.
//

VOID
Pcal9555SetPortRegBit (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum,
  IN CONST UINT8                          RegBase,
  IN CONST BOOLEAN                        LogicOne
  )
{
  EFI_STATUS                        Status;
  UINTN                             ReadLength;
  UINTN                             WriteLength;
  UINT8                             Data[2];
  EFI_I2C_DEVICE_ADDRESS            I2cDeviceAddr;
  EFI_I2C_ADDR_MODE                 I2cAddrMode;
  UINT8                             *RegValuePtr;
  UINT8                             GpioNumMask;
  UINT8                             SubAddr;
  EFI_I2C_HC_PROTOCOL               *I2cBus;

  //
  // Locate I2C host controller driver.
  //
  Status = gBS->LocateProtocol (&gEfiI2CHcProtocolGuid, NULL, (VOID **) &I2cBus);
  ASSERT_EFI_ERROR (Status);

  I2cDeviceAddr.I2CDeviceAddress = (UINTN) Pcal9555SlaveAddr;
  I2cAddrMode = EfiI2CSevenBitAddrMode;

  if (GpioNum < 8) {
    SubAddr = RegBase;
    GpioNumMask = (UINT8) (1 << GpioNum);
  } else {
    SubAddr = RegBase + 1;
    GpioNumMask = (UINT8) (1 << (GpioNum - 8));
  }

  //
  // Output port value always at 2nd byte in Data variable.
  //
  RegValuePtr = &Data[1];

  //
  // On read entry sub address at 2nd byte, on read exit output
  // port value in 2nd byte.
  //
  Data[1] = SubAddr;
  WriteLength = 1;
  ReadLength = 1;
  Status = I2cBus->ReadMultipleByte (
                      I2cBus,
                      I2cDeviceAddr,
                      I2cAddrMode,
                      &WriteLength,
                      &ReadLength,
                      &Data[1]
                      );
  ASSERT_EFI_ERROR (Status);

  //
  // Adjust output port bit given callers request.
  //
  if (LogicOne) {
    *RegValuePtr = *RegValuePtr | GpioNumMask;
  } else {
    *RegValuePtr = *RegValuePtr & ~(GpioNumMask);
  }

  //
  // Update register. Sub address at 1st byte, value at 2nd byte.
  //
  WriteLength = 2;
  Data[0] = SubAddr;
  Status = I2cBus->WriteMultipleByte (
                      I2cBus,
                      I2cDeviceAddr,
                      I2cAddrMode,
                      &WriteLength,
                      Data
                      );
  ASSERT_EFI_ERROR (Status);
}


EFI_SPI_PROTOCOL *
LocateSpiProtocol (
  IN  EFI_SMM_SYSTEM_TABLE2             *Smst
  )
{
  if (mPlatHelpSpiProtocolRef == NULL) {
    if (Smst != NULL) {
      Smst->SmmLocateProtocol (
              &gEfiSmmSpiProtocolGuid,
              NULL,
              (VOID **) &mPlatHelpSpiProtocolRef
              );
    } else {
      gBS->LocateProtocol (
             &gEfiSpiProtocolGuid,
             NULL,
             (VOID **) &mPlatHelpSpiProtocolRef
             );
    }
    ASSERT (mPlatHelpSpiProtocolRef != NULL);
  }
  return mPlatHelpSpiProtocolRef;
}

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
  if (FileNameGuid == NULL || SectionData == NULL || SectionDataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (FvNameGuid != NULL) {
    return EFI_UNSUPPORTED;  // Searching in specific FV unsupported in DXE.
  }

  return GetSectionFromAnyFv (FileNameGuid, EFI_SECTION_RAW, 0, SectionData, SectionDataSize);
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
  UINT32                            FreeOffset;
  UINT32                            PchRootComplexBar;
  EFI_STATUS                        Status;

  PchRootComplexBar = QNC_RCRB_BASE;

  Status = WriteFirstFreeSpiProtect (
             PchRootComplexBar,
             DirectValue,
             BaseAddress,
             Length,
             &FreeOffset
             );

  if (!EFI_ERROR (Status)) {
    S3BootScriptSaveMemWrite (
      S3BootScriptWidthUint32,
        (UINTN) (PchRootComplexBar + FreeOffset),
        1,
        (VOID *) (UINTN) (PchRootComplexBar + FreeOffset)
        );
  }

  return Status;
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
  EFI_STATUS        Status;
  EFI_SPI_PROTOCOL  *SpiProtocol;

  //
  // Enable lock of legacy SPI static configuration information.
  //

  SpiProtocol = LocateSpiProtocol (NULL);  // This routine will not be called in SMM.
  ASSERT_EFI_ERROR (SpiProtocol != NULL);
  if (SpiProtocol != NULL) {
    Status = SpiProtocol->Lock (SpiProtocol);

    if (!EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "Platform: Spi Config Locked Down\n"));
    } else if (Status == EFI_ACCESS_DENIED) {
      DEBUG ((EFI_D_INFO, "Platform: Spi Config already locked down\n"));
    } else {
      ASSERT_EFI_ERROR (Status);
    }
  }
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
  EFI_PLATFORM_TYPE_PROTOCOL        *PlatformType;
  EFI_STATUS                        Status;
  UINT64                            CpuAddressNvStorage;
  UINT64                            CpuAddressPlatformData;
  UINT64                            CpuAddressFixedRecovery;
  UINT64                            CpuAddressFlashDevice;
  UINT64                            SpiAddress;
  EFI_BOOT_MODE                     BootMode;

  BootMode = GetBootModeHob ();

  Status = gBS->LocateProtocol (&gEfiPlatformTypeProtocolGuid, NULL, (VOID **) &PlatformType);
  ASSERT_EFI_ERROR (Status);

  CpuAddressFlashDevice = SIZE_4GB - ((UINT64) PlatformType->FlashDeviceSize);
  DEBUG (
      (EFI_D_INFO,
      "Platform:FlashDeviceSize = 0x%08x Bytes\n",
      (UINTN) PlatformType->FlashDeviceSize)
      );

  //
  // Lock regions if Secure lock down.
  //

  if (FeaturePcdGet (PcdEnableSecureLock)) {

    CpuAddressNvStorage = (UINT64) PcdGet32 (PcdFlashNvStorageBase);

    //
    // Lock from start of flash device up to Smi writable flash storage areas.
    //
    SpiAddress = 0;
    if (!PlatformIsSpiRangeProtected ((UINT32) SpiAddress, (UINT32) (CpuAddressNvStorage - CpuAddressFlashDevice))) {
      DEBUG (
        (EFI_D_INFO,
        "Platform: Protect Region Base:Len 0x%08x:0x%08x\n",
        (UINTN) SpiAddress, (UINTN)(CpuAddressNvStorage - CpuAddressFlashDevice))
        );
      Status = PlatformWriteFirstFreeSpiProtect (
                 0,
                 (UINT32) SpiAddress,
                 (UINT32) (CpuAddressNvStorage - CpuAddressFlashDevice)
                 );

      ASSERT_EFI_ERROR (Status);
    }
    //
    // Move Spi Address to after Smi writable flash storage areas.
    //
    SpiAddress = CpuAddressNvStorage - CpuAddressFlashDevice;
    SpiAddress += ((UINT64) PcdGet32 (PcdFlashNvStorageSize)) + ((UINT64) FLASH_REGION_OEM_NV_STORAGE_SIZE);

    //
    // Lock from end of OEM area to end of flash part.
    //
    if (!PlatformIsSpiRangeProtected ((UINT32) SpiAddress, PlatformType->FlashDeviceSize - ((UINT32) SpiAddress))) {
      DEBUG (
        (EFI_D_INFO,
        "Platform: Protect Region Base:Len 0x%08x:0x%08x\n",
        (UINTN) SpiAddress,
        (UINTN) (PlatformType->FlashDeviceSize - ((UINT32) SpiAddress)))
        );
      ASSERT (SpiAddress < ((UINT64) PlatformType->FlashDeviceSize));
      Status = PlatformWriteFirstFreeSpiProtect (
                 0,
                 (UINT32) SpiAddress,
                 PlatformType->FlashDeviceSize - ((UINT32) SpiAddress)
                 );

      ASSERT_EFI_ERROR (Status);
    }
  } else {

    //
    // Lock Platform Data storage area
    //

    CpuAddressPlatformData = (UINT64) PcdGet32 (PcdPlatformDataBaseAddress);
    SpiAddress = CpuAddressPlatformData - CpuAddressFlashDevice;
    if (!PlatformIsSpiRangeProtected ((UINT32) SpiAddress,(UINT32) (PcdGet32 (PcdPlatformDataMaxLen)))) {
      DEBUG (
          (EFI_D_INFO,
          "Platform: Protect Region Base:Len 0x%08x:0x%08x\n",
          (UINTN) SpiAddress, (UINTN)(PcdGet32 (PcdPlatformDataMaxLen)))
          );
      Status = PlatformWriteFirstFreeSpiProtect (
          0,
          (UINT32) SpiAddress,
          (UINT32) (PcdGet32 (PcdPlatformDataMaxLen))
          );

      ASSERT_EFI_ERROR (Status);
    }

    //
    // Move Spi Address to beginning of Fixed Recovery area.
    //

    CpuAddressFixedRecovery = (UINT64) PcdGet32 (PcdFlashFvFixedStage1AreaBase);
    SpiAddress = CpuAddressFixedRecovery - CpuAddressFlashDevice;

    //
    // Lock Fixed Recovery area
    //

    if (!PlatformIsSpiRangeProtected ((UINT32) SpiAddress,(UINT32) (PcdGet32 (PcdFlashFvFixedStage1AreaSize)))) {
      DEBUG (
          (EFI_D_INFO,
          "Platform: Protect Region Base:Len 0x%08x:0x%08x\n",
          (UINTN) SpiAddress,
          (UINTN) (PcdGet32 (PcdFlashFvFixedStage1AreaSize)))
          );
      ASSERT (SpiAddress < ((UINT64) PlatformType->FlashDeviceSize));
      Status = PlatformWriteFirstFreeSpiProtect (
          0,
          (UINT32) SpiAddress,
          (UINTN) (PcdGet32 (PcdFlashFvFixedStage1AreaSize))
          );

      ASSERT_EFI_ERROR (Status);
    }

    //
    // Move Spi Address to beginning of BOOTROM override area.
    //

    SpiAddress = FLASH_REGION_BOOTROM_OVERRIDE_BASE - CpuAddressFlashDevice;

    //
    // Lock BOOTROM override area.
    //
    if (!PlatformIsSpiRangeProtected ((UINT32) SpiAddress,FLASH_REGION_BOOTROM_OVERRIDE_MAX_SIZE)) {
      DEBUG (
          (EFI_D_INFO,
          "Platform: Protect Region Base:Len 0x%08x:0x%08x\n",
          (UINTN) SpiAddress,
          (UINTN) FLASH_REGION_BOOTROM_OVERRIDE_MAX_SIZE)
          );
      ASSERT (SpiAddress < ((UINT64) PlatformType->FlashDeviceSize));
      Status = PlatformWriteFirstFreeSpiProtect (
          0,
          (UINT32) SpiAddress,
          FLASH_REGION_BOOTROM_OVERRIDE_MAX_SIZE
          );

      ASSERT_EFI_ERROR (Status);
    }
  }

  //
  // Always Lock flash config registers if about to boot a boot option
  // else lock depending on boot mode.
  //
  if (PreBootPolicy || (BootMode != BOOT_ON_FLASH_UPDATE)) {
    PlatformFlashLockConfig ();
  }

  //
  // Enable Quark Variable lock if PreBootPolicy.
  //
  if (PreBootPolicy) {
    Status = gRT->SetVariable (
                 QUARK_VARIABLE_LOCK_NAME,
                 &gQuarkVariableLockGuid,
                 0,
                 0,
                 (VOID *)NULL
                 );

    ASSERT_EFI_ERROR (Status);
  }
}

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
  )
{
  EFI_STATUS                        Status;
  UINT64                            CpuBaseAddress;
  SPI_INIT_INFO                     *SpiInfo;
  UINT8                             *WriteBuf;
  UINTN                             Index;
  UINTN                             SpiWriteAddress;
  EFI_SPI_PROTOCOL                  *SpiProtocol;

  if (!DoErase && !DoWrite) {
    return EFI_INVALID_PARAMETER;
  }
  if (DoWrite && Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if ((CpuWriteAddress % FLASH_BLOCK_SIZE) != 0) {
    return EFI_INVALID_PARAMETER;
  }
  if ((DataSize % FLASH_BLOCK_SIZE) != 0) {
    return EFI_INVALID_PARAMETER;
  }
  SpiProtocol = LocateSpiProtocol ((EFI_SMM_SYSTEM_TABLE2 *)Smst);
  if (SpiProtocol == NULL) {
    return EFI_NOT_READY;
  }

  //
  // Find info to allow usage of SpiProtocol->Execute.
  //
  Status = SpiProtocol->Info (
             SpiProtocol,
             &SpiInfo
             );
  if (EFI_ERROR(Status)) {
    return Status;
  }
  ASSERT (SpiInfo->InitTable != NULL);
  ASSERT (SpiInfo->EraseOpcodeIndex < SPI_NUM_OPCODE);
  ASSERT (SpiInfo->ProgramOpcodeIndex < SPI_NUM_OPCODE);

  CpuBaseAddress = PcdGet32 (PcdFlashAreaBaseAddress) - (UINT32)SpiInfo->InitTable->BiosStartOffset;
  ASSERT(CpuBaseAddress >= (SIZE_4GB - SIZE_8MB));
  if (CpuWriteAddress < CpuBaseAddress) {
    return (EFI_INVALID_PARAMETER);
  }

  SpiWriteAddress = CpuWriteAddress - ((UINTN) CpuBaseAddress);
  WriteBuf = Data;
  DEBUG (
    (EFI_D_INFO, "PlatformFlashWrite:SpiWriteAddress=%08x EraseIndex=%d WriteIndex=%d\n", 
    SpiWriteAddress,
    (UINTN) SpiInfo->EraseOpcodeIndex,
    (UINTN) SpiInfo->ProgramOpcodeIndex
    ));
  for (Index =0; Index < DataSize / FLASH_BLOCK_SIZE; Index++) {
    if (DoErase) {
      DEBUG (
        (EFI_D_INFO, "PlatformFlashWrite:Erase[%04x] SpiWriteAddress=%08x\n",
        Index,
        SpiWriteAddress
        ));
      Status = SpiProtocol->Execute (
                              SpiProtocol,
                              SpiInfo->EraseOpcodeIndex,// OpcodeIndex
                              0,                        // PrefixOpcodeIndex
                              FALSE,                    // DataCycle
                              TRUE,                     // Atomic
                              FALSE,                    // ShiftOut
                              SpiWriteAddress,          // Address
                              0,                        // Data Number
                              NULL,
                              EnumSpiRegionAll          // SPI_REGION_TYPE
                              );
      if (EFI_ERROR(Status)) {
        return Status;
      }
    }

    if (DoWrite) {
      DEBUG (
        (EFI_D_INFO, "PlatformFlashWrite:Write[%04x] SpiWriteAddress=%08x\n",
        Index,
        SpiWriteAddress
        ));
      Status = SpiProtocol->Execute (
                              SpiProtocol,
                              SpiInfo->ProgramOpcodeIndex,   // OpcodeIndex
                              0,                             // PrefixOpcodeIndex
                              TRUE,                          // DataCycle
                              TRUE,                          // Atomic
                              TRUE,                          // ShiftOut
                              SpiWriteAddress,               // Address
                              FLASH_BLOCK_SIZE,              // Data Number
                              WriteBuf,
                              EnumSpiRegionAll
                              );
      if (EFI_ERROR(Status)) {
        return Status;
      }
      WriteBuf+=FLASH_BLOCK_SIZE;
    }
    SpiWriteAddress+=FLASH_BLOCK_SIZE;
  }
  return EFI_SUCCESS;
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
  ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
  return FALSE;
}

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
  )
{
  Pcal9555SetPortRegBit (
    Pcal9555SlaveAddr,
    GpioNum,
    PCAL9555_REG_CFG_PORT0,
    CfgAsInput
    );
}

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
  )
{
  Pcal9555SetPortRegBit (
    Pcal9555SlaveAddr,
    GpioNum,
    PCAL9555_REG_OUT_PORT0,
    HighLevel
    );
}

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
  )
{
  Pcal9555SetPortRegBit (
    Pcal9555SlaveAddr,
    GpioNum,
    PCAL9555_REG_PULL_EN_PORT0,
    TRUE
    );
}

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
  )
{
  Pcal9555SetPortRegBit (
    Pcal9555SlaveAddr,
    GpioNum,
    PCAL9555_REG_PULL_EN_PORT0,
    FALSE
    );
}