/** @file
  Lib function for Pei QNC.

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

**/
#include "CommonHeader.h"

/**
  This function provides the necessary SOC initialization
  before MRC running. It sets RCBA, GPIO, PMBASE
  and some parts of SOC through SOC message method.
  If the function cannot complete it'll ASSERT().
**/
VOID
EFIAPI
PeiQNCPreMemInit (
  VOID
  )
{
  UINT32                            RegValue;

  // QNCPortWrite(Port#, Offset, Value)

  //
  // Set the fixed PRI Status encodings config.
  //
  QNCPortWrite (
    QUARK_NC_MEMORY_ARBITER_SB_PORT_ID,
    QUARK_NC_MEMORY_ARBITER_REG_ASTATUS,
    QNC_FIXED_CONFIG_ASTATUS
    );

  // Sideband register write to Remote Management Unit
  QNCPortWrite (QUARK_NC_RMU_SB_PORT_ID, QNC_MSG_TMPM_REG_PMBA, (BIT31 | PcdGet16 (PcdPmbaIoBaseAddress)));

  // Configurable I/O address in iLB (legacy block)

  LpcPciCfg32 (R_QNC_LPC_SMBUS_BASE) = BIT31 | PcdGet16 (PcdSmbaIoBaseAddress);
  LpcPciCfg32 (R_QNC_LPC_GBA_BASE) = BIT31 | PcdGet16 (PcdGbaIoBaseAddress);
  LpcPciCfg32 (R_QNC_LPC_PM1BLK) = BIT31 | PcdGet16 (PcdPm1blkIoBaseAddress);
  LpcPciCfg32 (R_QNC_LPC_GPE0BLK) = BIT31 | PcdGet16 (PcdGpe0blkIoBaseAddress);
  LpcPciCfg32 (R_QNC_LPC_WDTBA) = BIT31 | PcdGet16 (PcdWdtbaIoBaseAddress);

  //
  // Program RCBA Base Address
  //
  LpcPciCfg32AndThenOr (R_QNC_LPC_RCBA, (~B_QNC_LPC_RCBA_MASK), (((UINT32)(PcdGet64 (PcdRcbaMmioBaseAddress))) | B_QNC_LPC_RCBA_EN));

  //
  // Program Memory Manager fixed config values.
  //

  RegValue = QNCPortRead (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_BTHCTRL);
  RegValue &= ~(DRAM_NON_HOST_RQ_LIMIT_MASK);
  RegValue |= (V_DRAM_NON_HOST_RQ_LIMIT << DRAM_NON_HOST_RQ_LIMIT_BP);
  QNCPortWrite (QUARK_NC_MEMORY_MANAGER_SB_PORT_ID, QUARK_NC_MEMORY_MANAGER_BTHCTRL, RegValue);

  //
  // Program iCLK fixed config values.
  //
  QncIClkAndThenOr (
    QUARK_ICLK_MUXTOP,
    (UINT32) ~(B_MUXTOP_FLEX2_MASK | B_MUXTOP_FLEX1_MASK),
    (V_MUXTOP_FLEX2 << B_MUXTOP_FLEX2_BP) | (V_MUXTOP_FLEX1 << B_MUXTOP_FLEX1_BP)
    );
  QncIClkAndThenOr (
    QUARK_ICLK_REF2_DBUFF0,
    (UINT32) ~(BIT0), // bit[0] cleared
    0
    );
  QncIClkOr (
    QUARK_ICLK_SSC1,
    BIT0              // bit[0] set
    );
  QncIClkOr (
    QUARK_ICLK_SSC2,
    BIT0              // bit[0] set
    );
  QncIClkOr (
    QUARK_ICLK_SSC3,
    BIT0              // bit[0] set
    );

  //
  // Set RMU DMA disable bit post boot.
  //
  if(FeaturePcdGet(PcdRmuDmaLock)) {
    RegValue = QNCPortRead (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_OPTIONS_1);
    RegValue |= OPTIONS_1_DMA_DISABLE;
    QNCPortWrite (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_OPTIONS_1, RegValue);
  }
}

