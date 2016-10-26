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

  PeiDsAlloc.c

Abstract:

  This file implements DS debug feature NEM and RAM buffers allocations in PEI phase,
  see "Enabling DS for External Debugger" feature documentation for more details.

--*/

#include "CommonHeader.h"

EFI_STATUS
EFIAPI
DsRamAllocationCallback (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor,
  IN VOID                         *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR mDsRamNotifyDesc = {  
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    DsRamAllocationCallback   
};

//
// Function Implementations
//
VOID
BpeDsAllocation (
  VOID
  )
/*++

Routine Description:

  Allocate NEM buffer for Data Store function and set callback for DRAM allocation in PEI phase

Arguments:

  None

Returns:

  None

--*/
{
  EFI_STATUS                 Status;
  EFI_PHYSICAL_ADDRESS       *DsNemAddress;

  //
  // Check for Debugger
  //
  DsBreakpointEvent (DsConfigSwBpeIdCheck, 0, 0, &Status);

  //
  // If Debugger is present, ...
  //
  if (Status == EFI_SUCCESS) {
      //
      // ...allocate DS buffer in NEM
      //
      Status = PeiServicesAllocatePool (DS_CONFIG_NEM_SIZE, (VOID **)&DsNemAddress);
      ASSERT_EFI_ERROR (Status);

      //
      // Notify Debugger
      //
      DsBreakpointEvent (
        DsConfigSwBpeIdNemStart,
        (EFI_PHYSICAL_ADDRESS) (UINTN) DsNemAddress,
        DS_CONFIG_NEM_SIZE,
        &Status
        );

    //
    // Register Notify callback for doing RAM buffer allocation
    //
    Status = PeiServicesNotifyPpi (&mDsRamNotifyDesc);
    ASSERT_EFI_ERROR (Status);
  }
}

EFI_STATUS
EFIAPI
DsRamAllocationCallback (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor,
  IN VOID                         *Ppi
  )
/*++

Routine Description:

  Internal function to allocate RAM buffer for DS function in PEI phase

Arguments:

  PeiServices       General purpose services available to every PEIM.
  NotifyDescriptor  The notification structure this PEIM registered on install.
  Ppi               The memory discovered PPI.  Not used.
    
Returns:

  EFI_SUCCESS        The function completed successfully.

--*/
{
  EFI_STATUS                 Status;
  EFI_PHYSICAL_ADDRESS       DsDramAddress;
  UINTN                      SizeInByte;

  //
  // Disable DS Cache As Ram buffer
  //
  DsBreakpointEvent (DsConfigSwBpeIdNemStop, 0, 0, &Status);

  //
  // Check for Debugger
  //
  DsBreakpointEvent (DsConfigSwBpeIdCheck, 0, 0, &Status);

  //
  // If Debugger is present, ...
  //
  if (Status == EFI_SUCCESS) {
    SizeInByte = DS_CONFIG_DRAM_SIZE;
    //
    // ...allocate DS buffer from DRAM
    //
    Status = PeiServicesAllocatePages (
                              EfiRuntimeServicesData, 
                              SizeInByte / EFI_PAGE_SIZE,
                              &DsDramAddress
                              );
    ASSERT_EFI_ERROR (Status);

    //
    // Notify Debugger
    //
    DsBreakpointEvent (
      DsConfigSwBpeIdDramStart,
      DsDramAddress,
      DS_CONFIG_DRAM_SIZE,
      &Status
      );
  }

  return Status;
}
