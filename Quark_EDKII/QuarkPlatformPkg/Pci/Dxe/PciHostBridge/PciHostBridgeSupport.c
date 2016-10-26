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

    PciHostBridgeSupport.c
    
Abstract:

   Do platform initialization for PCI bridge.

--*/

#include "PciHostBridge.h"

EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *mPciRootBridgeIo;

EFI_STATUS
ChipsetPreprocessController (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL          *This,
  IN  EFI_HANDLE                                                RootBridgeHandle,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS               PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE              Phase
  )
/*++

Routine Description:
  This function is called for all the PCI controllers that the PCI 
  bus driver finds. Can be used to Preprogram the controller.

Arguments:
  This             -- The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance
  RootBridgeHandle -- The PCI Root Bridge handle
  PciBusAddress    -- Address of the controller on the PCI bus
  Phase            -- The Phase during resource allocation
    
Returns:
  EFI_SUCCESS
  
--*/

// GC_TODO:    PciAddress - add argument and description to function comment
//
// GC_TODO:    PciAddress - add argument and description to function comment
//
// GC_TODO:    PciAddress - add argument and description to function comment
//
// GC_TODO:    PciAddress - add argument and description to function comment
//
{

  EFI_STATUS  Status;
  UINT8       Latency;
  UINT8       CacheLineSize;

  if (mPciRootBridgeIo == NULL) {
    //
    // Get root bridge in the system.
    //
    Status = gBS->HandleProtocol (RootBridgeHandle, &gEfiPciRootBridgeIoProtocolGuid, &mPciRootBridgeIo);
    ASSERT_EFI_ERROR (Status);
  }

  if (Phase == EfiPciBeforeResourceCollection) {
    //
    // Program the latency register, CLS register
    //
    PciAddress.Register = PCI_LATENCY_TIMER_OFFSET;
    mPciRootBridgeIo->Pci.Read (
                            mPciRootBridgeIo,
                            EfiPciWidthUint8,
                            *((UINT64 *) &PciAddress),
                            1,
                            &Latency
                            );

    //
    // PCI-x cards come up with a default latency of 0x40. Don't touch them.
    //
    if (Latency == 0) {
      Latency = DEFAULT_PCI_LATENCY;
      mPciRootBridgeIo->Pci.Write (
                              mPciRootBridgeIo,
                              EfiPciWidthUint8,
                              *((UINT64 *) &PciAddress),
                              1,
                              &Latency
                              );
    }
    //
    // Program Cache Line Size as 64bytes
    // 16 of DWORDs = 64bytes (0x10)
    //
    PciAddress.Register = PCI_CACHELINE_SIZE_OFFSET;
    CacheLineSize       = 0x10;
    mPciRootBridgeIo->Pci.Write (
                            mPciRootBridgeIo,
                            EfiPciWidthUint8,
                            *((UINT64 *) &PciAddress),
                            1,
                            &CacheLineSize
                            );

  }

  return EFI_SUCCESS;
}

UINT64
GetAllocAttributes (
  IN  UINTN        RootBridgeIndex
  )
/*++

Routine Description:

  Returns the Allocation attributes for the BNB Root Bridge.

Arguments:

  RootBridgeIndex  -  The root bridge number. 0 based.
    
Returns:

  EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM | EFI_PCI_HOST_BRIDGE_MEM64_DECODE
    
--*/
{
  //
  // Cannot have more than one Root bridge
  //
  //ASSERT (RootBridgeIndex == 0);

  //
  // PCI Root Bridge does not support separate windows for Non-prefetchable
  // and Prefetchable memory. A PCI bus driver needs to include requests for
  // Prefetchable memory in the Non-prefetchable memory pool.
  // Further TNB does not support 64 bit memory apertures for PCI. BNB
  // can only have system memory above 4 GB,
  //

    return EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM | EFI_PCI_HOST_BRIDGE_MEM64_DECODE;
}