/**
  Do north cluster init which needs to be done AFTER MRC init.

  @param   VOID

  @retval  VOID
**/

VOID
EFIAPI
PeiQNCPostMemInit (
  VOID
  )
{
  //
  // Program SVID/SID the same as VID/DID for all devices except root ports.
  //
  QNCMmPci32(0, MC_BUS, MC_DEV, MC_FUN, R_EFI_PCI_SVID) = QNCMmPci32(0, MC_BUS, MC_DEV, MC_FUN, PCI_VENDOR_ID_OFFSET);
  QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, PCI_FUNCTION_NUMBER_QNC_LPC, R_EFI_PCI_SVID) = QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_LPC, PCI_FUNCTION_NUMBER_QNC_LPC, PCI_VENDOR_ID_OFFSET);
  QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_IOSF2AHB_0, PCI_FUNCTION_NUMBER_QNC_IOSF2AHB, R_EFI_PCI_SVID) = QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_IOSF2AHB_0, PCI_FUNCTION_NUMBER_QNC_IOSF2AHB, PCI_VENDOR_ID_OFFSET);
  QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_IOSF2AHB_1, PCI_FUNCTION_NUMBER_QNC_IOSF2AHB, R_EFI_PCI_SVID) = QNCMmPci32(0, PCI_BUS_NUMBER_QNC, PCI_DEVICE_NUMBER_QNC_IOSF2AHB_1, PCI_FUNCTION_NUMBER_QNC_IOSF2AHB, PCI_VENDOR_ID_OFFSET);
  return;
}

/**
  Used to check QNC if it's S3 state.  Clear the register state after query.

  @retval TRUE if it's S3 state.
  @retval FALSE if it's not S3 state.

**/
BOOLEAN
EFIAPI
QNCCheckS3AndClearState (
   VOID
  )
{
  BOOLEAN       S3WakeEventFound;
  UINT16        Pm1Sts;
  UINT16        Pm1En;
  UINT16        Pm1Cnt;
  UINT32        Gpe0Sts;
  UINT32        Gpe0En;
  UINT32        NewValue;

  S3WakeEventFound = FALSE;

  //
  // Read the ACPI registers,
  //
  Pm1Sts  = IoRead16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1S);
  Pm1En   = IoRead16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1E);
  Pm1Cnt  = IoRead16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C);
  Gpe0Sts = IoRead32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_GPE0S);
  Gpe0En  = IoRead32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_GPE0E);

  //
  // Clear Power Management 1 Enable Register and
  // General Purpost Event 0 Enables Register
  //
  IoWrite16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1E, 0);
  IoWrite32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_GPE0E, 0);

  if ((Pm1Sts & B_QNC_PM1BLK_PM1S_WAKE) != 0 && (Pm1Cnt & B_QNC_PM1BLK_PM1C_SLPTP) == V_S3) {

    //
    // Detect the actual WAKE event
    //
    if ((Pm1Sts & B_QNC_PM1BLK_PM1S_RTC) && (Pm1En & B_QNC_PM1BLK_PM1E_RTC)) {
      DEBUG ((EFI_D_INFO, "S3 Wake Event - RTC Alarm event detected\n"));
      S3WakeEventFound = TRUE;
    }
    if ((Pm1Sts & B_QNC_PM1BLK_PM1S_PCIEWSTS) && !(Pm1En & B_QNC_PM1BLK_PM1E_PWAKED)) {
      DEBUG ((EFI_D_INFO, "S3 Wake Event - PCIe WAKE detected\n"));
      S3WakeEventFound = TRUE;
    }
    if ((Gpe0Sts & B_QNC_GPE0BLK_GPE0S_PCIE) && (Gpe0En & B_QNC_GPE0BLK_GPE0E_PCIE)) {
      DEBUG ((EFI_D_INFO, "S3 Wake Event - PCIe event detected\n"));
      S3WakeEventFound = TRUE;
    }
    if ((Gpe0Sts & B_QNC_GPE0BLK_GPE0S_GPIO) && (Gpe0En & B_QNC_GPE0BLK_GPE0E_GPIO)) {
      DEBUG ((EFI_D_INFO, "S3 Wake Event - GPIO event detected\n"));
      S3WakeEventFound = TRUE;
    }
    if ((Gpe0Sts & B_QNC_GPE0BLK_GPE0S_EGPE) && (Gpe0En & B_QNC_GPE0BLK_GPE0E_EGPE)) {
      DEBUG ((EFI_D_INFO, "S3 Wake Event - External GPE event detected\n"));
      S3WakeEventFound = TRUE;
    }
    if (S3WakeEventFound == FALSE) {
      DEBUG ((EFI_D_INFO, "S3 Wake Event - Unknown event detected!!!!!\n"));
    }

    //
    // If no Power Button Override event occurs and one enabled wake event occurs,
    // just do S3 resume and clear the state.
    //
    IoWrite16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C, (Pm1Cnt & (~B_QNC_PM1BLK_PM1C_SLPTP)));

    //
    // Set EOS to de Assert SMI
    //
    IoWrite32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIS,  B_QNC_GPE0BLK_SMIS_EOS);	

    //
    // Enable SMI globally
    //
    NewValue = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
    NewValue |= SMI_EN;
    QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, NewValue);

    return  TRUE;
  }

  return  FALSE;
}

