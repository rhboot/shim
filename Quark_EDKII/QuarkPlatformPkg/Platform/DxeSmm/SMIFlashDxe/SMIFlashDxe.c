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

  SMIFlashDxe.c

Abstract:

 SMI driver to update BIOS capsule.

--*/

#include "SMIFlashDxe.h"

EFI_STATUS ChallengeCapsule();
EFI_STATUS CapsuleParse();
EFI_STATUS EraseBlock();
EFI_STATUS WriteBlock();

#define MAX_BLOCK_NUMBER FixedPcdGet32(PcdMaxSizeNonPopulateCapsule) / FLASH_BLOCK_SIZE

UINTN SmiFlashUpdateStateArray[SMI_FLASH_UPDATE_MAX_STATES] = {
  (UINTN)&ChallengeCapsule,  
  (UINTN)&CapsuleParse,
  (UINTN)&EraseBlock,
  (UINTN)&WriteBlock,
  (UINTN)-1,
};

SPI_IMAGE_PRESENT	mSpiImageList[] = {
  //           DefaultBaseAddress                             MaxSize                 FixedAddress     Updatable   AnyBlockUpdate   Present      
   {EDKII_BOOTROM_OVERRIDE_DEFAULT_BASE_ADDRESS,    EDKII_BOOTROM_OVERRIDE_MAX_SIZE,    TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_KEY_MODULE_DEFAULT_BASE_ADDRESS,          EDKII_KEY_MODULE_MAX_SIZE,          TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_SVN_DEFAULT_BASE_ADDRESS,                 EDKII_SVN_MAX_SIZE,                 TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_RECOVERY_IMAGE1_DEFAULT_BASE_ADDRESS,     EDKII_RECOVERY_IMAGE1_MAX_SIZE,     TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_OEM_AREA_DEFAULT_BASE_ADDRESS,            EDKII_OEM_AREA_MAX_SIZE,            TRUE,           TRUE,       TRUE,           FALSE}
  ,{EDKII_NVRAM_DEFAULT_BASE_ADDRESS,               EDKII_NVRAM_MAX_SIZE,               TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_PLATFORM_DATA_DEFAULT_BASE_ADDRESS,       EDKII_PLATFORM_DATA_MAX_SIZE,       TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_MFH_DEFAULT_BASE_ADDRESS,                 EDKII_MFH_MAX_SIZE,                 TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_RMU_DEFAULT_BASE_ADDRESS,                 EDKII_RMU_MAX_SIZE,                 TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_BOOT_STAGE1_IMAGE1_DEFAULT_BASE_ADDRESS,  EDKII_BOOT_STAGE1_IMAGE1_MAX_SIZE,  TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_BOOT_STAGE1_IMAGE2_DEFAULT_BASE_ADDRESS,  EDKII_BOOT_STAGE1_IMAGE2_MAX_SIZE,  TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_BOOT_STAGE2_COMPACT_DEFAULT_BASE_ADDRESS, EDKII_BOOT_STAGE2_COMPACT_MAX_SIZE, TRUE,           TRUE,       FALSE,          FALSE}
  ,{EDKII_OS_DEFAULT_BASE_ADDRESS,                  EDKII_OS_MAX_SIZE,                  TRUE,           TRUE,       TRUE,           FALSE}
};

UINT8    mSpiImageListSize;

//
// Module Globals
//
EFI_SPI_PROTOCOL                            *mEfiSpiProtocol = NULL;
EFI_SMM_CPU_PROTOCOL                        *mSmmCpu = NULL;
SPI_INIT_INFO                               *mSpiCtrlerInfo = NULL;

UPDATE_STATUS_PACKET                        mUpdateStatusPacket = {
	0x0,
	(UINTN) -1,
	EFI_NOT_READY
};

NEXT_STATE				                          mNextState;
UINTN				                                mStateId;
SMI_FLASH_STATE_MANAGER		                  mActiveState;
STATE_MACHINE_PRIVATE_DATA                  mStateMachinePrivateData[MAX_BLOCK_NUMBER];
UINTN                                       mCurrBlock = 0;
BOOLEAN                                     mLastBlockDone = FALSE;
UINT8					                              *mCapsuleSmmBuffer = NULL;
UINT32					                            mBufferOffset=0;
UINT32                                      mSpiFlashBaseAddress = 0;

