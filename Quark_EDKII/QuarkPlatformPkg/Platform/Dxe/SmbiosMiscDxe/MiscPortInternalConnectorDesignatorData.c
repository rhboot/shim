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

  MiscPortInternalConnectorDesignatorData.c
  
Abstract: 

  This driver parses the mSmbiosMiscDataTable structure and reports
  any generated data to the DataHub.

--*/


#include "CommonHeader.h"

#include "SmbiosMisc.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector1) = {
  STRING_TOKEN (STR_MISC_PORT1_INTERNAL_DESIGN),    // PortInternalConnectorDesignator
  STRING_TOKEN (STR_MISC_PORT1_EXTERNAL_DESIGN),    // PortExternalConnectorDesignator
  EfiPortConnectorTypeNone,                         // PortInternalConnectorType
  EfiPortConnectorTypePS2,                          // PortExternalConnectorType
  EfiPortTypeKeyboard,                              // PortType
  //mPs2KbyboardDevicePath                          // PortPath 
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector2) = {
  STRING_TOKEN (STR_MISC_PORT2_INTERNAL_DESIGN),    // PortInternalConnectorDesignator
  STRING_TOKEN (STR_MISC_PORT2_EXTERNAL_DESIGN),    // PortExternalConnectorDesignator
  EfiPortConnectorTypeNone,                         // PortInternalConnectorType
  EfiPortConnectorTypePS2,                          // PortExternalConnectorType
  EfiPortTypeMouse,                                 // PortType
  //mPs2MouseDevicePath                             // PortPath
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector3) = {
  STRING_TOKEN (STR_MISC_PORT3_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT3_EXTERNAL_DESIGN),
  EfiPortConnectorTypeOther,
  EfiPortConnectorTypeNone,
  EfiPortTypeSerial16550ACompatible,
  //mCom1DevicePath
  0
};



MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector4) = {
  STRING_TOKEN (STR_MISC_PORT4_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT4_EXTERNAL_DESIGN),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeRJ45,
  EfiPortTypeSerial16550ACompatible,
  //mCom2DevicePath
    0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector5) = {
  STRING_TOKEN (STR_MISC_PORT5_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT5_EXTERNAL_DESIGN),
  EfiPortConnectorTypeOther,
  EfiPortConnectorTypeNone,
  EfiPortTypeSerial16550ACompatible,
  //mCom3DevicePath
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector6) = {
  STRING_TOKEN (STR_MISC_PORT6_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT6_EXTERNAL_DESIGN),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeRJ45,
  EfiPortTypeSerial16550ACompatible,
  //mCom3DevicePath
    0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector7) = {
  STRING_TOKEN (STR_MISC_PORT7_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT7_EXTERNAL_DESIGN),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeDB25Male,
  EfiPortTypeParallelPortEcpEpp,
  //mLpt1DevicePath
  0
};


MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector8) = {
  STRING_TOKEN (STR_MISC_PORT8_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT8_EXTERNAL_DESIGN),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeUsb,
  EfiPortTypeUsb,
  //mUsb0DevicePath
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector9) = {
  STRING_TOKEN (STR_MISC_PORT9_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT9_EXTERNAL_DESIGN),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeUsb,
  EfiPortTypeUsb,
  //mUsb1DevicePath
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector10) = {
  STRING_TOKEN (STR_MISC_PORT10_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT10_EXTERNAL_DESIGN),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeUsb,
  EfiPortTypeUsb,
  //mUsb2DevicePath
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector11) = {
  STRING_TOKEN (STR_MISC_PORT11_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT11_EXTERNAL_DESIGN),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeUsb,
  EfiPortTypeUsb,
  //mUsb3DevicePath
  0
};


MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector12) = {
  STRING_TOKEN (STR_MISC_PORT12_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT12_EXTERNAL_DESIGN),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeRJ45,
  EfiPortTypeNetworkPort,
  //mGbNicDevicePath
  0
};


MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector13) = {
  STRING_TOKEN (STR_MISC_PORT13_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT13_EXTERNAL_DESIGN),
  EfiPortConnectorTypeOnboardFloppy,
  EfiPortConnectorTypeNone,
  EfiPortTypeOther,
  //mFloopyADevicePath
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector14) = {
  STRING_TOKEN (STR_MISC_PORT14_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT14_EXTERNAL_DESIGN),
  EfiPortConnectorTypeOnboardIde,
  EfiPortConnectorTypeNone,
  EfiPortTypeOther,
  //mIdeDevicePath
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector15) = {
  STRING_TOKEN (STR_MISC_PORT15_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT15_EXTERNAL_DESIGN),
  EfiPortConnectorTypeOnboardIde,
  EfiPortConnectorTypeNone,
  EfiPortTypeOther,
  //mSataDevicePath
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector16) = {
  STRING_TOKEN (STR_MISC_PORT16_INTERNAL_DESIGN),
  STRING_TOKEN (STR_MISC_PORT16_EXTERNAL_DESIGN),
  EfiPortConnectorTypeOnboardIde,
  EfiPortConnectorTypeNone,
  EfiPortTypeOther,
  //mSataDevicePath
  0
};

