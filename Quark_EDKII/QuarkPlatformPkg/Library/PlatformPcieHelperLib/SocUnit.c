/** @file
  System On Chip Unit (SOCUnit) routines.

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

/** Early initialisation of the SOC Unit

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
SocUnitEarlyInitialisation (
  VOID
  )
{
  UINT32      NewValue;

  //
  // Set the mixer load resistance
  //
  NewValue = QNCPortIORead (QUARK_SC_PCIE_AFE_SB_PORT_ID, QUARK_PCIE_AFE_PCIE_RXPICTRL0_L0);
  NewValue &= OCFGPIMIXLOAD_1_0_MASK;
  QNCPortIOWrite (QUARK_SC_PCIE_AFE_SB_PORT_ID, QUARK_PCIE_AFE_PCIE_RXPICTRL0_L0, NewValue);

  NewValue = QNCPortIORead (QUARK_SC_PCIE_AFE_SB_PORT_ID, QUARK_PCIE_AFE_PCIE_RXPICTRL0_L1);
  NewValue &= OCFGPIMIXLOAD_1_0_MASK;
  QNCPortIOWrite (QUARK_SC_PCIE_AFE_SB_PORT_ID, QUARK_PCIE_AFE_PCIE_RXPICTRL0_L1, NewValue);

  return EFI_SUCCESS;
}

/** Tasks to release PCI controller from reset pre wait for PLL Lock.

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
SocUnitReleasePcieControllerPreWaitPllLock (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  UINT32      NewValue;

  //
  // Assert PERST# and validate time assertion time.
  //
  PlatformPERSTAssert (PlatformType);
  ASSERT (PCIEXP_PERST_MIN_ASSERT_US <= (PCIEXP_DELAY_US_POST_CMNRESET_RESET + PCIEXP_DELAY_US_WAIT_PLL_LOCK + PCIEXP_DELAY_US_POST_SBI_RESET));

  //
  // PHY Common lane reset.
  //
  NewValue = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG);
  NewValue |= SOCCLKEN_CONFIG_PHY_I_CMNRESET_L;
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG, NewValue);

  //
  // Wait post common lane reset.
  //
  MicroSecondDelay (PCIEXP_DELAY_US_POST_CMNRESET_RESET);

  //
  // PHY Sideband interface reset.
  // Controller main reset
  //
  NewValue = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG);
  NewValue |= (SOCCLKEN_CONFIG_SBI_RST_100_CORE_B | SOCCLKEN_CONFIG_PHY_I_SIDE_RST_L);
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG, NewValue);

  return EFI_SUCCESS;
}

/** Tasks to release PCI controller from reset after PLL has locked

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
SocUnitReleasePcieControllerPostPllLock (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  UINT32 NewValue;

  //
  // Controller sideband interface reset.
  //
  NewValue = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG);
  NewValue |= SOCCLKEN_CONFIG_SBI_BB_RST_B;
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG, NewValue);

  //
  // Wait post sideband interface reset.
  //
  MicroSecondDelay (PCIEXP_DELAY_US_POST_SBI_RESET);

  //
  // Deassert PERST#.
  //
  PlatformPERSTDeAssert (PlatformType);

  //
  // Wait post de assert PERST#.
  //
  MicroSecondDelay (PCIEXP_DELAY_US_POST_PERST_DEASSERT);

  //
  // Controller primary interface reset.
  //
  NewValue = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG);
  NewValue |= SOCCLKEN_CONFIG_BB_RST_B;
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_SOCCLKEN_CONFIG, NewValue);

  return EFI_SUCCESS;
}

