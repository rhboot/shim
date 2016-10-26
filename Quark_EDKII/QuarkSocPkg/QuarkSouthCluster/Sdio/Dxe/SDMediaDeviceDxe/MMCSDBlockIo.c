/** @file

   Block I/O protocol for MMC/SD device

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

#include "SDMediaDevice.h"

/**
  Implements EFI_BLOCK_IO_PROTOCOL.Reset() function.

  @param  This                   The EFI_BLOCK_IO_PROTOCOL instance.
  @param  ExtendedVerification   Indicates that the driver may perform a more exhaustive.
                                 verification operation of the device during reset.
                                 (This parameter is ingored in this driver.)
                                
  @retval EFI_SUCCESS                Success
**/
EFI_STATUS
EFIAPI
MMCSDBlockReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  )      
{
  CARD_DATA                  *CardData;   
  EFI_SD_HOST_IO_PROTOCOL    *SDHostIo;

  CardData  = CARD_DATA_FROM_THIS(This);
  SDHostIo = CardData->SDHostIo;

  return SDHostIo->ResetSDHost (SDHostIo, Reset_DAT_CMD);
 }

/**
  Implements EFI_BLOCK_IO_PROTOCOL.ReadBlocks() function.

  @param  This                   The EFI_BLOCK_IO_PROTOCOL instance.
  @param  MediaId                The media id that the write request is for.
  @param  LBA                    The starting logical block address to read from on the device.
                                 The caller is responsible for writing to only legitimate locations.
  @param  BufferSize             The size of the Buffer in bytes. This must be a multiple of the 
                                 intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller
                                 is responsible for either having implicit or explicit ownership
                                 of the buffer.
                                
  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad
**/
EFI_STATUS
EFIAPI
MMCSDBlockReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )    
{
  EFI_STATUS                  Status;
  UINT32                      Address;
  CARD_DATA                   *CardData;   
  EFI_SD_HOST_IO_PROTOCOL     *SDHostIo;
  UINT32                      RemainingLength;
  UINT32                      TransferLength; 
  UINT8                       *BufferPointer;

  DEBUG((EFI_D_INFO, "Read(LBA=%08lx, Buffer=%08x, Size=%08x)\n", LBA, Buffer, BufferSize));
  Status   = EFI_SUCCESS;
  CardData  = CARD_DATA_FROM_THIS(This);
  SDHostIo = CardData->SDHostIo;

  if (CardData->CardType == SDMemoryCard2High) {
    //
    //Block Address
    //
    Address = (UINT32)DivU64x32 (MultU64x32 (LBA, CardData->BlockIoMedia.BlockSize), 512);
  } else {
    //
    //Byte Address
    // 
    Address  = (UINT32)MultU64x32 (LBA, CardData->BlockIoMedia.BlockSize);
  }


  if (!Buffer) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((EFI_D_ERROR, "MMCSDBlockReadBlocks:Invalid parameter \r\n"));
    goto Done;
  }

  if ((BufferSize % CardData->BlockIoMedia.BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    DEBUG ((EFI_D_ERROR, "MMCSDBlockReadBlocks: Bad buffer size \r\n"));
    goto Done;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Done;
  }


  if (BufferSize <= CardData->BlockIoMedia.BlockSize) {
    Status = SendCommand (
               SDHostIo, 
               READ_SINGLE_BLOCK,
               Address,
               InData, 
               CardData->AlignedBuffer,
               (UINT32)BufferSize,
               ResponseR1,
               TIMEOUT_DATA,
               (UINT32*)&(CardData->CardStatus)
               );
    if (!EFI_ERROR (Status)) {
      CopyMem (Buffer, CardData->AlignedBuffer, BufferSize);
    }

  } else {

    BufferPointer   = Buffer;
    RemainingLength = (UINT32)BufferSize;

    while (RemainingLength > 0) {
      if (RemainingLength > SDHostIo->HostCapability.BoundarySize) {
        TransferLength = SDHostIo->HostCapability.BoundarySize;
      } else {
        TransferLength = RemainingLength;
      }

      Status = SendCommand (
                 SDHostIo, 
                 READ_MULTIPLE_BLOCK,
                 Address,
                 InData, 
                 CardData->AlignedBuffer,
                 TransferLength,
                 ResponseR1,
                 TIMEOUT_DATA,
                 (UINT32*)&(CardData->CardStatus)
                 );

      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "MMCSDBlockReadBlocks: Fail to send READ_MULTIPLE_BLOCK \r\n"));
        break;
      }

      CopyMem (BufferPointer, CardData->AlignedBuffer, TransferLength);

      if (CardData->CardType == SDMemoryCard2High) {
        //
        //Block Address
        //
        Address += TransferLength / 512;
      } else {
        //
        //Byte Address
        // 
        Address += TransferLength;
      }
      BufferPointer   += TransferLength;
      RemainingLength -= TransferLength;  
   }

  }

  if (EFI_ERROR (Status)) {
    if ((CardData->CardType == SDMemoryCard) || 
        (CardData->CardType == SDMemoryCard2)||
        (CardData->CardType == SDMemoryCard2High)) {
         SendCommand (
           SDHostIo, 
           STOP_TRANSMISSION,
           0,
           NoData, 
           NULL,
           0,
           ResponseR1b,
           TIMEOUT_COMMAND,
           (UINT32*)&(CardData->CardStatus)
           );
    } else {
       SendCommand (
         SDHostIo, 
         STOP_TRANSMISSION,
         0,
         NoData, 
         NULL,
         0,
         ResponseR1,
         TIMEOUT_COMMAND,
         (UINT32*)&(CardData->CardStatus)
         );
    }

  }


