/** @file
  Implementation of Usb Controller PPI.

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

#include <PiPei.h>
#include <Ppi/UsbController.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>

#include "UsbPei.h"

//
// Globals
//
//

EFI_PEI_PPI_DESCRIPTOR mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiUsbControllerPpiGuid,
  NULL
};

UINTN IohOhciPciReg[IOH_MAX_OHCI_USB_CONTROLLERS] = {
  PCI_LIB_ADDRESS (IOH_USB_BUS_NUMBER, IOH_USB_OHCI_DEVICE_NUMBER, IOH_OHCI_FUNCTION_NUMBER, 0)
};

UINTN IohEhciPciReg[IOH_MAX_EHCI_USB_CONTROLLERS] = {	
  PCI_LIB_ADDRESS (IOH_USB_BUS_NUMBER, IOH_USB_EHCI_DEVICE_NUMBER, IOH_EHCI_FUNCTION_NUMBER, 0),
};

/**
  When EHCI get started in DXE, OHCI couldn't get the ownership
  of roothub after warm reset because CF@EHCI hasn't been cleared.
  We should clear that reg before UpdateBootMode. But Reg@EHCI is
  memory-mapped, so need assume a range of space without conflict
  in PCI memory space.
  
  @param[in]  PeiServices     The pointer of EFI_PEI_SERVICES

**/

VOID
SwitchConfigFlag (
  IN EFI_PEI_SERVICES          **PeiServices
  )
{
  UINT32             SavBaseAddr;
  UINT32             UsbBaseAddr;
  UINT16             SaveCmdData;
  UINT8              EhciCapLen;
  UINT8              Index;
  UsbBaseAddr = 0;

  for (Index = 0; Index < IOH_MAX_EHCI_USB_CONTROLLERS; Index++) {
    UsbBaseAddr = PcdGet32(PcdPeiQNCUsbControllerMemoryBaseAddress);
    //
    // Manage EHCI on IOH, set UsbBaseAddr
    //
    SavBaseAddr = PciRead32 (IohEhciPciReg[Index] | R_IOH_USB_MEMBAR);
    PciWrite32 (IohEhciPciReg[Index] | R_IOH_USB_MEMBAR, UsbBaseAddr);
    //
    // Save Cmd register
    //
    SaveCmdData = PciRead16 (IohEhciPciReg[Index] | R_IOH_USB_COMMAND);
    //
    // Enable EHCI on IOH
    //
    PciOr16 (IohEhciPciReg[Index] | R_IOH_USB_COMMAND, B_IOH_USB_COMMAND_BME | B_IOH_USB_COMMAND_MSE );
    //
    // Clear CF register on EHCI
    //
    EhciCapLen = MmioRead8 (UsbBaseAddr + R_IOH_EHCI_CAPLENGTH);
    MmioWrite32 (UsbBaseAddr + EhciCapLen + R_IOH_EHCI_CONFIGFLAGS, 0);

    DEBUG ((EFI_D_INFO, "CF@EHCI = %x \n", UsbBaseAddr + EhciCapLen + R_IOH_EHCI_CONFIGFLAGS));
    //
    // Restore EHCI UsbBaseAddr in PCI space
    //
    PciWrite32 (IohEhciPciReg[Index] | R_IOH_USB_MEMBAR, SavBaseAddr);
    //
    // Restore EHCI Command register in PCI space
    //
    PciWrite16(IohEhciPciReg[Index] | R_IOH_USB_COMMAND, SaveCmdData);
  }
}
/**
  Retrieved specified the USB controller information.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  This PEI_USB_CONTROLLER_PPI instance.
  @param  UsbControllerId       Indicate which usb controller information will be retrieved.
  @param  ControllerType        Indicate the controller is Ehci, Ohci, OHCI
  @param  BaseAddress           Indicate the memory bar of the controller

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.

**/