/**
  Used to check QNC if system wakes up from power on reset. Clear the register state after query.

  @retval TRUE  if system wakes up from power on reset
  @retval FALSE if system does not wake up from power on reset

**/
BOOLEAN
EFIAPI
QNCCheckPowerOnResetAndClearState (
   VOID
  )
{
  UINT16                Pm1Sts;
  UINT16                Pm1Cnt;

  //
  // Read the ACPI registers,
  // PM1_STS information cannot be lost after power down, unless CMOS is cleared.
  //
  Pm1Sts  = IoRead16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1S);
  Pm1Cnt  = IoRead16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C);

  //
  // If B_SLP_TYP is S5
  //
  if ((Pm1Sts & B_QNC_PM1BLK_PM1S_WAKE) != 0 && (Pm1Cnt & B_QNC_PM1BLK_PM1C_SLPTP) == V_S5) {
    IoWrite16 (PcdGet16 (PcdPm1blkIoBaseAddress) + R_QNC_PM1BLK_PM1C, (Pm1Cnt & (~B_QNC_PM1BLK_PM1C_SLPTP)));
    return  TRUE;
  }

  return  FALSE;
}

/**
  This function is used to clear SMI and wake status.

**/
VOID
EFIAPI
QNCClearSmiAndWake (
  VOID
  )
{
  UINT32    Gpe0Sts;
  UINT32    SmiSts;
  
  //
  // Read the ACPI registers
  //
  Gpe0Sts = IoRead32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_GPE0S);
  SmiSts  = IoRead32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIS);

  //
  // Clear any SMI or wake state from the boot
  //
  Gpe0Sts |= B_QNC_GPE0BLK_GPE0S_ALL;
  SmiSts  |= B_QNC_GPE0BLK_SMIS_ALL;

  //
  // Write them back
  //
  IoWrite32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_GPE0S, Gpe0Sts);
  IoWrite32 ((UINT16)(LpcPciCfg32 (R_QNC_LPC_GPE0BLK) & 0xFFFF) + R_QNC_GPE0BLK_SMIS,  SmiSts);	
}

/** Send DRAM Ready opcode.

  @param[in]       OpcodeParam  Parameter to DRAM ready opcode.

  @retval          VOID
**/
VOID
EFIAPI
QNCSendOpcodeDramReady (
  IN UINT32   OpcodeParam
  )
{

  //
  // Before sending DRAM ready place invalid value in Scrub Config.
  //
  QNCPortWrite (
    QUARK_NC_RMU_SB_PORT_ID,
    QUARK_NC_ECC_SCRUB_CONFIG_REG,
    SCRUB_CFG_INVALID
    );

  //
  // Send opcode and use param to notify HW of new RMU firmware location.
  //
  McD0PciCfg32 (QNC_ACCESS_PORT_MDR) = OpcodeParam;
  McD0PciCfg32 (QNC_ACCESS_PORT_MCR) = MESSAGE_SHADOW_DW (QUARK_NC_RMU_SB_PORT_ID, 0);

  //
  // HW completed tasks on DRAM ready when scrub config read back as zero.
  //
  while (QNCPortRead (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_ECC_SCRUB_CONFIG_REG) != 0) {
    MicroSecondDelay (10);
  }
}