Done:
  return Status;
}

/**
  Implements EFI_BLOCK_IO_PROTOCOL.WriteBlocks() function.

  @param  This                   The EFI_BLOCK_IO_PROTOCOL instance.
  @param  MediaId                The media id that the write request is for.
  @param  LBA                    The starting logical block address to read from on the device.
                                 The caller is responsible for writing to only legitimate locations.
  @param  BufferSize             The size of the Buffer in bytes. This must be a multiple of the 
                                 intrinsic block size of the device.
  @param  Buffer                 A pointer to the destination buffer for the data. The caller
                                 is responsible for either having implicit or explicit ownership
                                 of the buffer.
                                
  @retval EFI_SUCCESS                Success
  @retval EFI_DEVICE_ERROR           Hardware Error
  @retval EFI_INVALID_PARAMETER      Parameter is error
  @retval EFI_NO_MEDIA               No media
  @retval EFI_MEDIA_CHANGED          Media Change
  @retval EFI_BAD_BUFFER_SIZE        Buffer size is bad
**/
EFI_STATUS
EFIAPI
MMCSDBlockWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 LBA,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  )     
{
  EFI_STATUS                  Status;
  UINT32                      Address;
  CARD_DATA                   *CardData;   
  EFI_SD_HOST_IO_PROTOCOL     *SDHostIo;
  UINT32                      RemainingLength;
  UINT32                      TransferLength; 
  UINT8                       *BufferPointer;

  DEBUG((EFI_D_INFO, "Write(LBA=%08lx, Buffer=%08x, Size=%08x)\n", LBA, Buffer, BufferSize));
  Status   = EFI_SUCCESS;
  CardData  = CARD_DATA_FROM_THIS(This);
  SDHostIo = CardData->SDHostIo;
  if (CardData->CardType == SDMemoryCard2High) {
    //
    //Block Address
    //
    Address = (UINT32)DivU64x32 (MultU64x32 (LBA, CardData->BlockIoMedia.BlockSize), 512);
  } else {
    //
    //Byte Address
    // 
    Address = (UINT32)MultU64x32 (LBA, CardData->BlockIoMedia.BlockSize);
  }

  if (!Buffer) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((EFI_D_ERROR, "MMCSDBlockWriteBlocks: Invalid parameter \r\n"));
    goto Done;
  }

  if ((BufferSize % CardData->BlockIoMedia.BlockSize) != 0) {
    Status = EFI_BAD_BUFFER_SIZE;
    DEBUG ((EFI_D_ERROR, "MMCSDBlockWriteBlocks: Bad buffer size \r\n"));
    goto Done;
  }

  if (BufferSize == 0) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  if (This->Media->ReadOnly == TRUE) {
    Status = EFI_WRITE_PROTECTED;
    DEBUG ((EFI_D_ERROR, "MMCSDBlockWriteBlocks: Write protected \r\n"));
    goto Done;
  }


  if (BufferSize <= CardData->BlockIoMedia.BlockSize) {
    CopyMem (CardData->AlignedBuffer, Buffer, BufferSize);

    Status = SendCommand (
               SDHostIo, 
               WRITE_BLOCK,
               Address,
               OutData, 
               CardData->AlignedBuffer,
               (UINT32)BufferSize,
               ResponseR1,
               TIMEOUT_DATA,
               (UINT32*)&(CardData->CardStatus)
               );
  } else {
    BufferPointer   = Buffer;
    RemainingLength = (UINT32)BufferSize;

    while (RemainingLength > 0) {
      if (RemainingLength > SDHostIo->HostCapability.BoundarySize) {
        TransferLength = SDHostIo->HostCapability.BoundarySize;
      } else {
        TransferLength = RemainingLength;
      }

      if ((CardData->CardType == SDMemoryCard) || 
        (CardData->CardType == SDMemoryCard2)||
        (CardData->CardType == SDMemoryCard2High)) {
        //
        // Write performance improvement
        //
        if ((TransferLength / CardData->BlockIoMedia.BlockSize) > 64) {
          Status  = SendAppCommand (
                      CardData,
                      SET_WR_BLK_ERASE_COUNT,
                      (UINT32)(TransferLength / CardData->BlockIoMedia.BlockSize),
                      NoData,
                      NULL,
                      0,  
                      ResponseR1,
                      TIMEOUT_COMMAND,
                      (UINT32*)&(CardData->CardStatus)
                      );
        }
      }

      CopyMem (CardData->AlignedBuffer, BufferPointer, TransferLength);

      Status = SendCommand (
                 SDHostIo, 
                 WRITE_MULTIPLE_BLOCK,
                 Address,
                 OutData, 
                 CardData->AlignedBuffer,
                 (UINT32)TransferLength,
                 ResponseR1,
                 TIMEOUT_DATA,
                 (UINT32*)&(CardData->CardStatus)
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "MMCSDBlockWriteBlocks: Fail to send command WRITE_MULTIPLE_BLOCK \r\n"));
        break;
      }


      if (CardData->CardType == SDMemoryCard2High) {
        //
        //Block Address
        //
        Address += TransferLength / 512;
      } else {
        //
        //Byte Address
        // 
        Address += TransferLength;
      }
      BufferPointer   += TransferLength;
      RemainingLength -= TransferLength;  
    }

  }

  if (EFI_ERROR (Status)) {
    SendCommand (
      SDHostIo, 
      STOP_TRANSMISSION,
      0,
      NoData, 
      NULL,
      0,
      ResponseR1b,
      TIMEOUT_COMMAND,
      (UINT32*)&(CardData->CardStatus)
      );

  }


