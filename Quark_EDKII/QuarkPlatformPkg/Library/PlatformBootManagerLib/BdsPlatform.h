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

  BdsPlatform.h

Abstract:

  Head file for BDS Platform specific code

--*/

#ifndef _BDS_PLATFORM_H
#define _BDS_PLATFORM_H

#include <PiDxe.h>
#include <Ioh.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/LoadFile.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/PciIo.h>
#include <Protocol/UserManager.h>
#include <Protocol/DeferredImageLoad.h>
#include <Protocol/GenericMemoryTest.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/ExitPmAuth.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/PciPlatform.h>
#include <Protocol/AcpiS3Save.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/SecureBootHelper.h>
#include <Protocol/PlatformType.h>

#include <Guid/CapsuleVendor.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/GlobalVariable.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/PlatformBootManagerLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/PrintLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/LogoLib.h>
#include <Library/HiiLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/PlatformHelperLib.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Atapi.h>

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} BDS_CONSOLE_CONNECT_ENTRY;

#define CONSOLE_OUT 0x00000001
#define STD_ERROR   0x00000002
#define CONSOLE_IN  0x00000004
#define CONSOLE_ALL (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)

extern EFI_DEVICE_PATH_PROTOCOL  *gPlatformRootBridges [];
extern BDS_CONSOLE_CONNECT_ENTRY gPlatformConsole [];
extern EFI_DEVICE_PATH_PROTOCOL  *gPlatformAllPossibleAgpConsole [];
extern EFI_DEVICE_PATH_PROTOCOL  *gPlatformConnectSequence [];
extern EFI_DEVICE_PATH_PROTOCOL  *gPlatformDriverOption [];
extern EFI_DEVICE_PATH_PROTOCOL  *gPlatformBootOption [];
extern EFI_DEVICE_PATH_PROTOCOL  *gUserAuthenticationDevice[];
#define gPciRootBridge \
  { \
    ACPI_DEVICE_PATH, ACPI_DP, (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), (UINT8) \
      ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), EISA_PNP_ID (0x0A03), 0 \
  }

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, END_DEVICE_PATH_LENGTH, 0 \
  }

//
// Platform Root Bridge
//
typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ROOT_BRIDGE_DEVICE_PATH;


typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           PcieBridge;
  PCI_DEVICE_PATH           PcieDevice;
  ACPI_ADR_DEVICE_PATH      DisplayDevice;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_PCIE_DEVICE_PATH;

//
// Below is the platform USB controller device path for 
// USB disk as user authentication device. 
//
typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           PciDevice;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_USB_DEVICE_PATH;

typedef struct {
  USB_CLASS_DEVICE_PATH           UsbClass;
  EFI_DEVICE_PATH_PROTOCOL        End;
} USB_CLASS_FORMAT_DEVICE_PATH;  

typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           SerialDevice;
  UART_DEVICE_PATH          Uart;
  VENDOR_DEVICE_PATH        TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_PCI_SERIAL_DEVICE_PATH;

#define CLASS_HID           3
#define SUBCLASS_BOOT       1
#define PROTOCOL_KEYBOARD   1

//
// Platform BDS global variables.
//

extern PLATFORM_PCI_SERIAL_DEVICE_PATH    gSerialDevicePath;
extern SECUREBOOT_HELPER_PROTOCOL         *gSecureBootHelperProtocol;
extern EFI_PLATFORM_TYPE_PROTOCOL         *gPlatformType;

//
// Platform BDS Functions
//

/**
  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

  @param  Level         The memory test intensive level.

  @retval EFI_STATUS    Success test all the system memory and update
                        the memory resource

**/
EFI_STATUS
MemoryTest (
  IN EXTENDMEM_COVERAGE_LEVEL Level
  );

VOID
RegisterLoadOptions (
  VOID
  );

VOID
ConnectSequence (
  VOID
  )
/*++

Routine Description:

  Connect with predeined platform connect sequence, 
  the OEM/IBV can customize with their own connect sequence.
  
Arguments:

  None.
 
Returns:

  None.
  
--*/
;

/**
  Connect the predefined platform default authentication devices.

  This function connects the predefined device path for authentication device,
  and if the predefined device path has child device path, the child handle will 
  be connected too. But the child handle of the child will not be connected.

**/
VOID
ConnectAuthDevice (
  VOID
  );
  
/**
  This function is to identify a user, and return whether deferred images exist.

  @param[out]  User               Point to user profile handle.
  @param[out]  DeferredImageExist On return, points to TRUE if the deferred image
                                  exist or FALSE if it did not exist.

**/
VOID
CheckDeferredImage (
  OUT EFI_USER_PROFILE_HANDLE        *User,
  OUT BOOLEAN                        *DeferredImage
  );
  
VOID
InitializeStringSupport (
  VOID
  );
VOID
PrintBootPrompt (
  VOID
  );
VOID
PrintMfgModePrompt (
  VOID
  );
VOID
LockNonUpdatableFlash (
  VOID
  );
#endif