/**
  Calculate the total size for all images in Capsule.

  @param  CapsuleHeader    Pointer to the capsule image.
  @param  CapsuleImageSize Pointer to the Capsule image size. On output, it stores the got Capsule image size.

  @retval EFI_INVALID_PARAMETER The input parameter is NULL. Or Capsule image is not supported format.
  @retval EFI_SUCCESS           Calculate the total size for all images stored in Capsule image.

**/
EFI_STATUS
FindTotalCapsuleImageSize (
  IN EFI_CAPSULE_HEADER 	*CapsuleHeader,
  OUT UINT32	            *CapsuleImageSize
  ) 
{
  EFI_STATUS                  Status;
  UPDATE_HINT                 *HintTable;

  Status = EFI_SUCCESS;

  //
  // Check buffers are valid
  //
  if (CapsuleHeader == NULL || CapsuleImageSize == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto _Exit;
  }

  HintTable  = (UPDATE_HINT*) ((UINT8*) CapsuleHeader + CapsuleHeader->HeaderSize);

  //
  // Add the size of each image in the capsule
  //
  *CapsuleImageSize = (sizeof(UPDATE_HINT) * MAX_IMAGES_PER_CAPSULE);     // Hint table size
  while(HintTable->Size != 0x00){
    *CapsuleImageSize += HintTable->Size;
    HintTable++;
  }

_Exit:
  return Status;
}

