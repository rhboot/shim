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

  PciPlatform.c

Abstract:

    Registers onboard PCI ROMs with PCI.IO
Revision History

--*/

#include "CommonHeader.h"

#include "PciPlatform.h"


PCI_OPTION_ROM_TABLE mPciOptionRomTable[] = {
  { NULL_ROM_FILE_GUID,                    0, 0, 0, 0, 0xffff, 0xffff }
};
EFI_PCI_PLATFORM_PROTOCOL mPciPlatform = {
  PhaseNotify,
  PlatformPrepController,
  GetPlatformPolicy,
  GetPciRom
};

EFI_HANDLE mPciPlatformHandle = NULL;
EFI_HANDLE mImageHandle       = NULL;


EFI_STATUS
PhaseNotify (
  IN EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN EFI_HANDLE                                     HostBridge,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE  Phase,
  IN EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  )
{
  UINT8                  UsbHostBusNumber = 0;
  if (Phase == EfiPciHostBridgeEndResourceAllocation) {
    // Required for QuarkSouthCluster.
    // Enable USB controller memory, io and bus master before Ehci driver.
    EnableUsbMemIoBusMaster (UsbHostBusNumber);
    return EFI_SUCCESS;
  }
  return EFI_UNSUPPORTED;
}


EFI_STATUS
PlatformPrepController (
  IN  EFI_PCI_PLATFORM_PROTOCOL                      *This,
  IN  EFI_HANDLE                                     HostBridge,
  IN  EFI_HANDLE                                     RootBridge,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS    PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE   Phase,
  IN  EFI_PCI_CHIPSET_EXECUTION_PHASE                ChipsetPhase
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
GetPlatformPolicy (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL                     *This,
  OUT       EFI_PCI_PLATFORM_POLICY                       *PciPolicy
  )
{
  if (PciPolicy == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
GetPciRom (
  IN  CONST EFI_PCI_PLATFORM_PROTOCOL                   *This,
  IN        EFI_HANDLE                                  PciHandle,
  OUT       VOID                                        **RomImage,
  OUT       UINTN                                       *RomSize
  )
/*++

  Routine Description:
    Return a PCI ROM image for the onboard device represented by PciHandle

  Arguments:
    This      - Protocol instance pointer.
    PciHandle - PCI device to return the ROM image for.
    RomImage  - PCI Rom Image for onboard device
    RomSize   - Size of RomImage in bytes

  Returns:
    EFI_SUCCESS   - RomImage is valid
    EFI_NOT_FOUND - No RomImage

--*/
{
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINTN                         Segment;
  UINTN                         Bus;
  UINTN                         Device;
  UINTN                         Function;
  UINT16                        VendorId;
  UINT16                        DeviceId;
  UINT16                        DeviceClass;
  UINTN                         TableIndex;

  Status = gBS->HandleProtocol (
                  PciHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0x0A, 1, &DeviceClass);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 0, 1, &VendorId);

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint16, 2, 1, &DeviceId);

  //
  // Loop through table of video option rom descriptions
  //
  for (TableIndex = 0; mPciOptionRomTable[TableIndex].VendorId != 0xffff; TableIndex++) {

    //
    // See if the PCI device specified by PciHandle matches at device in mPciOptionRomTable
    //
    if (VendorId != mPciOptionRomTable[TableIndex].VendorId ||
        DeviceId != mPciOptionRomTable[TableIndex].DeviceId ||
        Segment != mPciOptionRomTable[TableIndex].Segment ||
        Bus != mPciOptionRomTable[TableIndex].Bus ||
        Device != mPciOptionRomTable[TableIndex].Device ||
        Function != mPciOptionRomTable[TableIndex].Function) {
      continue;
    }

    Status = GetSectionFromFv (
               &mPciOptionRomTable[TableIndex].FileName,
               EFI_SECTION_RAW,
               0,
               RomImage,
               RomSize
               );

    if (EFI_ERROR (Status)) {
      continue;
    }

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
PciPlatformDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:
  EFI_STATUS

--*/
{
  EFI_STATUS  Status;

  mImageHandle = ImageHandle;

  //
  // Install on a new handle
  //
  Status = gBS->InstallProtocolInterface (
                  &mPciPlatformHandle,
                  &gEfiPciPlatformProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPciPlatform
                  );

  return Status;
}
