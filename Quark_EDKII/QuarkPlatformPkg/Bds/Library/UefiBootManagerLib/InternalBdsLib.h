/** @file
  BDS library definition, include the file and data structure

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
--*/

#ifndef _INTERNAL_BDS_LIB_H_
#define _INTERNAL_BDS_LIB_H_

#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/Atapi.h>

#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadFile.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/PciIo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/IdeControllerInit.h>
#include <Protocol/BootLogo.h>

#include <Guid/MemoryTypeInformation.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>
#include <Guid/Performance.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/TimerLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/CapsuleLib.h>
#include <Library/PerformanceLib.h>

#if !defined (EFI_REMOVABLE_MEDIA_FILE_NAME)
    #if defined (MDE_CPU_EBC)
        //
        // Uefi specification only defines the default boot file name for IA32, X64
        // and IPF processor, so need define boot file name for EBC architecture here.
        //
        #define EFI_REMOVABLE_MEDIA_FILE_NAME L"\\EFI\\BOOT\\BOOTEBC.EFI"
    #else
        #error "Can not determine the default boot file name for unknown processor type!"
    #endif
#endif

typedef enum {
  AcpiFloppyBoot,
  MessageAtapiBoot,
  MessageSataBoot,
  MessageUsbBoot,
  MessageScsiBoot,
  MessageNetworkBoot,
  UnsupportedBoot
} BOOT_TYPE;

typedef
CHAR16 *
(* GET_BOOT_DESCRIPTION) (
  IN EFI_HANDLE          Handle
  );

/**
  Get the revision of the EFI_KEY_OPTION structure.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @return  Revision.
**/
#define KEY_OPTION_REVISION(KeyOption)           ((KeyOption)->KeyData & EFI_KEY_OPTION_REVISION_MASK)

/**
  Get the actual number of entries in EFI_KEY_OPTION.Keys, from 0-3.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @return  Actual number of entries in EFI_KEY_OPTION.Keys.
**/
#define KEY_OPTION_INPUT_KEY_COUNT(KeyOption)    (((KeyOption)->KeyData & EFI_KEY_OPTION_INPUT_KEY_COUNT_MASK) >> LowBitSet32 (EFI_KEY_OPTION_INPUT_KEY_COUNT_MASK))

/**
  Return whether the Shift key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Shift key needs pressed.
  @retval  FALSE Shift key needn't pressed.
**/
#define KEY_OPTION_SHIFT_PRESSED(KeyOption)      (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_SHIFT_PRESSED_MASK) != 0)

/**
  Return whether the Control key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Control key needs pressed.
  @retval  FALSE Control key needn't pressed.
**/
#define KEY_OPTION_CONTROL_PRESSED(KeyOption)    (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_CONTROL_PRESSED_MASK) != 0)

/**
  Return whether the Alt key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Alt key needs pressed.
  @retval  FALSE Alt key needn't pressed.
**/
#define KEY_OPTION_ALT_PRESSED(KeyOption)        (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_ALT_PRESSED_MASK) != 0)

/**
  Return whether the Logo key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Logo key needs pressed.
  @retval  FALSE Logo key needn't pressed.
**/
#define KEY_OPTION_LOGO_PRESSED(KeyOption)       (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_LOGO_PRESSED_MASK) != 0)

/**
  Return whether the Menu key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  Menu key needs pressed.
  @retval  FALSE Menu key needn't pressed.
**/
#define KEY_OPTION_MENU_PRESSED(KeyOption)       (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_MENU_PRESSED_MASK) != 0)

/**
  Return whether the SysReq key needs pressed.

  @param   KeyOption  Pointer to the EFI_KEY_OPTION structure. 

  @retval  TRUE  SysReq key needs pressed.
  @retval  FALSE SysReq key needn't pressed.
**/
#define KEY_OPTION_SYS_REQ_PRESSED(KeyOption)    (BOOLEAN) (((KeyOption)->KeyData & EFI_KEY_OPTION_SYS_REQ_PRESSED_MASK) != 0)

