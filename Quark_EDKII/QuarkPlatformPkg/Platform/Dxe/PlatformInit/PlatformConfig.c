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

  PlatformConfig.c

Abstract:

    Essential platform configuration.

Revision History

--*/

#include "PlatformInitDxe.h"

//
// The protocols, PPI and GUID defintions for this module
//

//
// The Library classes this module consumes
//

//
// RTC:28208 - System hang/crash when entering probe mode(ITP) when relocating SMBASE
//             Workaround to make default SMRAM UnCachable
//
#define SMM_DEFAULT_SMBASE                  0x30000     // Default SMBASE address
#define SMM_DEFAULT_SMBASE_SIZE_BYTES       0x10000     // Size in bytes of default SMRAM

BOOLEAN                       mMemCfgDone = FALSE;
UINT8                         ChipsetDefaultMac [6] = {0xff,0xff,0xff,0xff,0xff,0xff};

VOID
EFIAPI
PlatformInitializeUart0MuxGalileo (
  VOID
  )
/*++


Routine Description:

  This is the routine to initialize UART0 for DBG2 support. The hardware used in this process is a
  Legacy Bridge (Legacy GPIO), I2C controller, a bi-directional MUX and a Cypress CY8C9540A chip.

Arguments:

  None.

Returns:

  None.

--*/
{
  EFI_STATUS                        Status;
  EFI_I2C_DEVICE_ADDRESS            I2CSlaveAddress;
  UINTN                             Length;
  UINT8                             Buffer[2];

  if (PlatformLegacyGpioGetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, GALILEO_DETERMINE_IOEXP_SLA_RESUMEWELL_GPIO)) {
    I2CSlaveAddress.I2CDeviceAddress = GALILEO_IOEXP_J2HI_7BIT_SLAVE_ADDR;
  } else {
    I2CSlaveAddress.I2CDeviceAddress = GALILEO_IOEXP_J2LO_7BIT_SLAVE_ADDR;
  }

  //
  // Set GPIO_SUS<2> as an output, raise voltage to Vdd.
  //
  PlatformLegacyGpioSetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, 2, TRUE);

  //
  // Select Port 3
  //
  Length = 2;
  Buffer[0] = 0x18; //sub-address
  Buffer[1] = 0x03; //data

  Status = mI2cBus->WriteMultipleByte (
                       mI2cBus,
                       I2CSlaveAddress,
                       EfiI2CSevenBitAddrMode,
                       &Length,
                       &Buffer
                       );
  ASSERT_EFI_ERROR (Status);

  //
  // Set "Pin Direction" bit4 and bit5 as outputs
  //
  Length = 2;
  Buffer[0] = 0x1C; //sub-address
  Buffer[1] = 0xCF; //data

  Status = mI2cBus->WriteMultipleByte (
                       mI2cBus,
                       I2CSlaveAddress,
                       EfiI2CSevenBitAddrMode,
                       &Length,
                       &Buffer
                       );
  ASSERT_EFI_ERROR (Status);

  //
  // Lower GPORT3 bit4 and bit5 to Vss
  //
  Length = 2;
  Buffer[0] = 0x0B; //sub-address
  Buffer[1] = 0xCF; //data

  Status = mI2cBus->WriteMultipleByte (
                       mI2cBus,
                       I2CSlaveAddress,
                       EfiI2CSevenBitAddrMode,
                       &Length,
                       &Buffer
                       );
  ASSERT_EFI_ERROR (Status);
}

VOID
EFIAPI
PlatformInitializeUart0MuxGalileoGen2 (
  VOID
  )
/*++


Routine Description:

  This is the routine to initialize UART0 on GalileoGen2. The hardware used in this process is
  I2C controller and the configuring the following IO Expander signal.

  EXP1.P1_5 should be configured as an output & driven high.
  EXP1.P0_0 should be configured as an output & driven high.
  EXP0.P1_4 should be configured as an output, driven low.
  EXP1.P0_1 pullup should be disabled.
  EXP0.P1_5 Pullup should be disabled.

Arguments:

  None.

Returns:

  None.

--*/

