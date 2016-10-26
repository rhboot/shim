/** @file
  Runtime Lib function for QNC internal network access.

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


#include <PiDxe.h>

#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/QNCAccessLib.h>

///
/// Set Virtual Address Map Event
///
EFI_EVENT                               mDxeRuntimeQncAccessLibVirtualNotifyEvent = NULL;

///
/// Module global that contains the base physical address of the PCI Express MMIO range.
///
UINTN                                   mDxeRuntimeQncAccessLibPciExpressBaseAddress = 0;

/**
  Convert the physical PCI Express MMIO address to a virtual address.

  @param[in]    Event   The event that is being processed.
  @param[in]    Context The Event Context.
**/
VOID
EFIAPI
DxeRuntimeQncAccessLibVirtualNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                       Status;

  //
  // Convert the physical PCI Express MMIO address to a virtual address.
  //
  Status = EfiConvertPointer (0, (VOID **) &mDxeRuntimeQncAccessLibPciExpressBaseAddress);

  ASSERT_EFI_ERROR (Status);
}

/**
  The constructor function to setup globals and goto virtual mode notify.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor completed successfully.
  @retval Other value   The constructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
DxeRuntimeQncAccessLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Cache the physical address of the PCI Express MMIO range into a module global variable
  //
  mDxeRuntimeQncAccessLibPciExpressBaseAddress = (UINTN) PcdGet64(PcdPciExpressBaseAddress);

  //
  // Register SetVirtualAddressMap () notify function
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  DxeRuntimeQncAccessLibVirtualNotify,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mDxeRuntimeQncAccessLibVirtualNotifyEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  The destructor function frees any allocated buffers and closes the Set Virtual 
  Address Map event.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
  @retval Other value   The destructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
DxeRuntimeQncAccessLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Close the Set Virtual Address Map event
  //
  Status = gBS->CloseEvent (mDxeRuntimeQncAccessLibVirtualNotifyEvent);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Gets the base address of PCI Express for Quark North Cluster.

  @return The base address of PCI Express for Quark North Cluster.

**/
UINTN
EFIAPI
QncGetPciExpressBaseAddress (
  VOID
  )
{
  //
  // If system goes to virtual mode then virtual notify callback will update
  // mDxeRuntimeQncAccessLibPciExpressBaseAddress with virtual address of
  // PCIe memory base.
  //
  return mDxeRuntimeQncAccessLibPciExpressBaseAddress;
}