#define HOTKEY_SIGNATURE SIGNATURE_32 ('b', 'm', 'h', 'k')
typedef struct {
  UINT32                    Signature;
  LIST_ENTRY                Link;

  BOOLEAN                   IsContinue;
  UINT16                    BootOption;
  UINT8                     CodeCount;
  UINT8                     WaitingKey;
  EFI_KEY_DATA              KeyData[3];
} HOTKEY;

#define HOTKEY_FROM_LINK(a) CR (a, HOTKEY, Link, HOTKEY_SIGNATURE)

/**
  Get the Option Number that wasn't used.

  @param  OrderVariableName   Could be L"BootOrder" or L"DriverOrder".
  @param  FreeOptionNumber    To receive the minimal free option number.

  @retval EFI_SUCCESS           The option number is found
  @retval EFI_OUT_OF_RESOURCES  There is no free option number that can be used.
  @retval EFI_INVALID_PARAMETER FreeOptionNumber is NULL

**/
EFI_STATUS
GetFreeOptionNumber (
  IN  CHAR16    *OrderVariableName,
  OUT UINT16    *FreeOptionNumber
  );

EFI_STATUS
AddOptionNumberToOrderVariable (
  IN CHAR16               *OrderVariableName,
  IN UINT16               OptionNumber,
  IN UINTN                Position
  );

EFI_STATUS
DeleteOptionVariable (
  IN CHAR16                            *OptionOrderVariable,
  IN UINT16                            OptionNumber
  );

/**

  Allocates a block of memory to store performance data.

**/
VOID
AllocateMemoryForPerformanceData (
  VOID
  );

/**

  Free the memory to store performance data.

**/
VOID
FreeMemoryForPerformanceData (
  VOID
  );

/**

  Writes performance data of booting into the allocated memory.
  OS can process these records.

  @param  Event                 The triggered event.
  @param  Context               Context for this event.

**/
VOID
EFIAPI
WriteBootToOsPerformanceData (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );


/**
  Get the headers (dos, image, optional header) from an image

  @param  Device                SimpleFileSystem device handle
  @param  FileName              File name for the image
  @param  DosHeader             Pointer to dos header
  @param  Hdr                   The buffer in which to return the PE32, PE32+, or TE header.

  @retval EFI_SUCCESS           Successfully get the machine type.
  @retval EFI_NOT_FOUND         The file is not found.
  @retval EFI_LOAD_ERROR        File is not a valid image file.

**/
EFI_STATUS
GetImageHeader (
  IN  EFI_HANDLE                  Device,
  IN  CHAR16                      *FileName,
  OUT EFI_IMAGE_DOS_HEADER        *DosHeader,
  OUT EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr
  );

INTN
FindLoadOption (
  IN EFI_BOOT_MANAGER_LOAD_OPTION   *Key,
  IN EFI_BOOT_MANAGER_LOAD_OPTION   *Array,
  IN UINTN                          Count
  );

/**
  For a bootable Device path, return its boot type.

  @param  DevicePath                   The bootable device Path to check

  @retval AcpiFloppyBoot               If given device path contains ACPI_DEVICE_PATH type device path node
                                       which HID is floppy device.
  @retval MessageAtapiBoot             If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_ATAPI_DP.
  @retval MessageSataBoot              If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_SATA_DP.
  @retval MessageScsiBoot              If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_SCSI_DP.
  @retval MessageUsbBoot               If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_USB_DP.
  @retval MessageNetworkBoot           If given device path contains MESSAGING_DEVICE_PATH type device path node
                                       and its last device path node's subtype is MSG_MAC_ADDR_DP, MSG_VLAN_DP,
                                       MSG_IPv4_DP or MSG_IPv6_DP.
  @retval MiscBoot                     Other boot type,

**/
BOOT_TYPE
BootTypeFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  );

/**
  This routine adjust the memory information for different memory type and 
  save them into the variables for next boot.
**/
VOID
SetMemoryTypeInformationVariable (
  VOID
  );

