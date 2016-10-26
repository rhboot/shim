/** @file
  Legacy Interrupt Component.

  This component implements the Legacy Interrupt Protocol defined in the Framework CSM Spec.
  
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
#include "LegacyInterrupt.h"

//
// Protocol interface function prototypes
//

EFI_STATUS
EFIAPI
GetNumberPirqs (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *NumberPirqs
  );

EFI_STATUS
EFIAPI
GetLocation (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *Bus,
  OUT UINT8                          *Device,
  OUT UINT8                          *Function
  );

EFI_STATUS
EFIAPI
ReadPirq (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN  UINT8                          PirqNumber,
  OUT UINT8                          *PirqData
  );

EFI_STATUS
EFIAPI
WritePirq (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN UINT8                          PirqNumber,
  IN UINT8                          PirqData
  );

//
// Handle for the Legacy Interrupt Protocol instance produced by this driver
//
static EFI_HANDLE mLegacyInterruptHandle = NULL;

//
// The Legacy Interrupt Protocol instance produced by this driver
//
static EFI_LEGACY_INTERRUPT_PROTOCOL mLegacyInterrupt = {
  GetNumberPirqs,
  GetLocation,
  ReadPirq,
  WritePirq
};

//
// PIRQ Route Control registers' offsets within the LPC configuration space for PIRQA~H
//
static UINT8 mPirqRouteReg[QNC_NUMBER_PIRQS] = {
  R_QNC_LPC_PIRQA_ROUT,
  R_QNC_LPC_PIRQB_ROUT,
  R_QNC_LPC_PIRQC_ROUT,
  R_QNC_LPC_PIRQD_ROUT,
  R_QNC_LPC_PIRQE_ROUT,
  R_QNC_LPC_PIRQF_ROUT,
  R_QNC_LPC_PIRQG_ROUT,
  R_QNC_LPC_PIRQH_ROUT
};

/**
  Gets the number of PIRQs that this hardware supports.

  @param[in]   This         Indicates the EFI_LEGACY_INTERRUPT_PROTOCOL instance.
  @param[out]  NumberPirqs  Number of PIRQs that are supported.

  @retval  EFI_SUCCESS  The number of PIRQs was returned successfully.

**/
EFI_STATUS
EFIAPI
GetNumberPirqs (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *NumberPirqs
  )
{
  ASSERT (NumberPirqs != NULL);
  
  *NumberPirqs = QNC_NUMBER_PIRQS;

  return EFI_SUCCESS;
}

/**
  Gets the PCI location associated with this protocol.

  @param[in]   This      Indicates the EFI_LEGACY_INTERRUPT_PROTOCOL instance.
  @param[out]  Bus       PCI bus number of this device.
  @param[out]  Device    PCI device number of this device.
  @param[out]  Function  PCI function number of this device.

  @retval  EFI_SUCCESS  The PCI location was returned successfully.

**/
EFI_STATUS
EFIAPI
GetLocation (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  OUT UINT8                          *Bus,
  OUT UINT8                          *Device,
  OUT UINT8                          *Function
  )
{
  ASSERT (Bus != NULL && Device != NULL && Function != NULL);

  *Bus      = PCI_BUS_NUMBER_QNC;
  *Device   = PCI_DEVICE_NUMBER_QNC_LPC;
  *Function = PCI_FUNCTION_NUMBER_QNC_LPC;

  return EFI_SUCCESS;
}

/**
  Reads the given PIRQ register and returns the IRQ that is assigned to it.

  @param[in]   This        Indicates the EFI_LEGACY_INTERRUPT_PROTOCOL instance.
  @param[in]   PirqNumber  PIRQ A = 0, PIRQ B = 1, and so on.
  @param[out]  PirqData    IRQ assigned to this PIRQ.

  @retval  EFI_SUCCESS            The data was returned successfully.
  @retval  EFI_INVALID_PARAMETER  The PIRQ number invalid.

**/
EFI_STATUS
EFIAPI
ReadPirq (
  IN  EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN  UINT8                          PirqNumber,
  OUT UINT8                          *PirqData
  )
{
  if (PirqNumber >= QNC_NUMBER_PIRQS) {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (PirqData != NULL);
  
  *PirqData = (UINT8)(PciRead8 (PCI_QNC_LPC_ADDRESS (mPirqRouteReg[PirqNumber])) & 0x7f);
                
  return EFI_SUCCESS;
}

/**
  Writes data to the specified PIRQ register.

  @param[in]  This        Indicates the EFI_LEGACY_INTERRUPT_PROTOCOL instance.
  @param[in]  PirqNumber  PIRQ A = 0, PIRQ B = 1, and so on.
  @param[in]  PirqData    IRQ assigned to this PIRQ.

  @retval  EFI_SUCCESS            The PIRQ was programmed.
  @retval  EFI_INVALID_PARAMETER  The PIRQ number invalid.

**/
EFI_STATUS
EFIAPI
WritePirq (
  IN EFI_LEGACY_INTERRUPT_PROTOCOL  *This,
  IN UINT8                          PirqNumber,
  IN UINT8                          PirqData
  )
{
  if (PirqNumber >= QNC_NUMBER_PIRQS) {
    return EFI_INVALID_PARAMETER;
  }

  PciWrite8 (PCI_QNC_LPC_ADDRESS (mPirqRouteReg[PirqNumber]), PirqData);
    
  return EFI_SUCCESS;
}

/**
  Install Legacy Interrupt Protocol.

  @retval  EFI_SUCCESS  Installed Legacy Interrupt Protocol successfully.
  @retval  !EFI_SUCESS  Error installing Legacy Interrupt Protocol.

**/
EFI_STATUS
LegacyInterruptInstall (
  VOID
  )
{
  //
  // Make sure the Legacy Interrupt Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiLegacyInterruptProtocolGuid);

  //
  // Make a new handle and install the protocol
  //
  return gBS->InstallMultipleProtocolInterfaces (
                &mLegacyInterruptHandle,
                &gEfiLegacyInterruptProtocolGuid,
                &mLegacyInterrupt,
                NULL
                );
}