{
  //
  //  EXP1.P1_5 should be configured as an output & driven high.
  //
  PlatformPcal9555GpioSetDir (
    GALILEO_GEN2_IOEXP1_7BIT_SLAVE_ADDR,  // IO Expander 1.
    13,                                   // P1-5.
    TRUE
    );
  PlatformPcal9555GpioSetLevel (
    GALILEO_GEN2_IOEXP1_7BIT_SLAVE_ADDR,  // IO Expander 1.
    13,                                   // P1-5.
    TRUE
    );

  //
  // EXP1.P0_0 should be configured as an output & driven high.
  //
  PlatformPcal9555GpioSetDir (
    GALILEO_GEN2_IOEXP0_7BIT_SLAVE_ADDR,  // IO Expander 0.
    0,                                    // P0_0.
    TRUE
    );
  PlatformPcal9555GpioSetLevel (
    GALILEO_GEN2_IOEXP0_7BIT_SLAVE_ADDR,  // IO Expander 0.
    0,                                    // P0_0.
    TRUE
    );

  //
  //  EXP0.P1_4 should be configured as an output, driven low.
  //
  PlatformPcal9555GpioSetDir (
    GALILEO_GEN2_IOEXP0_7BIT_SLAVE_ADDR,  // IO Expander 0.
    12,                                   // P1-4.
    FALSE
    );
  PlatformPcal9555GpioSetLevel (          // IO Expander 0.
    GALILEO_GEN2_IOEXP0_7BIT_SLAVE_ADDR,  // P1-4
    12,
    FALSE
    );

  //
  // EXP1.P0_1 pullup should be disabled.
  //
  PlatformPcal9555GpioDisablePull (
    GALILEO_GEN2_IOEXP1_7BIT_SLAVE_ADDR,  // IO Expander 1.
    1                                     // P0-1.
    );

  //
  // EXP0.P1_5 Pullup should be disabled.
  //
  PlatformPcal9555GpioDisablePull (
    GALILEO_GEN2_IOEXP0_7BIT_SLAVE_ADDR,  // IO Expander 0.
    13                                    // P1-5.
    );
}

VOID
EFIAPI
PlatformConfigOnSmmConfigurationProtocol (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
/*++

Routine Description:

  Function runs in PI-DXE to perform platform specific config when
  SmmConfigurationProtocol is installed.

Arguments:
  Event       - The event that occured.
  Context     - For EFI compatiblity.  Not used.

Returns:
  None.
--*/

{
  EFI_STATUS            Status;
  UINT32                NewValue;
  UINT64                BaseAddress;
  UINT64                SmramLength;
  EFI_CPU_ARCH_PROTOCOL *CpuArchProtocol;   // RTC:28208 - System hang/crash when entering probe mode(ITP) when relocating SMBASE
  VOID                  *SmmCfgProt;

  Status = gBS->LocateProtocol (&gEfiSmmConfigurationProtocolGuid, NULL, &SmmCfgProt);
  if (Status != EFI_SUCCESS){
    DEBUG ((DEBUG_INFO, "gEfiSmmConfigurationProtocolGuid triggered but not valid.\n"));
    return;
  }
  if (mMemCfgDone) {
    DEBUG ((DEBUG_INFO, "Platform DXE Mem config already done.\n"));
    return;
  }

  //
  // Disable eSram block (this will also clear/zero eSRAM)
  // We only use eSRAM in the PEI phase. Disable now that we are in the DXE phase
  //
  NewValue = QNCPortRead (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_ESRAMPGCTRL_BLOCK);
  NewValue |= BLOCK_DISABLE_PG;
  QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_ESRAMPGCTRL_BLOCK, NewValue);

  //
  // Update HMBOUND to top of DDR3 memory and LOCK
  // We disabled eSRAM so now we move HMBOUND down to top of DDR3
  //
  QNCGetTSEGMemoryRange (&BaseAddress, &SmramLength);
  NewValue = (UINT32)(BaseAddress + SmramLength);
  DEBUG ((EFI_D_INFO,"Locking HMBOUND at: = 0x%8x\n",NewValue));
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QUARK_NC_HOST_BRIDGE_HMBOUND_REG, (NewValue | HMBOUND_LOCK));

  if(FeaturePcdGet (PcdEnableSecureLock)) {
    //
    // Lock IMR5 now that HMBOUND is locked (legacy S3 region)
    //
    NewValue = QNCPortRead (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR5+QUARK_NC_MEMORY_MANAGER_IMRXL);
    NewValue |= IMR_LOCK;
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR5+QUARK_NC_MEMORY_MANAGER_IMRXL, NewValue);

    //
    // Lock IMR6 now that HMBOUND is locked (ACPI Reclaim/ACPI/Runtime services/Reserved)
    //
    NewValue = QNCPortRead (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR6+QUARK_NC_MEMORY_MANAGER_IMRXL);
    NewValue |= IMR_LOCK;
    QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_IMR6+QUARK_NC_MEMORY_MANAGER_IMRXL, NewValue);

    //
    // Disable IMR2 memory protection (RMU Main Binary)
    //
    QncImrWrite (
              QUARK_NC_MEMORY_MANAGER_IMR2,
              (UINT32)(IMRL_RESET & ~IMR_EN),
              (UINT32)IMRH_RESET,
              (UINT32)IMRX_ALL_ACCESS,
              (UINT32)IMRX_ALL_ACCESS
          );

    //
    // Disable IMR3 memory protection (Default SMRAM)
    //
    QncImrWrite (
              QUARK_NC_MEMORY_MANAGER_IMR3,
              (UINT32)(IMRL_RESET & ~IMR_EN),
              (UINT32)IMRH_RESET,
              (UINT32)IMRX_ALL_ACCESS,
              (UINT32)IMRX_ALL_ACCESS
          );

    //
    // Disable IMR4 memory protection (eSRAM).
    //
    QncImrWrite (
              QUARK_NC_MEMORY_MANAGER_IMR4,
              (UINT32)(IMRL_RESET & ~IMR_EN),
              (UINT32)IMRH_RESET,
              (UINT32)IMRX_ALL_ACCESS,
              (UINT32)IMRX_ALL_ACCESS
          );
  }

  //
  // RTC:28208 - System hang/crash when entering probe mode(ITP) when relocating SMBASE
  //             Workaround to make default SMRAM UnCachable
  //
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **) &CpuArchProtocol);
  ASSERT_EFI_ERROR (Status);

  CpuArchProtocol->SetMemoryAttributes (
                     CpuArchProtocol,
                     (EFI_PHYSICAL_ADDRESS) SMM_DEFAULT_SMBASE,
                     SMM_DEFAULT_SMBASE_SIZE_BYTES,
                     EFI_MEMORY_WB
                     );

  mMemCfgDone = TRUE;
}