/**

  Relocate RMU Main binary to memory after MRC to improve performance.

  @param[in]  DestBaseAddress  - Specify the new memory address for the RMU Main binary.
  @param[in]  SrcBaseAddress   - Specify the current memory address for the RMU Main binary.
  @param[in]  Size             - Specify size of the RMU Main binary.

  @retval     VOID

**/
VOID
EFIAPI
RmuMainRelocation (
  IN CONST UINT32   DestBaseAddress,
  IN CONST UINT32   SrcBaseAddress,
  IN CONST UINTN    Size
  )
{
  //
  // Shadow RMU Main binary into main memory.
  //
  CopyMem ((VOID *)(UINTN)DestBaseAddress,(VOID *)(UINTN) SrcBaseAddress, Size);
}


/**
  Get the total memory size

**/
UINT32
QNCGetTotalMemorysize (
  VOID
  )
{
  return  QNCPortRead(QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QUARK_NC_HOST_BRIDGE_HMBOUND_REG) && HMBOUND_MASK;

}


/**
  Get the memory range of TSEG.
  The TSEG's memory is below TOLM.

  @param[out] BaseAddress The base address of TSEG's memory range
  @param[out] MemorySize  The size of TSEG's memory range

**/
VOID
EFIAPI
QNCGetTSEGMemoryRange (
  OUT UINT64  *BaseAddress,
  OUT UINT64  *MemorySize
  )
{
  UINT64 Register = 0;
  UINT64 SMMAddress = 0;
  
  Register = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HSMMC);

  //
  // Get the SMRAM Base address
  //
  SMMAddress = Register & SMM_START_MASK;
  *BaseAddress = LShift16 (SMMAddress);

  //
  // Get the SMRAM size
  //
  SMMAddress = ((Register & SMM_END_MASK) | (~SMM_END_MASK)) + 1;
  *MemorySize = SMMAddress - (*BaseAddress);

  DEBUG ((
    EFI_D_INFO,
    "TSEG's memory range: BaseAddress = 0x%x, Size = 0x%x\n",
    (UINT32)*BaseAddress,
    (UINT32)*MemorySize
    ));
}


/**
  This routine is the chipset code that accepts a request to "open" a region of SMRAM.
  The region could be legacy ABSEG, HSEG, or TSEG near top of physical memory.
  The use of "open" means that the memory is visible from all boot-service
  and SMM agents.

  @retval FALSE  Cannot open a locked SMRAM region
  @retval TRUE   Success to open SMRAM region.
**/
BOOLEAN
EFIAPI
QNCOpenSmramRegion (
  VOID
  )
{
  UINT32                     Smram;
  UINT32                     SmmOpen;
  
  SmmOpen = (SMM_WRITE_OPEN | SMM_READ_OPEN);

  // Read the SMRAM register
  Smram = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HSMMC);

  //
  //  Is the platform locked?
  //
  if (Smram & SMM_LOCKED) {
    // Cannot Open a locked region
    DEBUG ((EFI_D_WARN, "Cannot open a locked SMRAM region\n"));
    return FALSE;
  }

  //
  // Open all SMRAM regions for Host access only
  //
  Smram |= SmmOpen;
  Smram &= ~(NON_HOST_SMM_WR_OPEN | NON_HOST_SMM_RD_OPEN);

  //
  // Write the SMRAM register
  //
  QncHsmmcWrite (Smram);

  return TRUE;
}

