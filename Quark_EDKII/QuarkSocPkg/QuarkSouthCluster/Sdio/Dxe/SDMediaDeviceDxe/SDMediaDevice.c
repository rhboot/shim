/** @file

  The definition for SD media device driver model and blkio protocol routines.

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


EFI_DRIVER_BINDING_PROTOCOL gSDMediaDeviceDriverBinding = {
  SDMediaDeviceSupported,
  SDMediaDeviceStart,
  SDMediaDeviceStop,
  0x20,
  NULL,
  NULL
};

/**
  Entry point for EFI drivers.

  @param  ImageHandle      EFI_HANDLE.
  @param  SystemTable      EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS      Driver is successfully loaded.
  @return Others           Failed.

**/
EFI_STATUS
EFIAPI
InitializeSDMediaDevice (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gSDMediaDeviceDriverBinding,
           ImageHandle,
           &gSDMediaDeviceName,
           &gSDMediaDeviceName2
           );
}


/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has BlockIoProtocol installed will be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SDMediaDeviceSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{  
  EFI_STATUS                Status;
  EFI_SD_HOST_IO_PROTOCOL   *SDHostIo;

  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSDHostIoProtocolGuid,
                  (VOID **)&SDHostIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiSDHostIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

Exit:
  return Status;
}

/**
  Starting the SD Media Device Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @retval EFI_SUCCESS          This driver supports this device.
  @retval EFI_UNSUPPORTED      This driver does not support this device.
  @retval EFI_DEVICE_ERROR     This driver cannot be started due to device Error.
                               EFI_OUT_OF_RESOURCES- Failed due to resource shortage.

**/
EFI_STATUS
EFIAPI
SDMediaDeviceStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
	EFI_STATUS                Status;
  EFI_SD_HOST_IO_PROTOCOL   *SDHostIo;
  CARD_DATA                 *CardData;

  CardData = NULL;
  
  //
  // Open PCI I/O Protocol and save pointer to open protocol
  // in private data area.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSDHostIoProtocolGuid,
                  (VOID **) &SDHostIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SDMediaDeviceStart: Fail to open gEfiSDHostIoProtocolGuid \r\n"));
    goto Exit;
  } 
  
  Status = SDHostIo->DetectCardAndInitHost (SDHostIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "SDMediaDeviceStart: Fail to DetectCardAndInitHost \r\n"));
    goto Exit;
  }

  CardData = (CARD_DATA*)AllocateZeroPool(sizeof (CARD_DATA));
  if (CardData == NULL) {
    Status =  EFI_OUT_OF_RESOURCES;
    DEBUG ((EFI_D_ERROR, "SDMediaDeviceStart: Fail to AllocateZeroPool(CARD_DATA) \r\n"));
    goto Exit;
  }

  ASSERT (SDHostIo->HostCapability.BoundarySize >= 4 * 1024);
  CardData->RawBufferPointer = (UINT8*)((UINTN)DMA_MEMORY_TOP);
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiBootServicesData,
                  EFI_SIZE_TO_PAGES (2 * SDHostIo->HostCapability.BoundarySize),
                  (EFI_PHYSICAL_ADDRESS *)(&CardData->RawBufferPointer)
                  );

  if (CardData->RawBufferPointer == NULL) {
    DEBUG ((EFI_D_ERROR, "SDMediaDeviceStart: Fail to AllocateZeroPool(2*x) \r\n"));
    Status =  EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  CardData->AlignedBuffer = CardData->RawBufferPointer - ((UINTN)(CardData->RawBufferPointer) & (SDHostIo->HostCapability.BoundarySize - 1)) + SDHostIo->HostCapability.BoundarySize;

  CardData->Signature = CARD_DATA_SIGNATURE;
  CardData->SDHostIo  = SDHostIo;
  
  Status = MMCSDCardInit (CardData);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SDMediaDeviceStart: Fail to MMCSDCardInit \r\n"));
    goto Exit;
  }

  if (CardData->CardType == MMCCard) {
    if (IsCEATADevice (CardData)) {
      CardData->CardType = CEATACard;
    }
  } 

  if (CardData->CardType == CEATACard) {
    Status = CEATABlockIoInit (CardData);
  } else {
    Status = MMCSDBlockIoInit (CardData);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SDMediaDeviceStart: Fail to BlockIoInit \r\n"));
    goto Exit;
  }


  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiBlockIoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &CardData->BlockIo
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SDMediaDeviceStart: Fail to install gEfiBlockIoProtocolGuid \r\n"));
    goto Exit;
  }
  
  //
  // Install the component name protocol
  //
  CardData->ControllerNameTable = NULL;

  AddUnicodeString2 (
    "eng",
    gSDMediaDeviceName.SupportedLanguages,
    &CardData->ControllerNameTable,
    L"MMC/SD Media Device",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gSDMediaDeviceName2.SupportedLanguages,
    &CardData->ControllerNameTable,
    L"MMC/SD Media Device",
    FALSE
    );
  
Exit:
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "SDMediaDeviceStart: End with failure\r\n"));
    if (CardData != NULL) {
      if (CardData->RawBufferPointer != NULL) {
        gBS->FreePages ((EFI_PHYSICAL_ADDRESS)CardData->RawBufferPointer, EFI_SIZE_TO_PAGES (2 * SDHostIo->HostCapability.BoundarySize));
      }
      FreePool (CardData);
    }
  }
  
  return Status;	
}


/**
  Stop this driver on ControllerHandle. Support stoping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
EFIAPI
SDMediaDeviceStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                Status;
  CARD_DATA                 *CardData;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo; 

	//
  // First find BlockIo Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  CardData  = CARD_DATA_FROM_THIS(BlockIo);

  //
  // Uninstall Block I/O protocol from the device handle
  //
  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiBlockIoProtocolGuid,
                  BlockIo
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (CardData != NULL) {
    if (CardData->RawBufferPointer != NULL) {
      gBS->FreePages ((EFI_PHYSICAL_ADDRESS)CardData->RawBufferPointer, EFI_SIZE_TO_PAGES (2 * CardData->SDHostIo->HostCapability.BoundarySize));
    }
    FreeUnicodeStringTable (CardData->ControllerNameTable);
    FreePool (CardData);
  }
  
  gBS->CloseProtocol (
         Controller,
         &gEfiSDHostIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}