EFI_STATUS
GetOhciController (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_CONTROLLER_PPI         *This,
  IN UINT8                          UsbControllerId,
  IN UINTN                          *ControllerType,
  IN UINTN                          *BaseAddress
  )
{
  IOH_OHCI_DEVICE         *PeiIohOhciDev;

  PeiIohOhciDev = IOH_OHCI_DEVICE_FROM_THIS (This);

  if (UsbControllerId >= IOH_MAX_OHCI_USB_CONTROLLERS) {
    return EFI_INVALID_PARAMETER;
  }
  *ControllerType = PEI_OHCI_CONTROLLER;
  *BaseAddress = PeiIohOhciDev->MmioBase[UsbControllerId];

  return EFI_SUCCESS;
}
/**
  Retrieved specified the USB controller information.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  This PEI_USB_CONTROLLER_PPI instance.
  @param  UsbControllerId       Indicate which usb controller information will be retrieved.
  @param  ControllerType        Indicate the controller is Ehci, Ohci, OHCI
  @param  BaseAddress           Indicate the memory bar of the controller

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.

**/

EFI_STATUS
GetEhciController (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_CONTROLLER_PPI         *This,
  IN UINT8                          UsbControllerId,
  IN UINTN                          *ControllerType,
  IN UINTN                          *BaseAddress
  )
{
  IOH_EHCI_DEVICE         *PeiIohEhciDev;

  PeiIohEhciDev = IOH_EHCI_DEVICE_FROM_THIS (This);
  
  if (UsbControllerId >= IOH_MAX_EHCI_USB_CONTROLLERS) {
    return EFI_INVALID_PARAMETER;
  }
  *ControllerType = PEI_EHCI_CONTROLLER;
  *BaseAddress = PeiIohEhciDev->MmioBase[UsbControllerId];

  return EFI_SUCCESS;
}

/**
  Retrieved specified the USB controller information.

  @param  IohOhciPciReg         Ohci device address list.
  @param  OhciCount             The count of the OHCI
  @param  IohEhciPciReg         Ehci device address list.
  @param  EhciCount             The count of the EHCI

**/

VOID
EnableBusMaster (
  IN UINTN IohOhciPciReg[],
  IN UINT8 OhciCount,
  IN UINTN IohEhciPciReg[],
  IN UINT8 EhciCount
  )
{
  UINT8  Index;
  UINT16 CmdReg;
  for (Index = 0; Index < OhciCount; Index ++) {
	  CmdReg = PciRead16 (IohOhciPciReg[Index] | R_IOH_USB_COMMAND);
	  CmdReg = (UINT16) (CmdReg | B_IOH_USB_COMMAND_BME );
	  PciWrite16 (IohOhciPciReg[Index] | R_IOH_USB_COMMAND, CmdReg);    
  }
  for (Index = 0; Index < EhciCount; Index ++) {
	  CmdReg = PciRead16 (IohEhciPciReg[Index] | R_IOH_USB_COMMAND);
	  CmdReg = (UINT16) (CmdReg | B_IOH_USB_COMMAND_BME );
	  PciWrite16 (IohEhciPciReg[Index] | R_IOH_USB_COMMAND, CmdReg);    
  }
}

PEI_USB_CONTROLLER_PPI mUsbControllerPpi[2] = { {GetOhciController}, {GetEhciController}};

