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

  PlatformSecServicesLib.c

Abstract:

  Provide platform specific services to QuarkResetVector & QuarkSecLib.

--*/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/MfhLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/QNCAccessLib.h>
#include <Platform.h>

typedef VOID (EFIAPI *PLATFORM_BOOT_IMAGE) (VOID);

//
// Routines local to this source module.
//

/** Copy boot image to sram and call it.

**/
STATIC
VOID
CopyImageToSramAndCall (
  IN CONST UINT32                         FlashAddress,
  IN CONST UINT32                         LengthBytes
  )
{
  volatile UINT32                   Index;
  UINT32                            *Dest;
  UINT32                            *Src;
  UINT32                            OffsetLocation;
  UINT32                            SramStage1Base;
  PLATFORM_BOOT_IMAGE               BootImage;

  SramStage1Base = PcdGet32 (PcdEsramStage1Base);
  Dest = (UINT32 *) SramStage1Base;
  Src = (UINT32 *) FlashAddress;
  for (Index = 0; Index < (LengthBytes / 4); Index++, Dest++, Src++) {
    *Dest = *Src;
  }
  OffsetLocation = (SramStage1Base + PcdGet32 (PcdFvSecurityHeaderSize));
  BootImage = (PLATFORM_BOOT_IMAGE) ((*((UINT32 *) OffsetLocation)) + OffsetLocation);

  //
  // Boot to image at FlashAddress.
  //
  BootImage ();

  //
  // We should never return if we do then loop forever.
  //
  for (Index = 0; Index == 0;);
}

//
// Routines exported by this source module.
//

/** Copy and call fixed recovery image.

**/
VOID
EFIAPI
PlatformCopyFixedRecoveryImageToSramAndCall (
  VOID
  )
{
  volatile UINT32                   Index;
  UINT32                            *Dest;
  UINT32                            *Src;
  UINT32                            OffsetLocation;
  volatile UINT32                   SramStage1Base;
  volatile UINT32                   HeaderSize;
  volatile UINT32                   CopyLength;
  PLATFORM_BOOT_IMAGE               BootImage;

  //
  // Make all locals derived from PCDs volatile so that they
  // are put on the stack before the copy.
  //
  SramStage1Base = PcdGet32 (PcdEsramStage1Base);
  HeaderSize = PcdGet32 (PcdFvSecurityHeaderSize);
  Dest = (UINT32 *) SramStage1Base;
  Src = (UINT32 *) PcdGet32 (PcdFlashFvFixedStage1AreaBase);
  CopyLength = (PcdGet32 (PcdFlashFvFixedStage1AreaSize) / 4);

  for (Index = 0; Index < CopyLength; Index++, Dest++, Src++) {
    *Dest = *Src;
  }
  OffsetLocation = (SramStage1Base + HeaderSize);
  BootImage = (PLATFORM_BOOT_IMAGE) ((*((UINT32 *) OffsetLocation)) + OffsetLocation);

  //
  // Boot to fixed recovery image.
  //
  BootImage ();

  //
  // We should never return if we do then loop forever.
  //
  for (Index = 0; Index == 0;);
}

/** Check if running recovery image in SRAM.

  @retval  TRUE    If running recovery image from SRAM.
  @retval  FALSE   If not recovery image from SRAM.
**/
BOOLEAN
EFIAPI
PlatformAmIFixedRecoveryRunningFromSram (
  VOID
  )
{
  UINT32                            CheckAddr;
  QUARK_EDKII_STAGE1_HEADER         *Edk2ImageHeader;

  CheckAddr = (UINT32) &PlatformAmIFixedRecoveryRunningFromSram;
  if ((CheckAddr >= PcdGet32 (PcdEsramStage1Base)) && (CheckAddr < (PcdGet32 (PcdEsramStage1Base) + PcdGet32 (PcdESramMemorySize)))) {
    Edk2ImageHeader = (QUARK_EDKII_STAGE1_HEADER *) (PcdGet32 (PcdEsramStage1Base) + PcdGet32 (PcdFvSecurityHeaderSize));
    return ((UINT8)Edk2ImageHeader->ImageIndex & QUARK_STAGE1_IMAGE_TYPE_MASK) == QUARK_STAGE1_RECOVERY_IMAGE_TYPE;
  }
  return FALSE;
}

/** Check if force recovery conditions met.

  @return 0 if conditions not met or already running recovery image in SRAM.
  @return address PlatformCopyFixedRecoveryImageToSramAndCall if conditions met.
**/
UINT32
EFIAPI
PlatformIsForceRecoveryConditionsMet (
  VOID
  )
{
  UINT32                            Sticky32;
  UINT32                            Straps32;

  //
  // Check for 'Force Recovery Boot' by reading 'B_CFG_STICKY_RW_FORCE_RECOVERY'
  //
  Sticky32 = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_CFG_STICKY_RW);
  Straps32 = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_STPDDRCFG);

  if ((Sticky32 & B_CFG_STICKY_RW_FORCE_RECOVERY) || ((Straps32 & B_STPDDRCFG_FORCE_RECOVERY) == 0)) {
    //
    // Clear sticky bit 'B_CFG_STICKY_RW_FORCE_RECOVERY'
    //
    Sticky32 &= ~B_CFG_STICKY_RW_FORCE_RECOVERY;
    QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_CFG_STICKY_RW, Sticky32);

    //
    // Check if already running recovery image.
    //
    if (PlatformAmIFixedRecoveryRunningFromSram ()) {
      //
      // Already running recovery, do nothing.
      //
      return 0;
    }

    //
    // Indicate conditions met by returning address of PlatformCopyFixedRecoveryImageToSramAndCall.
    //
    return (UINT32) &PlatformCopyFixedRecoveryImageToSramAndCall;
  }

  return 0;
}

/** Copy boot stage1 image to sram and Call it.

  This routine is called from the QuarkResetVector to copy stage1 image to SRAM
  the routine should NOT be called from QuarkSecLib.

**/
VOID
EFIAPI
PlatformCopyBootStage1ImageToSramAndCall (
  VOID
  )
{
  MFH_FLASH_ITEM                    FlashItem;

  //
  // Find First stage1 flash module info or fixed recovery module info.
  //
  if (!MfhLibFindFirstStage1OrFixedRecovery (&FlashItem, NULL)) {
    //
    // No stage1 boot image found, force call to fixed recovery.
    //
    PlatformCopyFixedRecoveryImageToSramAndCall ();
  }

  //
  // Copy MFH specified image to SRAM and call it.
  //
  CopyImageToSramAndCall (
    FlashItem.FlashAddress,
    FlashItem.LengthBytes
    );
}