/**
  This routine is the chipset code that accepts a request to "close" a region of SMRAM.
  The region could be legacy AB or TSEG near top of physical memory.
  The use of "close" means that the memory is only visible from SMM agents,
  not from BS or RT code.

  @retval FALSE  Cannot open a locked SMRAM region
  @retval TRUE   Success to open SMRAM region.
**/
BOOLEAN
EFIAPI
QNCCloseSmramRegion (
  VOID
  )
{
  UINT32                    Smram;
  UINT32                    SmmOpen;
  
  SmmOpen = (SMM_WRITE_OPEN | SMM_READ_OPEN | NON_HOST_SMM_WR_OPEN | NON_HOST_SMM_RD_OPEN);

	// Read the SMRAM register
	Smram = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HSMMC);
	
	//
  //  Is the platform locked?
  //
  if(Smram & SMM_LOCKED) {
    // Cannot Open a locked region
    DEBUG ((EFI_D_WARN, "Cannot close a locked SMRAM region\n"));
    return FALSE;
  }
  
  Smram &= (~SmmOpen);
  
  QncHsmmcWrite (Smram);

  return TRUE;
}

/**
  This routine is the chipset code that accepts a request to "lock" SMRAM.
  The region could be legacy AB or TSEG near top of physical memory.
  The use of "lock" means that the memory can no longer be opened
  to BS state.
**/
VOID
EFIAPI
QNCLockSmramRegion (
  VOID
  )
{
  UINT32                    Smram;

	// Read the SMRAM register
	Smram = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HSMMC);
  Smram |= SMM_LOCKED;
  
  QncHsmmcWrite (Smram);

  return;
}


/**
  Updates the PAM registers in the MCH for the requested range and mode.

  @param   Start        The start address of the memory region
  @param   Length       The length, in bytes, of the memory region
  @param   ReadEnable   Pointer to the boolean variable on whether to enable read for legacy memory section.
                        If NULL, then read attribute will not be touched by this call.
  @param   ReadEnable   Pointer to the boolean variable on whether to enable write for legacy memory section.
                        If NULL, then write attribute will not be touched by this call.
  @param   Granularity  A pointer to granularity, in bytes, that the PAM registers support

  @retval  RETURN_SUCCESS            The PAM registers in the MCH were updated
  @retval  RETURN_INVALID_PARAMETER  The memory range is not valid in legacy region.

**/
EFI_STATUS
EFIAPI
QNCLegacyRegionManipulation (
  IN  UINT32                  Start,
  IN  UINT32                  Length,
  IN  BOOLEAN                 *ReadEnable,
  IN  BOOLEAN                 *WriteEnable,
  OUT UINT32                  *Granularity
  )
{
  //
  // Do nothing cos no such support on QNC
  //
  return RETURN_SUCCESS;
}

/**
  Determine if QNC is supported.

  @retval FALSE  QNC is not supported.
  @retval TRUE   QNC is supported.
**/
BOOLEAN
IsQncSupported (
  VOID
  )
{
  UINT16  SocVendorId;
  UINT16  SocDeviceId;

  SocVendorId = MmioRead16 (
                  PciDeviceMmBase (MC_BUS,
                  MC_DEV,
                  MC_FUN) + PCI_VENDOR_ID_OFFSET
                  );

  SocDeviceId = MmioRead16 (
                  PciDeviceMmBase (MC_BUS,
                  MC_DEV,
                  MC_FUN) + PCI_DEVICE_ID_OFFSET
                  );

  //
  // Verify that this is a supported chipset
  //
  if ((SocVendorId != QUARK_MC_VENDOR_ID) || ((SocDeviceId != QUARK_MC_DEVICE_ID) && (SocDeviceId != QUARK2_MC_DEVICE_ID))) {
    DEBUG ((DEBUG_ERROR, "QNC code doesn't support the Soc VendorId:0x%04x Soc DeviceId:0x%04x!\n", SocVendorId, SocDeviceId));
    return FALSE;
  }
  return TRUE;
}

/**
  Enable SMI detection of legacy flash access violations.
**/
VOID
EFIAPI
QncEnableLegacyFlashAccessViolationSmi (
  VOID
  )
{
  UINT32  BcValue;

  BcValue = LpcPciCfg32 (R_QNC_LPC_BIOS_CNTL);

  //
  // Clear BIOSWE & set BLE.
  //
  BcValue &= (~B_QNC_LPC_BIOS_CNTL_BIOSWE);
  BcValue |= (B_QNC_LPC_BIOS_CNTL_BLE);

  LpcPciCfg32 (R_QNC_LPC_BIOS_CNTL) = BcValue;

  DEBUG ((EFI_D_INFO, "BIOS Control Lock Enabled!\n"));
}

