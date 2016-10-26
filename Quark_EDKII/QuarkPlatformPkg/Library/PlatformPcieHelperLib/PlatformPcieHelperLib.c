/** @file
  Platform Pcie Helper Lib.

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

//
// Routines local to this source module.
//
STATIC
VOID
LegacyGpioSetLevel (
  IN CONST UINT32                         LevelRegOffset,
  IN CONST UINT32                         GpioNum,
  IN CONST BOOLEAN                        HighLevel
  )
{
  UINT32                            RegValue;
  UINT32                            GpioBaseAddress;
  UINT32                            GpioNumMask;

  GpioBaseAddress =  LpcPciCfg32 (R_QNC_LPC_GBA_BASE) & B_QNC_LPC_GPA_BASE_MASK;
  ASSERT (GpioBaseAddress > 0);

  RegValue = IoRead32 (GpioBaseAddress + LevelRegOffset);
  GpioNumMask = (1 << GpioNum);
  if (HighLevel) {
    RegValue |= (GpioNumMask);
  } else {
    RegValue &= ~(GpioNumMask);
  }
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGLVL_RESUME_WELL, RegValue);
}

//
// Routines exported by this component.
//

/**
  Platform assert PCI express PERST# signal.

  @param   PlatformType     See EFI_PLATFORM_TYPE enum definitions.

**/
VOID
EFIAPI
PlatformPERSTAssert (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  if (PlatformType == GalileoGen2) {
    LegacyGpioSetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, GALILEO_GEN2_PCIEXP_PERST_RESUMEWELL_GPIO, FALSE);
  } else {
    LegacyGpioSetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, PCIEXP_PERST_RESUMEWELL_GPIO, FALSE);
  }
}

/**
  Platform de assert PCI express PERST# signal.

  @param   PlatformType     See EFI_PLATFORM_TYPE enum definitions.

**/
VOID
EFIAPI
PlatformPERSTDeAssert (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  if (PlatformType == GalileoGen2) {
    LegacyGpioSetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, GALILEO_GEN2_PCIEXP_PERST_RESUMEWELL_GPIO, TRUE);
  } else {
    LegacyGpioSetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, PCIEXP_PERST_RESUMEWELL_GPIO, TRUE);
  }
}

/** Early initialisation of the PCIe controller.

  @param   PlatformType     See EFI_PLATFORM_TYPE enum definitions.

  @retval   EFI_SUCCESS               Operation success.

**/
EFI_STATUS
EFIAPI
PlatformPciExpressEarlyInit (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{

  //
  // Release and wait for PCI controller to come out of reset.
  //
  SocUnitReleasePcieControllerPreWaitPllLock (PlatformType);
  MicroSecondDelay (PCIEXP_DELAY_US_WAIT_PLL_LOCK);
  SocUnitReleasePcieControllerPostPllLock (PlatformType);

  //
  // Early PCIe initialisation
  //
  SocUnitEarlyInitialisation ();

  //
  // Do North cluster early PCIe init.
  //
  PciExpressEarlyInit ();

  return EFI_SUCCESS;
}