VOID
EFIAPI
PlatformConfigOnSpiReady (
  IN  EFI_EVENT Event,
  IN  VOID      *Context
  )
/*++

Routine Description:

  Function runs in PI-DXE to perform platform specific config when SPI
  interface is ready.

Arguments:
  Event       - The event that occured.
  Context     - For EFI compatiblity.  Not used.

Returns:
  None.

--*/
{
  EFI_STATUS                        Status;
  VOID                              *SpiReadyProt = NULL;
  EFI_PLATFORM_TYPE_PROTOCOL        *PlatformType;
  EFI_BOOT_MODE                      BootMode;

  BootMode = GetBootModeHob ();

  PlatformType = &mPrivatePlatformData.PlatformType;

  Status = gBS->LocateProtocol (&gEfiSmmSpiReadyProtocolGuid, NULL, &SpiReadyProt);
  if (Status != EFI_SUCCESS){
    DEBUG ((DEBUG_INFO, "gEfiSmmSpiReadyProtocolGuid triggered but not valid.\n"));
    return;
  }

  //
  // Lock regions SPI flash.
  //
  PlatformFlashLockPolicy (FALSE);

  //
  // Configurations and checks to be done when DXE tracing available.
  //

  //
  // Platform specific Signal routing.
  //

  //
  // Skip any signal not needed for recovery and flash update.
  //
  if (BootMode != BOOT_ON_FLASH_UPDATE && BootMode != BOOT_IN_RECOVERY_MODE) {

    //
    // Galileo Platform UART0 support.
    //
    if (PlatformType->Type == Galileo) {
      //
      // Use MUX to connect out UART0 pins.
      //
      PlatformInitializeUart0MuxGalileo ();
    }

    //
    // GalileoGen2 Platform UART0 support.
    //
    if (PlatformType->Type == GalileoGen2) {
      //
      // Use route out UART0 pins.
      //
      PlatformInitializeUart0MuxGalileoGen2 ();
    }
  }
}

EFI_STATUS
EFIAPI
CreateConfigEvents (
  VOID
  )
/*++

Routine Description:

Arguments:
  None

Returns:
  EFI_STATUS

--*/
{
  EFI_EVENT   EventSmmCfg;
  EFI_EVENT   EventSpiReady;
  VOID        *RegistrationSmmCfg;
  VOID        *RegistrationSpiReady;

  //
  // Schedule callback for when SmmConfigurationProtocol installed.
  //
  EventSmmCfg = EfiCreateProtocolNotifyEvent (
                  &gEfiSmmConfigurationProtocolGuid,
                  TPL_CALLBACK,
                  PlatformConfigOnSmmConfigurationProtocol,
                  NULL,
                  &RegistrationSmmCfg
                  );
  ASSERT (EventSmmCfg != NULL);

  //
  // Schedule callback to setup SPI Flash Policy when SPI interface ready.
  //
  EventSpiReady = EfiCreateProtocolNotifyEvent (
                    &gEfiSmmSpiReadyProtocolGuid,
                    TPL_CALLBACK,
                    PlatformConfigOnSpiReady,
                    NULL,
                    &RegistrationSpiReady
                    );
  ASSERT (EventSpiReady != NULL);
  return EFI_SUCCESS;
}