/**
  Setup RMU Thermal sensor registers for Vref mode.
**/
VOID
EFIAPI
QNCThermalSensorSetVRefMode (
  VOID
  )
{
  UINT32                             Tscgf1Config;
  UINT32                             Tscgf2Config;
  UINT32                             Tscgf2Config2;

  Tscgf1Config = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF1_CONFIG);
  Tscgf2Config = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG);
  Tscgf2Config2 = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG2);

  Tscgf1Config &= ~(B_TSCGF1_CONFIG_ISNSCURRENTSEL_MASK);
  Tscgf1Config |= (V_TSCGF1_CONFIG_ISNSCURRENTSEL_VREF_MODE << B_TSCGF1_CONFIG_ISNSCURRENTSEL_BP);

  Tscgf1Config &= ~(B_TSCGF1_CONFIG_IBGEN);
  Tscgf1Config |= (V_TSCGF1_CONFIG_IBGEN_VREF_MODE << B_TSCGF1_CONFIG_IBGEN_BP);

  Tscgf2Config2 &= ~(B_TSCGF2_CONFIG2_ISPARECTRL_MASK);
  Tscgf2Config2 |= (V_TSCGF2_CONFIG2_ISPARECTRL_VREF_MODE << B_TSCGF2_CONFIG2_ISPARECTRL_BP);

  Tscgf2Config2 &= ~(B_TSCGF2_CONFIG2_ICALCOARSETUNE_MASK);
  Tscgf2Config2 |= (V_TSCGF2_CONFIG2_ICALCOARSETUNE_VREF_MODE << B_TSCGF2_CONFIG2_ICALCOARSETUNE_BP);

  Tscgf2Config &= ~(B_TSCGF2_CONFIG_IDSCONTROL_MASK);
  Tscgf2Config |= (V_TSCGF2_CONFIG_IDSCONTROL_VREF_MODE << B_TSCGF2_CONFIG_IDSCONTROL_BP);

  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF1_CONFIG, Tscgf1Config);
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG, Tscgf2Config);
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG2, Tscgf2Config2);
}

