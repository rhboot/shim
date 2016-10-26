/** @file

  PCH SPI SMM Driver implements the SPI Host Controller Compatibility Interface.

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
#include "PchSpi.h"

SPI_INSTANCE          *mSpiInstance;

EFI_STATUS
EFIAPI
InstallPchSpi (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
  )
/*++

Routine Description:

  Entry point for the SPI host controller driver.
  
Arguments:

  ImageHandle       Image handle of this driver.
  SystemTable       Global system service table.

Returns:

  EFI_SUCCESS           Initialization complete.
  EFI_UNSUPPORTED       The chipset is unsupported by this driver.
  EFI_OUT_OF_RESOURCES  Do not have enough resources to initialize the driver.
  EFI_DEVICE_ERROR      Device error, driver exits abnormally.

--*/
{
  EFI_STATUS    Status;

  //
  // Allocate pool for SPI protocol instance
  //
  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData, // MemoryType don't care
                    sizeof (SPI_INSTANCE),
                    &mSpiInstance
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (mSpiInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  ZeroMem ((VOID *) mSpiInstance, sizeof (SPI_INSTANCE));
  //
  // Initialize the SPI protocol instance
  //
  Status = SpiProtocolConstructor (mSpiInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install the SMM EFI_SPI_PROTOCOL interface
  //
  Status = gSmst->SmmInstallProtocolInterface (
            &(mSpiInstance->Handle),
            &gEfiSmmSpiProtocolGuid,
            EFI_NATIVE_INTERFACE,
            &(mSpiInstance->SpiProtocol)
            );
  if (EFI_ERROR (Status)) {
    gSmst->SmmFreePool (mSpiInstance);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
SpiPhaseInit (
  VOID
  )
/*++
Routine Description:

  This function is a a hook for Spi Smm phase specific initialization

Arguments:

  None

Returns:
  
  None

--*/
{
  UINTN       Index;
  static CONST UINT32 SpiRegister[] = {
    R_QNC_RCRB_SPIS,
    R_QNC_RCRB_SPIPREOP,
    R_QNC_RCRB_SPIOPMENU,
    R_QNC_RCRB_SPIOPMENU + 4
  };

  //
  // Save SPI Registers for S3 resume usage
  //
  for (Index = 0; Index < sizeof (SpiRegister) / sizeof (UINT32); Index++) {
    S3BootScriptSaveMemWrite (
      S3BootScriptWidthUint32,
      (UINTN) (mSpiInstance->PchRootComplexBar + SpiRegister[Index]),
      1,
      (VOID *) (UINTN) (mSpiInstance->PchRootComplexBar + SpiRegister[Index])
      );
  }
}
