/** @file
QuarkSCSocId module initialization module

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
#include "IohBds.h"
#include "IohInit.h"
//
// Definitions
//
#define DXE_DEVICE_DISABLED 0
#define DXE_DEVICE_ENABLED 1


EFI_STATUS
InitializeIohSsvidSsid (
   IN UINT8   Bus,
   IN UINT8   Device,
   IN UINT8   Func
   );

/**
   The entry function for IohInit driver.

   This function just call initialization function.

   @param ImageHandle   The driver image handle for GmchInit driver
   @param SystemTable   The pointer to System Table

   @retval EFI_SUCCESS  Success to initialize every module.
   @return EFI_STATUS   The status of initialization work.

**/
EFI_STATUS
EFIAPI
IohInit (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{

  InitializeIohSsvidSsid(IOH_BUS, PCI_IOSF2AHB_0_DEV_NUM, 0);

  InitializeIohSsvidSsid(IOH_BUS, PCI_IOSF2AHB_1_DEV_NUM, 0);

  //Enable Selected IOH Uart port
  if (PcdGetBool(PcdIohUartEnable)) {
    IohMmPci8(0, IOH_BUS, PCI_IOSF2AHB_0_DEV_NUM, PcdGet8(PcdIohUartFunctionNumber), PCI_REG_PCICMD) |= 0x7;
  }

  return EFI_SUCCESS;
}
