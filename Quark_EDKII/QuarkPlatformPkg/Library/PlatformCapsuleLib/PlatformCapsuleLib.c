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

Abstract:
  Capsule Library instance to update capsule image to flash. 

**/
#include <PiDxe.h>

#include <Guid/QuarkCapsuleGuid.h>
#include <Protocol/FirmwareVolumeBlockSecurity.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CapsuleLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/QNCAccessLib.h>

extern	VOID SendCapsuleSmi(UINTN Addr);
extern  VOID GetUpdateStatusSmi(UINTN Addr);

/**
  Those capsules supported by the firmwares.

  @param  CapsuleHeader    Points to a capsule header.

  @retval EFI_SUCESS       Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
SupportCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  if (CompareGuid (&gEfiQuarkCapsuleGuid, &CapsuleHeader->CapsuleGuid)) {
    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  The firmware implements to process the capsule image.

  @param  CapsuleHeader         Points to a capsule header.
  
  @retval EFI_SUCESS            Process Capsule Image successfully. 
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
EFIAPI
ProcessCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  CAPSULE_INFO_PACKET                     *CapsuleInfoPacketPtr;
  CAPSULE_FRAGMENT                        *CapsuleFragmentPtr;
  UPDATE_STATUS_PACKET                    *UpdateStatusPacketPtr;
  EFI_STATUS                              Status;
  UINT16                                  WdtBaseAddress;
  STATIC UINT8                            WdtConfigRegister; 
  UINT8                                   WdtResetEnabled;
  UINT8                                   WdtLockEnabled; 
  FIRMWARE_VOLUME_BLOCK_SECURITY_PROTOCOL *FvbSecurityProtocol;
  EFI_HOB_GUID_TYPE                       *SecurityHeaderHob;
  VOID                                    *SecurityHeader;
  UINTN                                   TempSize;
  UINT8                                   *AuthImage;

  TempSize = 0;

  if (SupportCapsuleImage (CapsuleHeader) != EFI_SUCCESS) {
    return EFI_UNSUPPORTED;
  }

  //
  // Locate Fvb Security Protocol for Capsule File Verification.
  //
  Status = gBS->LocateProtocol(
             &gFirmwareVolumeBlockSecurityGuid,
             NULL,
             (VOID**) &FvbSecurityProtocol
             );
  ASSERT_EFI_ERROR (Status);
  ASSERT (FvbSecurityProtocol != NULL);
  ASSERT (FvbSecurityProtocol->SecurityAuthenticateImage != NULL);

  if (FvbSecurityProtocol != NULL) {
    //
    // If NoReset Capsule we should have a security header prefix to CapHdr.
    //
    Status = FvbSecurityProtocol->SecurityAuthenticateImage ((VOID *) CapsuleHeader);
    if (EFI_ERROR(Status)) {
      //
      // If Reset Capsule get SecHdr from HOB created at PEI stage.
      //
      SecurityHeaderHob = GetFirstGuidHob (&gEfiQuarkCapsuleSecurityHeaderGuid);
      if (SecurityHeaderHob != NULL) {
        SecurityHeader = GET_GUID_HOB_DATA (SecurityHeaderHob);
        if (SecurityHeader != NULL) {
          DEBUG ((EFI_D_INFO, "ProcessCapsuleImage: SecHdrPtr:SigId 0x%8X:0x%8X \n",
           (UINTN) SecurityHeader,
           (UINTN) *((UINT32 *) SecurityHeader)
           ));

          TempSize =
            PcdGet32 (PcdFvSecurityHeaderSize) +
            CapsuleHeader->HeaderSize +
            CapsuleHeader->CapsuleImageSize;

           AuthImage = AllocatePool (TempSize);
           ASSERT (AuthImage != NULL);

           CopyMem (
             (VOID *) AuthImage,
             SecurityHeader,
             PcdGet32 (PcdFvSecurityHeaderSize)
             );

           TempSize -= PcdGet32 (PcdFvSecurityHeaderSize);
           CopyMem (
             (VOID *) &AuthImage[PcdGet32 (PcdFvSecurityHeaderSize)],
             (VOID *) CapsuleHeader,
             TempSize);

           Status = FvbSecurityProtocol->SecurityAuthenticateImage (
                                           (VOID *) &AuthImage[PcdGet32 (PcdFvSecurityHeaderSize)]
                                           );

           FreePool (AuthImage);
        }
      }
    }
    ASSERT_EFI_ERROR (Status);
  }

  CapsuleFragmentPtr    = (CAPSULE_FRAGMENT *) AllocateZeroPool (sizeof (CAPSULE_FRAGMENT));
  CapsuleInfoPacketPtr  = (CAPSULE_INFO_PACKET *) AllocateZeroPool (sizeof (CAPSULE_INFO_PACKET));
  UpdateStatusPacketPtr = (UPDATE_STATUS_PACKET *) AllocateZeroPool (sizeof (UPDATE_STATUS_PACKET));
  DEBUG ((EFI_D_INFO, "CapsuleImage Address is %10p, CapsuleImage Size is %8x\n", CapsuleHeader, CapsuleHeader->CapsuleImageSize));
  DEBUG ((EFI_D_INFO, "CapsuleFragment Address is %10p, CapsuleInfo Address is %10p\n", CapsuleFragmentPtr, CapsuleInfoPacketPtr));

  //
  // Prepare structures to store capsule image.
  //

  CapsuleFragmentPtr->Address      = (UINTN) CapsuleHeader;
  CapsuleFragmentPtr->BufferOffset = 0;
  CapsuleFragmentPtr->Size         = CapsuleHeader->CapsuleImageSize;
  CapsuleFragmentPtr->Flags        = BIT0;

  CapsuleInfoPacketPtr->CapsuleLocation = (UINTN) CapsuleFragmentPtr;
  CapsuleInfoPacketPtr->CapsuleSize     = CapsuleHeader->CapsuleImageSize;
  CapsuleInfoPacketPtr->Status          = EFI_SUCCESS;
  
  Print (L"Start to update capsule image!\n");

  //
  // Check if WDT reset and lock are enabled
  //
  WdtBaseAddress = (LpcPciCfg16(R_QNC_LPC_WDTBA));
  WdtConfigRegister = IoRead8((WdtBaseAddress + R_QNC_LPC_WDT_WDTCR));
  WdtResetEnabled = WdtConfigRegister & BIT4;
  WdtLockEnabled = IoRead8((WdtBaseAddress + R_QNC_LPC_WDT_WDTLR)) & BIT0;
  ASSERT ((WdtLockEnabled == 0 && WdtResetEnabled == 0));
  
  //
  // Explicitly clear WDT Reset Enable
  //
  IoWrite8((WdtBaseAddress + R_QNC_LPC_WDT_WDTCR), (WdtConfigRegister & ~BIT4));
  
  //
  // Trig SMI to update Capsule image.
  //
  SendCapsuleSmi((UINTN)CapsuleInfoPacketPtr);

  UpdateStatusPacketPtr->BlocksCompleted = 0;
  UpdateStatusPacketPtr->TotalBlocks = (UINTN) -1;
  UpdateStatusPacketPtr->Status = EFI_SUCCESS;

  do {
    //
    // Trig SMI to get the status to updating capsule image.
    //
    gBS->Stall (200000);
    GetUpdateStatusSmi((UINTN)UpdateStatusPacketPtr);

    if (UpdateStatusPacketPtr->TotalBlocks != (UINTN) -1) {
      Print (L"Updated Blocks completed: %d of %d\n", UpdateStatusPacketPtr->BlocksCompleted, UpdateStatusPacketPtr->TotalBlocks);
    }
  } while ((UpdateStatusPacketPtr->Status == EFI_SUCCESS) && (UpdateStatusPacketPtr->BlocksCompleted < UpdateStatusPacketPtr->TotalBlocks));
  
  //
  // Restore Previous WDT Reset Enable setting
  //
  IoWrite8((WdtBaseAddress + R_QNC_LPC_WDT_WDTCR), (WdtConfigRegister));


  Status = UpdateStatusPacketPtr->Status;
  if (EFI_ERROR (Status)) {
    Print (L"Invalid capsule format. Please furnish a valid capsule. Return status is %r!\n", UpdateStatusPacketPtr->Status);
  } else {
    Print (L"Capsule image is updated done!\n");
  }
  
  FreePool (CapsuleFragmentPtr);
  FreePool (CapsuleInfoPacketPtr);
  FreePool (UpdateStatusPacketPtr);

  return Status;
}