/**
  Setup RMU Thermal sensor registers for Ratiometric mode.
**/
VOID
EFIAPI
QNCThermalSensorSetRatiometricMode (
  VOID
  )
{
  UINT32                             Tscgf1Config;
  UINT32                             Tscgf2Config;
  UINT32                             Tscgf2Config2;
  UINT32                             Tscgf3Config;

  Tscgf1Config = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF1_CONFIG);
  Tscgf2Config = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG);
  Tscgf2Config2 = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG2);
  Tscgf3Config = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF3_CONFIG);

  Tscgf1Config &= ~(B_TSCGF1_CONFIG_ISNSCURRENTSEL_MASK);
  Tscgf1Config |= (V_TSCGF1_CONFIG_ISNSCURRENTSEL_RATIO_MODE << B_TSCGF1_CONFIG_ISNSCURRENTSEL_BP);

  Tscgf1Config &= ~(B_TSCGF1_CONFIG_ISNSCHOPSEL_MASK);
  Tscgf1Config |= (V_TSCGF1_CONFIG_ISNSCHOPSEL_RATIO_MODE  << B_TSCGF1_CONFIG_ISNSCHOPSEL_BP);

  Tscgf1Config &= ~(B_TSCGF1_CONFIG_ISNSINTERNALVREFEN);
  Tscgf1Config |= (V_TSCGF1_CONFIG_ISNSINTERNALVREFEN_RATIO_MODE << B_TSCGF1_CONFIG_ISNSINTERNALVREFEN_BP);

  Tscgf1Config &= ~(B_TSCGF1_CONFIG_IBGEN);
  Tscgf1Config |= (V_TSCGF1_CONFIG_IBGEN_RATIO_MODE << B_TSCGF1_CONFIG_IBGEN_BP);

  Tscgf1Config &= ~(B_TSCGF1_CONFIG_IBGCHOPEN);
  Tscgf1Config |= (V_TSCGF1_CONFIG_IBGCHOPEN_RATIO_MODE << B_TSCGF1_CONFIG_IBGCHOPEN_BP);

  Tscgf2Config2 &= ~(B_TSCGF2_CONFIG2_ICALCONFIGSEL_MASK);
  Tscgf2Config2 |= (V_TSCGF2_CONFIG2_ICALCONFIGSEL_RATIO_MODE << B_TSCGF2_CONFIG2_ICALCONFIGSEL_BP);

  Tscgf2Config2 &= ~(B_TSCGF2_CONFIG2_ISPARECTRL_MASK);
  Tscgf2Config2 |= (V_TSCGF2_CONFIG2_ISPARECTRL_RATIO_MODE << B_TSCGF2_CONFIG2_ISPARECTRL_BP);

  Tscgf2Config2 &= ~(B_TSCGF2_CONFIG2_ICALCOARSETUNE_MASK);
  Tscgf2Config2 |= (V_TSCGF2_CONFIG2_ICALCOARSETUNE_RATIO_MODE << B_TSCGF2_CONFIG2_ICALCOARSETUNE_BP);

  Tscgf2Config &= ~(B_TSCGF2_CONFIG_IDSCONTROL_MASK);
  Tscgf2Config |= (V_TSCGF2_CONFIG_IDSCONTROL_RATIO_MODE << B_TSCGF2_CONFIG_IDSCONTROL_BP);

  Tscgf2Config &= ~(B_TSCGF2_CONFIG_IDSTIMING_MASK);
  Tscgf2Config |= (V_TSCGF2_CONFIG_IDSTIMING_RATIO_MODE << B_TSCGF2_CONFIG_IDSTIMING_BP);

  Tscgf3Config &= ~(B_TSCGF3_CONFIG_ITSGAMMACOEFF_MASK);
  Tscgf3Config |= (V_TSCGF3_CONFIG_ITSGAMMACOEFF_RATIO_MODE << B_TSCGF3_CONFIG_ITSGAMMACOEFF_BP);

  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF1_CONFIG, Tscgf1Config);
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG, Tscgf2Config);
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF2_CONFIG2, Tscgf2Config2);
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF3_CONFIG, Tscgf3Config);
}