Done:
  return EFI_SUCCESS;
}

/**
  Implements EFI_BLOCK_IO_PROTOCOL.FlushBlocks() function.
    (In this driver, this function just returns EFI_SUCCESS.)

  @param  This                   The EFI_BLOCK_IO_PROTOCOL instance.
                                
  @retval EFI_SUCCESS
  @retval Others
**/
EFI_STATUS
EFIAPI
MMCSDBlockFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  )    
{
  return EFI_SUCCESS;
}


/**
  MMC/SD card BlockIo init function.

  @param  CardData               Pointer to CARD_DATA.
                                
  @retval EFI_SUCCESS
  @retval Others
**/
EFI_STATUS
MMCSDBlockIoInit (
  IN  CARD_DATA    *CardData   
  )   
{
  //
  //BlockIO protocol
  //
  CardData->BlockIo.Revision    = EFI_BLOCK_IO_PROTOCOL_REVISION;
  CardData->BlockIo.Media       = &(CardData->BlockIoMedia);
  CardData->BlockIo.Reset       = MMCSDBlockReset;
  CardData->BlockIo.ReadBlocks  = MMCSDBlockReadBlocks ;
  CardData->BlockIo.WriteBlocks = MMCSDBlockWriteBlocks;
  CardData->BlockIo.FlushBlocks = MMCSDBlockFlushBlocks;

  CardData->BlockIoMedia.MediaId          = 0;
  CardData->BlockIoMedia.RemovableMedia   = FALSE;
  CardData->BlockIoMedia.MediaPresent     = TRUE;
  CardData->BlockIoMedia.LogicalPartition = FALSE;

  if (CardData->CSDRegister.PERM_WRITE_PROTECT || CardData->CSDRegister.TMP_WRITE_PROTECT) {
    CardData->BlockIoMedia.ReadOnly         = TRUE;
  } else {
    CardData->BlockIoMedia.ReadOnly         = FALSE;
  }


  CardData->BlockIoMedia.WriteCaching     = FALSE;
  CardData->BlockIoMedia.BlockSize        = CardData->BlockLen;
  CardData->BlockIoMedia.IoAlign          = 1;
  CardData->BlockIoMedia.LastBlock        = (EFI_LBA)(CardData->BlockNumber - 1);


  return EFI_SUCCESS;

}