/**
  Check whether there is a instance in BlockIoDevicePath, which contain multi device path
  instances, has the same partition node with HardDriveDevicePath device path

  @param  BlockIoDevicePath      Multi device path instances which need to check
  @param  HardDriveDevicePath    A device path which starts with a hard drive media
                                 device path.

  @retval TRUE                   There is a matched device path instance.
  @retval FALSE                  There is no matched device path instance.

**/
BOOLEAN
MatchPartitionDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL   *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  );

/**
  This function will register the new boot#### or driver#### options.
  After the boot#### or driver#### updated, the BootOrder or DriverOrder will also be updated.

  @param  Options           Pointer to array of load options to add.
  @param  OptionCount       Number of load options.
  @param  Position          Position of the new load option to put in the ****Order variable.

  @retval EFI_SUCCESS           The boot#### or driver#### have been success
                                registered
  @retval EFI_STATUS            Return the status of gRT->SetVariable ().

**/
EFI_STATUS
AddLoadOptions (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Options,
  IN UINTN                              OptionCount,
  IN UINTN                              Position
  );

/**
  This function will connect all current system handles recursively.     
  gBS->ConnectController() service is invoked for each handle exist in system handler buffer.  
  If the handle is bus type handler, all childrens will be connected recursively by gBS->ConnectController().
**/
VOID
ConnectAllEfi (
  VOID
  );


/**
  Connect all the drivers to all the controllers.

  This function makes sure all the current system drivers manage the correspoinding
  controllers if have. And at the same time, makes sure all the system controllers
  have driver to manage it if have.
**/
VOID
ConnectAllDriversToAllControllers (
  VOID
  );

/**
  Return the array of key options.

  @param Count  Return the number of key options.

  @retval NULL  No key option.
  @retval Other Pointer to the key options.
**/
EFI_BOOT_MANAGER_KEY_OPTION *
EFIAPI
EfiBootManagerGetKeyOptions (
  OUT UINTN     *Count
  );

/**
  Free the key options returned from EfiBootManagerGetKeyOptions.

  @param KeyOptions     Pointer to the key options.
  @param KeyOptionCount Number of the key options.

  @retval EFI_SUCCESS   The key options are freed.
  @retval EFI_NOT_FOUND KeyOptions is NULL.
**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeKeyOptions (
  IN EFI_BOOT_MANAGER_KEY_OPTION    *KeyOptions,
  IN UINTN                          KeyOptionCount
  );

/**
  Connect the specific Usb device which match the short form device path.

  @param  DevicePath             A short-form device path that starts with the first
                                 element being a USB WWID or a USB Class device
                                 path

  @return EFI_INVALID_PARAMETER  DevicePath is NULL pointer.
                                 DevicePath is not a USB device path.

  @return EFI_SUCCESS            Success to connect USB device
  @return EFI_NOT_FOUND          Fail to find handle for USB controller to connect.

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectUsbShortFormDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  );
  
/**
  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created successfully, Dispatch service which load the missing drivers
  is called according to input parameter, since in some cases no driver 
  dependency is assumed exist, so may need not to call this service.

  @param  DevicePathToConnect   The device path which will be connected, it CANNOT be
                                a multi-instance device path
  @param  NeedDispatch          Whether requires dispatch service during connection                                 
  @param  MatchingHandle        Return the controller handle closest to the DevicePathToConnect

  @retval EFI_INVALID_PARAMETER DevicePathToConnect is NULL.
  @retval EFI_NOT_FOUND         Failed to create all handles associate with every device path node.
  @retval EFI_SUCCESS           Successful to create all handles associate with every device path node.

**/
EFI_STATUS
ConnectDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect,
  IN BOOLEAN                    NeedDispatch,
  OUT EFI_HANDLE                *MatchingHandle          OPTIONAL
  );

extern EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION  mEfiBootManagerRefreshLegacyBootOption;
extern EFI_BOOT_MANAGER_LEGACY_BOOT                 mEfiBootManagerLegacyBoot;
#endif // _BDS_LIB_H_