/**
  Setup RMU Thermal sensor trip point values.

  @param[in]  CatastrophicTripOnDegreesCelsius  - Catastrophic set trip point threshold.
  @param[in]  HotTripOnDegreesCelsius           - Hot set trip point threshold.
  @param[in]  HotTripOffDegreesCelsius          - Hot clear trip point threshold.

  @retval  EFI_SUCCESS            Trip points setup.
  @retval  EFI_INVALID_PARAMETER  Invalid trip point value.

**/
EFI_STATUS
EFIAPI
QNCThermalSensorSetTripValues (
  IN  CONST UINTN             CatastrophicTripOnDegreesCelsius,
  IN  CONST UINTN             HotTripOnDegreesCelsius,
  IN  CONST UINTN             HotTripOffDegreesCelsius
  )
{
  UINT32 RegisterValue;

  //
  // Register fields are 8-bit temperature values of granularity 1 degree C
  // where 0x00 corresponds to -50 degrees C
  // and 0xFF corresponds to 205 degrees C.
  //
  // User passes unsigned values in degrees Celsius so trips < 0 not supported.
  //
  // Add 50 to user values to get values for register fields.
  //

  if ((CatastrophicTripOnDegreesCelsius > 205) || (HotTripOnDegreesCelsius > 205) || (HotTripOffDegreesCelsius > 205)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set new values.
  //
  RegisterValue =
    ((0 + 50) << TS_CAT_TRIP_CLEAR_THOLD_BP) | // Cat Trip Clear value must be less than Cat Trip Set Value.
    ((CatastrophicTripOnDegreesCelsius + 50) << TS_CAT_TRIP_SET_THOLD_BP) |
    ((HotTripOnDegreesCelsius + 50) << TS_HOT_TRIP_SET_THOLD_BP) |
    ((HotTripOffDegreesCelsius + 50) << TS_HOT_TRIP_CLEAR_THOLD_BP)
    ;

  QNCPortWrite (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_TS_TRIP, RegisterValue);

  return EFI_SUCCESS;
}

/**
  Enable RMU Thermal sensor with a Catastrophic Trip point.

  @retval  EFI_SUCCESS            Trip points setup.
  @retval  EFI_INVALID_PARAMETER  Invalid trip point value.

**/
EFI_STATUS
EFIAPI
QNCThermalSensorEnableWithCatastrophicTrip (
  IN  CONST UINTN             CatastrophicTripOnDegreesCelsius
  )
{
  UINT32                             Tscgf3Config;
  UINT32                             TsModeReg;
  UINT32                             TsTripReg;

  //
  // Trip Register fields are 8-bit temperature values of granularity 1 degree C
  // where 0x00 corresponds to -50 degrees C
  // and 0xFF corresponds to 205 degrees C.
  //
  // User passes unsigned values in degrees Celsius so trips < 0 not supported.
  //
  // Add 50 to user values to get values for register fields.
  //

  if (CatastrophicTripOnDegreesCelsius > 205) {
    return EFI_INVALID_PARAMETER;
  }

  Tscgf3Config = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF3_CONFIG);
  TsModeReg = QNCPortRead (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_TS_MODE);
  TsTripReg = QNCPortRead (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_TS_TRIP);

  //
  // Setup Catastrophic Trip point.
  //
  TsTripReg &= ~(TS_CAT_TRIP_SET_THOLD_MASK);
  TsTripReg |= ((CatastrophicTripOnDegreesCelsius + 50) << TS_CAT_TRIP_SET_THOLD_BP);
  TsTripReg &= ~(TS_CAT_TRIP_CLEAR_THOLD_MASK);
  TsTripReg |= ((0 + 50) << TS_CAT_TRIP_CLEAR_THOLD_BP);  // Cat Trip Clear value must be less than Cat Trip Set Value.
  QNCPortWrite (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_TS_TRIP, TsTripReg);

  //
  // To enable the TS do the following:
  //    1)  Take the TS out of reset by setting itsrst to 0x0.
  //    2)  Enable the TS using RMU Thermal sensor mode register.
  //

  Tscgf3Config &= ~(B_TSCGF3_CONFIG_ITSRST);
  TsModeReg |= TS_ENABLE;

  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_TSCGF3_CONFIG, Tscgf3Config);
  QNCPortWrite (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_TS_MODE, TsModeReg);

  return EFI_SUCCESS;
}

/**
  Lock all RMU Thermal sensor control & trip point registers.

**/
VOID
EFIAPI
QNCThermalSensorLockAllRegisters (
  VOID
  )
{
  UINT32                             RegValue;
  UINT32                             LockMask;

  LockMask = TS_LOCK_THRM_CTRL_REGS_ENABLE | TS_LOCK_AUX_TRIP_PT_REGS_ENABLE;

  RegValue = QNCPortRead (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_CONFIG);
  RegValue |= LockMask;
  QNCPortWrite (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_CONFIG, RegValue);

  ASSERT ((LockMask == (QNCPortRead (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_CONFIG) & LockMask)));
}

/**
  Set chipset policy for double bit ECC error.

  @param[in]       PolicyValue  Policy to config on double bit ECC error.

**/
VOID
EFIAPI
QNCPolicyDblEccBitErr (
  IN  CONST UINT32                        PolicyValue
  )
{
  UINT32 Register;
  Register = QNCPortRead (QUARK_NC_RMU_SB_PORT_ID, QUARK_NC_RMU_REG_WDT_CONTROL);
  Register &= ~(B_WDT_CONTROL_DBL_ECC_BIT_ERR_MASK);
  Register |= PolicyValue;
  QNCPortWrite (
    QUARK_NC_RMU_SB_PORT_ID,
    QUARK_NC_RMU_REG_WDT_CONTROL,
    Register
    );
}