/**
  Check if we need to preserve any items in the Platform Data area

  @param  CapsulePlatformData    Pointer to the capsule image Platform Data.

  @retval EFI_SUCCESS           Platform Data policy implemented successfully.

**/
EFI_STATUS
CheckPlatformDataPolicy (
  IN PDAT_AREA 	          *CapsulePlatformData,
  IN EFI_CAPSULE_HEADER   *CapsuleHeader
  ) 
{
  EFI_STATUS                Status;
  PDAT_ITEM                 *PDataFlashItem;
  PDAT_ITEM                 *PDataCapsuleItem;
  PDAT_LIB_FINDCONTEXT      PDatCapsuleFindContext;
  PDAT_LIB_FINDCONTEXT      PDatFlashFindContext;
  UINT32                    CalcCrc;

  Status = EFI_SUCCESS;

  //
  // Check if we should preserve the PlatformData MAC addresses
  //
  if (!(CapsuleHeader->Flags & PD_UPDATE_MAC)) {
    //
    // Find MAC1 in PlatformData in both SPI flash and capsule
    // Copy MAC1 from the SPI flash PlatformData to the capsule PlatformData
    //
    PDataCapsuleItem = PDatLibFindFirstWithFilter ((PDAT_AREA *)CapsulePlatformData, PDAT_FIND_NETWORK_ITEMS, &PDatCapsuleFindContext, NULL);
    PDataFlashItem = PDatLibFindFirstWithFilter (NULL, PDAT_FIND_NETWORK_ITEMS, &PDatFlashFindContext, NULL);
    if ((PDataFlashItem != NULL) && (PDataCapsuleItem != NULL)) {
      CopyMem (PDataCapsuleItem->Data, PDataFlashItem->Data, PDataFlashItem->Header.Length);
    } else {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Find MAC2 in PlatformData in both SPI flash and capsule
    // Copy MAC2 from the SPI flash PlatformData to the capsule PlatformData
    //
    PDataCapsuleItem = PDatLibFindNextWithFilter(PDAT_FIND_NETWORK_ITEMS, &PDatCapsuleFindContext, NULL);
    PDataFlashItem = PDatLibFindNextWithFilter(PDAT_FIND_NETWORK_ITEMS, &PDatFlashFindContext, NULL);
    if ((PDataFlashItem != NULL) && (PDataCapsuleItem != NULL)) {
      CopyMem (PDataCapsuleItem->Data, PDataFlashItem->Data, PDataFlashItem->Header.Length);
    } else {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Update the PlatformData CRC in the capsule
    //
    Status = PlatformCalculateCrc32 (
              (UINT8 *) &CapsulePlatformData->Body[0],
              (UINTN) CapsulePlatformData->Header.Length,
              &CalcCrc
              );
    CapsulePlatformData->Header.Crc = CalcCrc;
  }

  return Status;
}

/**
  Initialize State machine to store each block information.

  @param  StateElement     Pointer to State machine element. On output, it will record each FV block.
  @param  CapsuleHeader    Pointer to the capsule image.
  @param  HintTable        Pointer to HintTable that stores each FV information.

  @return the total number of the recorded FV block.

**/
UINT32
InitialiseStateMachineData (
  IN OUT  STATE_MACHINE_PRIVATE_DATA  	**StateElement,
  IN EFI_CAPSULE_HEADER                 *CapsuleHeader,
  IN UPDATE_HINT                        *HintTable
  )
{
  UINT32            ImageSourceAddr;
  UINT32            ImageDestinationAddr;
  UINT32            NumberOfBlocks;
  
  ASSERT(mEfiSpiProtocol != NULL);

  ImageSourceAddr = (UINT32)((UINT8*) CapsuleHeader + HintTable->SourceOffset);
  ImageDestinationAddr = HintTable->TargetAddr;
  ASSERT ((ImageDestinationAddr & (FLASH_BLOCK_SIZE-1)) == 0);      // Check that the destination SPI address is block aligned
  NumberOfBlocks = 0;

  while (ImageDestinationAddr <= ((HintTable->TargetAddr + HintTable->Size)-FLASH_BLOCK_SIZE) && (ImageDestinationAddr != 0)) {
    ASSERT (ImageDestinationAddr >= mSpiFlashBaseAddress);

    if (ImageDestinationAddr == EDKII_PLATFORM_DATA_DEFAULT_BASE_ADDRESS) {
      CheckPlatformDataPolicy((PDAT_AREA *)ImageSourceAddr, CapsuleHeader);
    }

    ((STATE_MACHINE_PRIVATE_DATA*)*StateElement)->SourceAddr      = ImageSourceAddr;
    ((STATE_MACHINE_PRIVATE_DATA*)*StateElement)->TargetAddr      = ImageDestinationAddr;
    ((STATE_MACHINE_PRIVATE_DATA*)*StateElement)->BlockSize       = FLASH_BLOCK_SIZE;
    ((STATE_MACHINE_PRIVATE_DATA*)*StateElement)->IsValid         = TRUE;
    ((STATE_MACHINE_PRIVATE_DATA*)*StateElement)->UpdateStatus    = FALSE;
    ((STATE_MACHINE_PRIVATE_DATA*)*StateElement)->SmmSpiProtocol  = mEfiSpiProtocol;

    *StateElement = ( (STATE_MACHINE_PRIVATE_DATA*) *StateElement ) + 1;
    ImageSourceAddr += FLASH_BLOCK_SIZE;
    ImageDestinationAddr += FLASH_BLOCK_SIZE;
    NumberOfBlocks++;
  }

  return NumberOfBlocks;
}

/**
  Parse Capsule image, and record each block of the updated FV into the state machine structure.

  @retval EFI_OUT_OF_RESOURCES   No enough buffer to store capsule image.
  @retval EFI_SUCCESS            Parse Capsule image successfully.
  
**/
EFI_STATUS
CapsuleParse(
  VOID
  )
{
  UINT8                       Index;
  EFI_CAPSULE_HEADER          *CapsuleHeader;
  UPDATE_HINT                 *HintTable;
  STATE_MACHINE_PRIVATE_DATA  *StateElement;

  if (mCapsuleSmmBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CapsuleHeader = (EFI_CAPSULE_HEADER *) mCapsuleSmmBuffer;
  HintTable     = (UPDATE_HINT*)((UINT8*) CapsuleHeader + CapsuleHeader->HeaderSize);
  StateElement  = &mStateMachinePrivateData[0];

  mUpdateStatusPacket.TotalBlocks = 0;

  //
  // Check for full SPI flash update first
  // Also allow full update of 'BIOS_SECTION'
  //
  if ((HintTable->Size == (SIZE_4GB - mSpiFlashBaseAddress)) && (HintTable->TargetAddr == mSpiFlashBaseAddress)) {
    mUpdateStatusPacket.TotalBlocks += InitialiseStateMachineData (&StateElement, CapsuleHeader, HintTable); 
  } else if ((HintTable->Size == EDKII_BIOS_SECTION_MAX_SIZE) && (HintTable->TargetAddr == EDKII_BIOS_SECTION_DEFAULT_BASE_ADDRESS)) {
    mUpdateStatusPacket.TotalBlocks += InitialiseStateMachineData (&StateElement, CapsuleHeader, HintTable); 
  } else {
    while(HintTable->Size != 0x00 && mUpdateStatusPacket.TotalBlocks < MAX_BLOCK_NUMBER) {
      if (HintTable->TargetAddr < mSpiFlashBaseAddress ||
         (UINT64)HintTable->TargetAddr + (UINT64)HintTable->Size > SIZE_4GB) {
        //
        // Only support to update Flash region (between mSpiFlashBaseAddress and SIZE_4GB). 
        //
        return EFI_INVALID_PARAMETER;    
      }
    
      //
      // Only update SPI flash images that can be updated.
      //
      for (Index = 0; Index < mSpiImageListSize; Index++) {
        if (mSpiImageList[Index].FixedAddress) {
          if ((HintTable->TargetAddr >= mSpiImageList[Index].SpiImageDefaultBaseAddress) &&
              (((UINT64)HintTable->TargetAddr + (UINT64)HintTable->Size) <= ((UINT64)mSpiImageList[Index].SpiImageDefaultBaseAddress + (UINT64)mSpiImageList[Index].SpiImageMaxSize)) &&
              mSpiImageList[Index].Updatable) {

            if (mSpiImageList[Index].AnyBlockUpdate) {
              mUpdateStatusPacket.TotalBlocks += InitialiseStateMachineData (&StateElement, CapsuleHeader, HintTable); 
            } else if (HintTable->TargetAddr == mSpiImageList[Index].SpiImageDefaultBaseAddress) {
              mUpdateStatusPacket.TotalBlocks += InitialiseStateMachineData (&StateElement, CapsuleHeader, HintTable); 
            } else {
              return EFI_INVALID_PARAMETER;    
            }
          }

        } else {
         //
         // Validate this image using the MFH
         //
//          if ((MfhValidateImageBaseAddress(HintTable->TargetAddr, (UINT32)((UINT8*) CapsuleHeader + HintTable->SourceOffset))) &&
//              (HintTable->Size <= mSpiImageList[Index].SpiImageMaxSize) && (mSpiImageList[Index].Updatable)) {
//            //
//            // If so, update SM flash data
//            //
//            mUpdateStatusPacket.TotalBlocks += InitialiseStateMachineData (&StateElement, CapsuleHeader, HintTable); 
//          } else {
//            return EFI_INVALID_PARAMETER;    
//          }
        }
      }
      HintTable++;
    }
  }
  
  //
  // Block number can't exceed the reserved max block number. 
  //
  ASSERT (mUpdateStatusPacket.TotalBlocks < MAX_BLOCK_NUMBER);
 
  mCurrBlock = 0;
  mUpdateStatusPacket.Status = EFI_SUCCESS;
  return EFI_SUCCESS;
}

/**
  Verify whether the capsule image is the supported and valid capsule image.

  @retval EFI_OUT_OF_RESOURCES   No enough memory to store Capsule image.
  @retval EFI_INVALID_PARAMETER  Capsule image format is unsupported. 
  @retval EFI_SUCCESS            Get the updated Capsule images.
  
**/
EFI_STATUS 
ChallengeCapsule (
  VOID
  )
{
  INT8                        *Buffer;
  EFI_CAPSULE_HEADER          *CapsuleHeader;
  EFI_STATUS                  Status;
  UINT32		                  TotalCapsuleImageSize;

  if (mCapsuleSmmBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Get Capsule size
  //
  Buffer        = mCapsuleSmmBuffer;
  CapsuleHeader = (EFI_CAPSULE_HEADER *) Buffer;

  //
  // Check the format of capsule file
  //
  if (!CompareGuid (&CapsuleHeader->CapsuleGuid, &gEfiQuarkCapsuleGuid)) {
    //
    // unsupported capsule image
    //
    return EFI_INVALID_PARAMETER;
  }

  Status = FindTotalCapsuleImageSize (CapsuleHeader, &TotalCapsuleImageSize);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Capsule image size check.
  //
  if (CapsuleHeader->CapsuleImageSize != (CapsuleHeader->HeaderSize + TotalCapsuleImageSize)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Erase current block specified by the glboal mCurrBlock variable until all blocks are erased. 

  @retval EFI_SUCCESS    Erase current block successfully. 
  
**/
EFI_STATUS 
EraseBlock (
  VOID
  )
{
  EFI_STATUS    Status;
  UINTN         WriteAddress;

  if (!mStateMachinePrivateData[mCurrBlock].IsValid) {
    mLastBlockDone = TRUE;
    return EFI_SUCCESS;
  }

  //
  // Erase Flash
  //
  WriteAddress = (UINTN)(mStateMachinePrivateData[mCurrBlock].TargetAddr - mSpiFlashBaseAddress);
  Status = mStateMachinePrivateData[mCurrBlock].SmmSpiProtocol->Execute (
                                          mStateMachinePrivateData[mCurrBlock].SmmSpiProtocol,
                                          mSpiCtrlerInfo->EraseOpcodeIndex,  // OpcodeIndex
                                          0,                                 // PrefixOpcodeIndex
                                          FALSE,                             // DataCycle
                                          TRUE,                              // Atomic
                                          FALSE,                             // ShiftOut
                                          WriteAddress,                      // Address
                                          0,                                 // Data Number
                                          NULL,
                                          EnumSpiRegionAll                   // SPI_REGION_TYPE
                                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SMIFlashDxe: Erase Flash Error WriteAddress=0x%08x TargetAddr=0x%08x, mSpiFlashBaseAddress=0x%08x\n",
      (UINTN) WriteAddress,
      (UINTN) mStateMachinePrivateData[mCurrBlock].TargetAddr,
      (UINTN) mSpiFlashBaseAddress
      ));
  }
  //
  // Flash needs to be erased correctly. 
  //
  ASSERT_EFI_ERROR (Status);

  //
  // Flash write is always enabled. Todo fix. Need to disable flash write first after erase done.
  //
  return EFI_SUCCESS;
}

/**
  Write the update data into the current block specified by the glboal mCurrBlock variable. 

  @retval EFI_SUCCESS    Write data into flash block correctly.
**/
EFI_STATUS 
WriteBlock (
  VOID
  )
{
  EFI_STATUS    Status;
  UINTN         WriteAddress;

  //
  // Write Flash
  //
  if (!mStateMachinePrivateData[mCurrBlock].IsValid) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Write Flash
  //
  WriteAddress = (UINTN)(mStateMachinePrivateData[mCurrBlock].TargetAddr - mSpiFlashBaseAddress);
  Status = mStateMachinePrivateData[mCurrBlock].SmmSpiProtocol->Execute (
                                          mStateMachinePrivateData[mCurrBlock].SmmSpiProtocol,
                                          mSpiCtrlerInfo->ProgramOpcodeIndex, // OpcodeIndex
                                          0,                                  // PrefixOpcodeIndex
                                          TRUE,                               // DataCycle
                                          TRUE,                               // Atomic
                                          TRUE,                               // ShiftOut
                                          WriteAddress,                       // Address
                                          mStateMachinePrivateData[mCurrBlock].BlockSize,   // Data Number
                                          (UINT8 *)(UINTN)mStateMachinePrivateData[mCurrBlock].SourceAddr,
                                          EnumSpiRegionAll
                                          );

  //
  // Data needs to be written to flash. 
  //
  ASSERT_EFI_ERROR (Status);

  //
  // Flash write is always enabled. Todo fix. Need to disable flash write first after write done.
  //
  mCurrBlock++;
  mUpdateStatusPacket.BlocksCompleted = mCurrBlock;

  return EFI_SUCCESS;
}

/**
  Clear state machine structures for next capsule update. 
  
**/
VOID 
ResetStateMachineDataStructures()
{
  UINTN                       Counter; 

  for(Counter =0; Counter < MAX_BLOCK_NUMBER; Counter++){
    mStateMachinePrivateData[Counter].SmmSpiProtocol = NULL;
    mStateMachinePrivateData[Counter].SourceAddr = 0x0;
    mStateMachinePrivateData[Counter].TargetAddr  = 0x0;
    mStateMachinePrivateData[Counter].BlockSize  = 0x0;
    mStateMachinePrivateData[Counter].IsValid = FALSE;
    mStateMachinePrivateData[Counter].UpdateStatus = FALSE;
  }

  mBufferOffset =0;
}

/**
  Go through capsule update step by step.
  The step is like ChallengeCapsule, CapsuleParse, and loop each block with Erase and Write.
  
  @return Status return from each step.
**/
EFI_STATUS
SmiStateMachine (
  VOID
  )
{
  EFI_STATUS    Status;

  if(mActiveState == SMI_FLASH_UPDATE_START){
		mStateId = CHALLENGE_CAPSULE_STATE;
    mActiveState = SMI_FLASH_UPDATE_IN_PROGRESS;
	}
  //
  // Initialize next_state_ptr( )
  //
  mNextState.NextStatePointer = (NEXT_STATE_POINTER)SmiFlashUpdateStateArray[mStateId];
  //
  // Transition to next state
  //
  Status  = mNextState.NextStatePointer();
  mUpdateStatusPacket.Status = Status;

  if (!EFI_ERROR (Status)) {
    //
    // Adjust the state_id
    //
    //  CHALLENGE_CAPSULE_STATE=0,
    //  PARSE_CAPSULE_STATE,
    //  ERASE_BLOCK_STATE,
    //  WRITE_BLOCK_STATE

    if (mStateId == WRITE_BLOCK_STATE){
      //
      // check if more blocks to update; if so:
      //
      if (mCurrBlock < MAX_BLOCK_NUMBER && mStateMachinePrivateData[mCurrBlock].IsValid == TRUE) {
        mStateId = ERASE_BLOCK_STATE;
      } else {
        mStateId++;
      }
    } else {
	    mStateId++;
    }
  }

  return Status;
}

/**
  SMI Handler to update flash and get update status.
  
  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]   SwRegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in,out] SwContext       A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in,out] CommBufferSize  The size of the CommBuffer.
  
  @retval EFI_UNSUPPORTED        The input SMI is not the expected SMI value.
  @retval EFI_INVALID_PARAMETER  The input capsule image is not the valid capsule. 
  @retval EFI_OUT_OF_RESOURCES   No enough memory to store the capsule image.
  @retval EFI_SUCCESS            Update capsule or Get update status successfully.
**/
EFI_STATUS
EFIAPI
SMIFlashSMIHandler (
  IN EFI_HANDLE                        DispatchHandle,
  IN CONST EFI_SMM_SW_REGISTER_CONTEXT *SwRegisterContext,
  IN OUT EFI_SMM_SW_CONTEXT            *SwContext,
  IN OUT UINTN                         *CommBufferSize
  )
{
  UINT8                 Function;
  UINT8                 Operation;
  UPDATE_STATUS_PACKET  *UpdateStatusPacketPtr;
  CAPSULE_INFO_PACKET   *CapsuleInfo;
  CAPSULE_FRAGMENT      *CapsuleFragment;
  UINT16                Data16;
  UINTN                 BufferAddress;
  EFI_STATUS            Status;

  //
  // Extract the subfunction number from AH, and process the requested subfunction.
  //
  Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      sizeof (UINT16),
                      EFI_SMM_SAVE_STATE_REGISTER_RAX,
                      SwContext->SwSmiCpuIndex,
                      &Data16
                      );
  ASSERT_EFI_ERROR (Status);

  //
  // Get Function and Operation
  //
  Function = (UINT8)((Data16 & 0xFF00) >> 8);
  Operation = (UINT8)(Data16 & 0xFF);
  if (Function != SMI_CAP_FUNCTION || Operation != SwRegisterContext->SwSmiInputValue) {
    return EFI_UNSUPPORTED;
  }
  
  Status = mSmmCpu->ReadSaveState (
                      mSmmCpu,
                      sizeof (UINTN),
                      EFI_SMM_SAVE_STATE_REGISTER_RBX,
                      SwContext->SwSmiCpuIndex,
                      &BufferAddress
                      );
  ASSERT_EFI_ERROR (Status);

  if (Operation == SMI_INPUT_UPDATE_CAP) {
    //
    // Copy the Capsule into SMM capsule Buffer for processing
    //
    CapsuleInfo = (CAPSULE_INFO_PACKET *) BufferAddress;
    CapsuleFragment = (CAPSULE_FRAGMENT *) CapsuleInfo->CapsuleLocation;
    if (mCapsuleSmmBuffer == NULL) {
      //
      // Return Error if SMM Buffer to store cap is not avaiable
      //
      mUpdateStatusPacket.Status = EFI_OUT_OF_RESOURCES;
      CapsuleInfo->Status = EFI_OUT_OF_RESOURCES;
      return mUpdateStatusPacket.Status;
    }

    mUpdateStatusPacket.TotalBlocks = (UINTN) -1;
    mUpdateStatusPacket.BlocksCompleted = 0;
    mUpdateStatusPacket.Status = EFI_NOT_READY;

    //
    // Process all the fragments - Each SMI could have 1 or more fragements
    // must be terminated by NULL fragment entry
    //
    while ((UINT8 *)(UINTN)CapsuleFragment->Address != NULL && CapsuleFragment->Size != 0) {
      //
      // Check wether capsule chunk is one expected.Chunks should be passed in sequence.
      //
      if (CapsuleFragment->BufferOffset != mBufferOffset) {
        mUpdateStatusPacket.Status = EFI_INVALID_PARAMETER;
        CapsuleInfo->Status = EFI_INVALID_PARAMETER;
        mBufferOffset = 0;
        return mUpdateStatusPacket.Status;
      }
      //
      // Make sure capsule size is not larger than ACPI NVS buffer.
      //
      ASSERT ((mBufferOffset + CapsuleFragment->Size) <= FixedPcdGet32(PcdMaxSizeNonPopulateCapsule));
      if ((mBufferOffset + CapsuleFragment->Size) > FixedPcdGet32(PcdMaxSizeNonPopulateCapsule)) {
        //
        // return no buffer available, if capsule too big
        //
        mUpdateStatusPacket.Status = EFI_OUT_OF_RESOURCES;
        CapsuleInfo->Status = EFI_OUT_OF_RESOURCES;
        mBufferOffset = 0;
        return mUpdateStatusPacket.Status;
      }
      //
      // Copy capsule chunk to ACPI NVS buffer
      //
      CopyMem (mCapsuleSmmBuffer + mBufferOffset, (UINT8 *)(UINTN)CapsuleFragment->Address, CapsuleFragment->Size);
      mBufferOffset = mBufferOffset + CapsuleFragment->Size;

      //
      // When all Capsule chunks are passed, last fragment entry flags Bit0 set to 1 
      //
      if((CapsuleFragment->Flags & BIT0) == BIT0) {
        ASSERT (mBufferOffset == CapsuleInfo->CapsuleSize);
        //
        // Update Capusle Image
      	//
        mUpdateStatusPacket.Status = EFI_SUCCESS;
        mActiveState = SMI_FLASH_UPDATE_START;
        do {
          Status = SmiStateMachine();
        } while (!EFI_ERROR (Status) && (mStateId != (SMI_FLASH_UPDATE_MAX_STATES - 1)));
        ResetStateMachineDataStructures();

        if (EFI_ERROR (Status)) {
          return Status;
        }
        break;
      }
      CapsuleFragment++;
    }

    //
    // Return status for Each capsule transfer call
    //
    CapsuleInfo->Status = EFI_SUCCESS;
  } else if (Operation == SMI_INPUT_GET_CAP) {
    //
    // Return current update status
    //
    UpdateStatusPacketPtr = (UPDATE_STATUS_PACKET *)BufferAddress;
    UpdateStatusPacketPtr->Status = mUpdateStatusPacket.Status;
    UpdateStatusPacketPtr->BlocksCompleted = mUpdateStatusPacket.BlocksCompleted;
    UpdateStatusPacketPtr->TotalBlocks = mUpdateStatusPacket.TotalBlocks;
  }

  return EFI_SUCCESS;
}

/**
  SMI Flash Dxe Entry point. It intializes memory and services,
  and registers two SMI handler for capsule update.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SMIFlashDxeEntry (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    SwHandle;
  EFI_SMM_SW_DISPATCH2_PROTOCOL *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT   SwContext;
  EFI_PHYSICAL_ADDRESS          Memory;
  EFI_BOOT_MODE                 BootMode;

  BootMode = GetBootModeHob ();

  mSpiImageListSize = sizeof(mSpiImageList)/sizeof(SPI_IMAGE_PRESENT);

  Status = gSmst->SmmLocateProtocol (&gEfiSmmSpiProtocolGuid, NULL, &mEfiSpiProtocol);
  ASSERT_EFI_ERROR (Status);

  //
  // Find info to allow usage of SpiProtocol->Execute.
  //
  Status = mEfiSpiProtocol->Info (
                              mEfiSpiProtocol,
                              &mSpiCtrlerInfo
                              );
  ASSERT_EFI_ERROR (Status);
  ASSERT (mSpiCtrlerInfo != NULL);
  ASSERT (mSpiCtrlerInfo->InitTable != NULL && mSpiCtrlerInfo->EraseOpcodeIndex < SPI_NUM_OPCODE && mSpiCtrlerInfo->ProgramOpcodeIndex < SPI_NUM_OPCODE);

  mSpiFlashBaseAddress = FixedPcdGet32 (PcdFlashAreaBaseAddress) - (UINT32)mSpiCtrlerInfo->InitTable->BiosStartOffset;
  ASSERT(mSpiFlashBaseAddress >= (SIZE_4GB - SPI_FLASH_8M));

  //
  // Allocate Buffer for capsule in SMM. SMM is not enough so use ACPI NVS.
  // mCapsuleSmmBuffer = AllocatePages (EFI_SIZE_TO_PAGES (FixedPcdGet32(PcdMaxSizeNonPopulateCapsule)));
  //
  Status = gBS->AllocatePages (
             AllocateAnyPages, 
             EfiACPIMemoryNVS, 
             EFI_SIZE_TO_PAGES (FixedPcdGet32(PcdMaxSizeNonPopulateCapsule)), 
             &Memory
             );
  ASSERT_EFI_ERROR (Status);

  mCapsuleSmmBuffer = (UINT8 *) (UINTN) Memory;
  ASSERT (mCapsuleSmmBuffer != NULL);
  if (mCapsuleSmmBuffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Status = gSmst->SmmLocateProtocol (&gEfiSmmCpuProtocolGuid, NULL, &mSmmCpu);
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmLocateProtocol (&gEfiSmmSwDispatch2ProtocolGuid, NULL, &SwDispatch);
  ASSERT_EFI_ERROR (Status);

  //
  // Register SMI handler for Update and GetInfo.
  //
  SwContext.SwSmiInputValue = SMI_INPUT_UPDATE_CAP;
  Status = SwDispatch->Register (
                    SwDispatch, 
                    SMIFlashSMIHandler, 
                    &SwContext, 
                    &SwHandle
                    );
  ASSERT_EFI_ERROR (Status);

  SwContext.SwSmiInputValue = SMI_INPUT_GET_CAP;
  Status = SwDispatch->Register (
                    SwDispatch, 
                    SMIFlashSMIHandler, 
                    &SwContext, 
                    &SwHandle
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Init trapping of SPI Flash access violations on secure lock builds.
  //
  if (FeaturePcdGet (PcdEnableSecureLock) && BootMode != BOOT_ON_FLASH_UPDATE && BootMode != BOOT_IN_RECOVERY_MODE) {
    AccessViolationHandlerInit ();
    QncEnableLegacyFlashAccessViolationSmi ();
  }
  return EFI_SUCCESS;
}
