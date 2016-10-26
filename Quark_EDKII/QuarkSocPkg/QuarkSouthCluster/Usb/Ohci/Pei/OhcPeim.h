/** @file
  Provides the definition of Usb Hc Protocol and OHCI controller 
  private data structure.

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



#ifndef _OHCI_PEIM_H
#define _OHCI_PEIM_H

#include <PiPei.h>

#include <Ppi/UsbController.h>
#include <Ppi/UsbHostController.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/IoLib.h>
#include <Library/RedirectPeiServicesLib.h>

typedef struct _USB_OHCI_HC_DEV USB_OHCI_HC_DEV;

#include "UsbHcMem.h"
#include "OhciReg.h"
#include "OhciSched.h"
#include "OhciUrb.h"
#include "Descriptor.h"

#define EFI_USB_SPEED_FULL 0x0000
#define EFI_USB_SPEED_LOW  0x0001
#define EFI_USB_SPEED_HIGH 0x0002

#define PAGESIZE                    4096

#define HC_1_MICROSECOND            1
#define HC_1_MILLISECOND            (1000 * HC_1_MICROSECOND)
#define HC_1_SECOND                 (1000 * HC_1_MILLISECOND)


#define USB_OHCI_HC_DEV_SIGNATURE   SIGNATURE_32('o','h','c','i')

struct _USB_OHCI_HC_DEV {
  UINTN                        Signature;
  PEI_USB_HOST_CONTROLLER_PPI  UsbHostControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR       PpiDescriptor;
  UINT32                       UsbHostControllerBaseAddress;
  VOID                         *MemPool;
};

#define PEI_RECOVERY_USB_OHC_DEV_FROM_EHCI_THIS(a)  CR (a, USB_OHCI_HC_DEV, UsbHostControllerPpi, USB_OHCI_HC_DEV_SIGNATURE)

//
// Func List
//

/**
  Provides software reset for the USB host controller.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  Attributes            A bit mask of the reset operation to perform.

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_UNSUPPOURTED      The type of reset specified by Attributes is
                                not currently supported by the host controller.
  @retval EFI_DEVICE_ERROR      Host controller isn't halted to reset.

**/
EFI_STATUS
InitializeUsbHC (
  IN EFI_PEI_SERVICES   **PeiServices,
  IN USB_OHCI_HC_DEV    *Ohc,
  IN UINT16             Attributes
  );

/**
  Submits control transfer to a target USB device.
  
  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  DeviceAddress          The target device address.
  @param  DeviceSpeed            Target device speed.
  @param  MaximumPacketLength    Maximum packet size the default control transfer 
                                 endpoint is capable of sending or receiving.
  @param  Request                USB device request to send.
  @param  TransferDirection      Specifies the data direction for the data stage.
  @param  Data                   Data buffer to be transmitted or received from USB device.
  @param  DataLength             The size (in bytes) of the data buffer.
  @param  TimeOut                Indicates the maximum timeout, in millisecond.
  @param  TransferResult         Return the result of this control transfer.

  @retval EFI_SUCCESS            Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES   The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_TIMEOUT            Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR       Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
OhciControlTransfer (
  IN  EFI_PEI_SERVICES                    **PeiServices,
  IN  PEI_USB_HOST_CONTROLLER_PPI         *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               DeviceSpeed,
  IN  UINT8                               MaxPacketLength,
  IN  EFI_USB_DEVICE_REQUEST              *Request,
  IN  EFI_USB_DATA_DIRECTION              TransferDirection,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  UINTN                               TimeOut,
  OUT UINT32                              *TransferResult
  );
/**
  Submits bulk transfer to a bulk endpoint of a USB device.
  
  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction in bit 7.
  @param  MaxiPacketLength      Maximum packet size the endpoint is capable of 
                                sending or receiving.
  @param  Data                  A pointers to the buffers of data to transmit 
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  DataToggle            On input, the initial data toggle for the transfer;
                                On output, it is updated to to next data toggle to use of 
                                the subsequent bulk transfer.
  @param  TimeOut               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete.                                
  @param  TransferResult        A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
OhciBulkTransfer (
  IN EFI_PEI_SERVICES                     **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI          *This,
  IN  UINT8                               DeviceAddress,
  IN  UINT8                               EndPointAddress,
  IN  UINT8                               MaxPacketLength,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  OUT UINT32                              *TransferResult
  );
/**
  Retrieves the number of root hub ports.

  @param[in]  PeiServices       The pointer to the PEI Services Table.
  @param[in]  This              The pointer to this instance of the 
                                PEI_USB_HOST_CONTROLLER_PPI.
  @param[out] NumOfPorts        The pointer to the number of the root hub ports.                                
                                
  @retval EFI_SUCCESS           The port number was retrieved successfully.
  @retval EFI_INVALID_PARAMETER PortNumber is NULL.

**/

EFI_STATUS
EFIAPI
OhciGetRootHubNumOfPorts (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI           *This,
  OUT UINT8                *NumOfPorts
  );
/**
  Retrieves the current status of a USB root hub port.
  
  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  PortNumber             The root hub port to retrieve the state from.  
  @param  PortStatus             Variable to receive the port state.

  @retval EFI_SUCCESS            The status of the USB root hub port specified.
                                 by PortNumber was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid.

**/

EFI_STATUS
EFIAPI
OhciGetRootHubPortStatus (
  IN  EFI_PEI_SERVICES             **PeiServices,
  IN  PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN  UINT8                        PortNumber,
  OUT EFI_USB_PORT_STATUS          *PortStatus
  );
/**

  Sets a feature for the specified root hub port.

  @param  This                  A pointer to the EFI_USB_HC_PROTOCOL.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be set.
  @param  PortFeature           Indicates the feature selector associated
                                with the feature set request.

  @retval EFI_SUCCESS           The feature specified by PortFeature was set for the
                                USB root hub port specified by PortNumber.
  @retval EFI_DEVICE_ERROR      Set feature failed because of hardware issue
  @retval EFI_INVALID_PARAMETER PortNumber is invalid or PortFeature is invalid.
**/
EFI_STATUS
EFIAPI
OhciSetRootHubPortFeature (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN UINT8                        PortNumber,
  IN EFI_USB_PORT_FEATURE         PortFeature
  );
/**
  Clears a feature for the specified root hub port.
  
  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS            The feature specified by PortFeature was cleared 
                                 for the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid or PortFeature is invalid.

**/

EFI_STATUS
EFIAPI
OhciClearRootHubPortFeature (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN UINT8                        PortNumber,
  IN EFI_USB_PORT_FEATURE         PortFeature
  );
#endif