/**
  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            PPI successfully installed

**/
EFI_STATUS
PeimInitializeIchUsb (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;
  UINTN                   i;
  EFI_PHYSICAL_ADDRESS    AllocateAddress;
  IOH_OHCI_DEVICE         *PeiIohOhciDev;
  IOH_EHCI_DEVICE         *PeiIohEhciDev;
  UINT16                  CmdReg;

  Status = PeiServicesAllocatePages (
             EfiBootServicesCode,
             1,
             &AllocateAddress
             );
  ASSERT_EFI_ERROR (Status);

  EnableBusMaster (
    IohOhciPciReg,
    IOH_MAX_OHCI_USB_CONTROLLERS,
    IohEhciPciReg,
    IOH_MAX_EHCI_USB_CONTROLLERS
    );
  
  if (FeaturePcdGet (PcdEhciRecoveryEnabled)) {
    DEBUG ((EFI_D_INFO, "UsbPei:EHCI is used for recovery\n"));
    //
    // EHCI recovery is enabled
    //
    PeiIohEhciDev = (IOH_EHCI_DEVICE *)((UINTN)AllocateAddress);
    ZeroMem (PeiIohEhciDev, sizeof(IOH_EHCI_DEVICE));
    
    PeiIohEhciDev->Signature            = PEI_IOH_EHCI_SIGNATURE;
    CopyMem(&(PeiIohEhciDev->UsbControllerPpi), &mUsbControllerPpi[1], sizeof(PEI_USB_CONTROLLER_PPI));
    CopyMem(&(PeiIohEhciDev->PpiList), &mPpiList, sizeof(mPpiList));
    PeiIohEhciDev->PpiList.Ppi          = &PeiIohEhciDev->UsbControllerPpi;
    
    //
    // Assign resources and enable Ehci controllers
    //
    for (i = 0; i < IOH_MAX_EHCI_USB_CONTROLLERS; i++) {
      DEBUG ((EFI_D_INFO, "UsbPei:Enable the %dth EHCI controller for recovery\n", i));
      PeiIohEhciDev->MmioBase[i] = PcdGet32(PcdPeiQNCUsbControllerMemoryBaseAddress) + IOH_USB_CONTROLLER_MMIO_RANGE * i;
      //
      // Assign base address register, Enable Bus Master and Memory Io
      //
      PciWrite32 (IohEhciPciReg[i] | R_IOH_USB_MEMBAR, PeiIohEhciDev->MmioBase[i]);
      CmdReg = PciRead16 (IohEhciPciReg[i] | R_IOH_USB_COMMAND);
      CmdReg = (UINT16) (CmdReg | B_IOH_USB_COMMAND_MSE | B_IOH_USB_COMMAND_BME );
      PciWrite16 (IohEhciPciReg[i] | R_IOH_USB_COMMAND, CmdReg);
    } 	
    //
    // Install USB Controller PPI
    //
    Status = (**PeiServices).InstallPpi (
                               PeiServices, 
                               &PeiIohEhciDev->PpiList
                               );
	
    ASSERT_EFI_ERROR (Status);
  } else {
    DEBUG ((EFI_D_INFO, "UsbPei:OHCI is used for recovery\n"));
    //
    // OHCI recovery is enabled
    //
    SwitchConfigFlag ((EFI_PEI_SERVICES**)PeiServices);
    PeiIohOhciDev = (IOH_OHCI_DEVICE *)((UINTN)AllocateAddress);
    ZeroMem (PeiIohOhciDev, sizeof(IOH_OHCI_DEVICE));
    
    PeiIohOhciDev->Signature            = PEI_IOH_OHCI_SIGNATURE;
    CopyMem(&(PeiIohOhciDev->UsbControllerPpi), &mUsbControllerPpi[0], sizeof(PEI_USB_CONTROLLER_PPI));
    CopyMem(&(PeiIohOhciDev->PpiList), &mPpiList, sizeof(mPpiList));
    PeiIohOhciDev->PpiList.Ppi          = &PeiIohOhciDev->UsbControllerPpi;
    //
    // Assign resources and enable OHCI controllers
    //
    for (i = 0; i < IOH_MAX_OHCI_USB_CONTROLLERS; i++) {
      DEBUG ((EFI_D_INFO, "UsbPei:Enable the %dth OHCI controller for recovery\n", i));
      PeiIohOhciDev->MmioBase[i] = PcdGet32(PcdPeiQNCUsbControllerMemoryBaseAddress) + IOH_USB_CONTROLLER_MMIO_RANGE * i;
      //
      // Assign base address register, Enable Bus Master and Memory Io
      //      
      PciWrite32 (IohOhciPciReg[i] | R_IOH_USB_MEMBAR, PeiIohOhciDev->MmioBase[i]);

      Status = PeiServicesAllocatePages (
                 EfiBootServicesCode,
                 1,
                 &AllocateAddress
                 );
      ASSERT_EFI_ERROR (Status);      
      MmioWrite32(PeiIohOhciDev->MmioBase[i] + R_IOH_USB_OHCI_HCCABAR, (UINT32)AllocateAddress);
      
      CmdReg = PciRead16 (IohOhciPciReg[i] | R_IOH_USB_COMMAND);
      CmdReg = (UINT16) (CmdReg | B_IOH_USB_COMMAND_MSE | B_IOH_USB_COMMAND_BME );
      PciWrite16 (IohOhciPciReg[i] | R_IOH_USB_COMMAND, CmdReg);
    }
    //
    // Install USB Controller PPI
    //
    Status = (**PeiServices).InstallPpi (
                               PeiServices,
                               &PeiIohOhciDev->PpiList
                               );
    
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

