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
--*/


#ifndef _UEFI_BOOT_MANAGER_LIB_H_
#define _UEFI_BOOT_MANAGER_LIB_H_

//
// Boot Manager load option library functions.
//

//
// Load Option Type
//
typedef enum {
  LoadOptionTypeBoot,
  LoadOptionTypeDriver,
  LoadOptionTypeMax
} EFI_BOOT_MANAGER_LOAD_OPTION_TYPE;

typedef enum {
  LoadOptionNumberMax = 0x10000,
  LoadOptionNumberUnassigned = LoadOptionNumberMax
} EFI_BOOT_MANAGER_LOAD_OPTION_NUMBER;

//
// Common structure definition for DriverOption and BootOption
//
typedef struct {
  //
  // Data read from UEFI NV variables
  //
  UINTN                             OptionNumber;       // #### numerical value, could be LoadOptionNumberUnassigned
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE OptionType;         // LoadOptionTypeBoot or LoadOptionTypeDriver
  UINT32                            Attributes;         // Load Option Attributes
  CHAR16                            *Description;       // Load Option Description
  EFI_DEVICE_PATH_PROTOCOL          *FilePath;          // Load Option Device Path
  UINT8                             *OptionalData;      // Load Option optional data to pass into image
  UINT32                            OptionalDataSize;   // Load Option size of OptionalData
  BOOLEAN                           BootNext;           // TRUE if this was a L"BootNext" Variable

  //
  // Used at runtime
  //
  EFI_STATUS                        Status;             // Status returned from boot attempt gBS->StartImage ()
  CHAR16                            *ExitData;          // Exit data returned from gBS->StartImage () 
  UINTN                             ExitDataSize;       // Size of ExitData
} EFI_BOOT_MANAGER_LOAD_OPTION;

/**
  Returns an array of load options based on the EFI variable
  L"BootOrder"/L"DriverOrder" and the L"Boot####"/L"Driver####" variables impled by it.
  #### is the hex value of the UINT16 in each BootOrder/DriverOrder entry. 

  @param  LoadOptionCount   Returns number of entries in the array.
  @param  LoadOptionType    The type of the load option.

  @retval NULL  No load options exist.
  @retval !NULL Array of load option entries.

**/
EFI_BOOT_MANAGER_LOAD_OPTION *
EFIAPI
EfiBootManagerGetLoadOptions (
  OUT UINTN                            *LoadOptionCount,
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE LoadOptionType
  );

/**
  Free an array of load options which return from
  EfiBootManagerGetLoadOptions().

  @param  LoadOptions      Pointer to the array of load options to free.
  @param  LoadOptionCount  Number of array entries in LoadOptions.

  @return EFI_SUCCESS           LoadOptions was freed.
  @return EFI_INVALID_PARAMETER LoadOptions is NULL.
**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeLoadOptions (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOptions,
  IN  UINTN                         LoadOptionCount
  );

/**
  Initialize a load option.

  @param Option           Pointer to the load option to be initialized.
  @param OptionNumber     Option number of the load option.
  @param OptionType       Type of the load option.
  @param Attributes       Attributes of the load option.
  @param Description      Description of the load option.
  @param FilePath         Device path of the load option.
  @param OptionalData     Optional data of the load option.
  @param OptionalDataSize Size of the optional data of the load option.

  @retval EFI_SUCCESS           The load option was initialized successfully.
  @retval EFI_INVALID_PARAMETER Option, Description or FilePath is NULL.
**/
EFI_STATUS
EFIAPI
EfiBootManagerInitializeLoadOption (
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION   *Option,
  IN  UINTN                             OptionNumber,
  IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE OptionType,
  IN  UINT32                            Attributes,
  IN  CHAR16                            *Description,
  IN  EFI_DEVICE_PATH_PROTOCOL          *FilePath,
  IN  UINT8                             *OptionalData,
  IN  UINT32                            OptionalDataSize
  );

/**
  Free a load option that was created by the library.

  @param  LoadOption   Pointer to the load option to free.
  CONCERN: Check Boot#### instead of BootOrder, optimize, spec clarify
  @return EFI_SUCCESS           LoadOption was freed.
  @return EFI_INVALID_PARAMETER LoadOption is NULL.

**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeLoadOption (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOption
  );

/**
  Initialize the load option from the VariableName.

  @param  VariableName          EFI Variable name which could be Boot#### or
                                Driver####
  @param  LoadOption            Pointer to the load option to be initialized

  @retval EFI_SUCCESS           The option was created
  @retval EFI_INVALID_PARAMETER VariableName or LoadOption is NULL.
  @retval EFI_NOT_FOUND         The variable specified by VariableName cannot be found.
**/
EFI_STATUS
EFIAPI
EfiBootManagerVariableToLoadOption (
  IN CHAR16                           *VariableName,
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *LoadOption
  );

/**
  Create the Boot#### or Driver#### variable from the load option.
  
  @param  LoadOption      Pointer to the load option.

  @retval EFI_SUCCESS     The variable was created.
  @retval Others          Error status returned by RT->SetVariable.
**/
EFI_STATUS
EFIAPI
EfiBootManagerLoadOptionToVariable (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION     *LoadOption
  );

/**
  This function will update the Boot####/Driver#### and the BootOrder/DriverOrder
  to add a new load option.

  @param  Option        Pointer to load option to add.
  @param  Position      Position of the new load option to put in the BootOrder/DriverOrder.

  @retval EFI_SUCCESS   The load option has been successfully added.
  @retval Others        Error status returned by RT->SetVariable.
**/
EFI_STATUS
EFIAPI
EfiBootManagerAddLoadOptionVariable (
  IN EFI_BOOT_MANAGER_LOAD_OPTION  *Option,
  IN UINTN                         Position
  );

/**
  Delete the load option according to the OptionNumber and OptionType.
  
  Only the BootOrder/DriverOrder is updated to remove the reference of the OptionNumber.
  
  @param  OptionNumber        Option number of the load option.
  @param  OptionType          Type of the load option.

  @retval EFI_NOT_FOUND       The load option cannot be found.
  @retval EFI_SUCCESS         The load option was deleted.
**/
EFI_STATUS
EFIAPI
EfiBootManagerDeleteLoadOptionVariable (
  IN UINTN                              OptionNumber,
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  OptionType
  );

/**
  The comparator to be used by EfiBootManagerSortLoadOption.
  
  @param Left    Point to the EFI_BOOT_MANAGER_LOAD_OPTION.
  @param Right   Point to the EFI_BOOT_MANAGER_LOAD_OPTION.

  @retval TRUE   Left load option should be in front of the Right load option.
  @retval FALSE  Right load option should be in front of the Left load option.
**/
typedef
BOOLEAN
(EFIAPI *EFI_BOOT_MANAGER_LOAD_OPTION_COMPARATOR) (
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Left,
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Right
  );

/**
  Sort the load options. The DriverOrder/BootOrder variables will be re-created to 
  reflect the new order.
TODO: Expose or not?

  @param OptionType        The type of the load option.
  @param Comparator        The comparator function pointer.
**/
VOID
EFIAPI
EfiBootManagerSortLoadOptionVariable (
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE        OptionType,
  EFI_BOOT_MANAGER_LOAD_OPTION_COMPARATOR  Comparator
  );


/**
  Return the index of the load option in the load option array.

  The function consider two load options are equal when the 
  OptionType, Attributes, Description, FilePath and OptionalData are equal.
TODO: Expose or not?

  @param Key    Pointer to the load option to be found.
  @param Array  Pointer to the array of load options to be found.
  @param Count  Number of entries in the Array.

  @retval -1          Key wasn't found in the Array.
  @retval 0 ~ Count-1 The index of the Key in the Array.
**/
INTN
EFIAPI
EfiBootManagerFindLoadOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Array,
  IN UINTN                              Count
  );

//
// Boot Manager hot key library functions.
//

#pragma pack(1)
///
/// EFI Key Option.
///
typedef struct {
  ///
  /// Specifies options about how the key will be processed.
  ///
  EFI_BOOT_KEY_DATA  KeyData;
  ///
  /// The CRC-32 which should match the CRC-32 of the entire EFI_LOAD_OPTION to
  /// which BootOption refers. If the CRC-32s do not match this value, then this key
  /// option is ignored.
  ///
  UINT32             BootOptionCrc;
  ///
  /// The Boot#### option which will be invoked if this key is pressed and the boot option
  /// is active (LOAD_OPTION_ACTIVE is set).
  ///
  UINT16             BootOption;
  ///
  /// The key codes to compare against those returned by the
  /// EFI_SIMPLE_TEXT_INPUT and EFI_SIMPLE_TEXT_INPUT_EX protocols.
  /// The number of key codes (0-3) is specified by the EFI_KEY_CODE_COUNT field in KeyOptions.
  ///
  EFI_INPUT_KEY      Keys[3];
  UINT16             OptionNumber;
} EFI_BOOT_MANAGER_KEY_OPTION;
#pragma pack()

/**
  Start the hot key service so that the key press can trigger the boot option.

  @param HotkeyTriggered  Return the waitable event and it will be signaled 
                          when a valid hot key is pressed.

  @retval EFI_SUCCESS     The hot key service is started.
**/
  
EFI_STATUS
EFIAPI
EfiBootManagerStartHotkeyService (
  IN EFI_EVENT      *HotkeyTriggered
  );

//
// Modifier for EfiBootManagerAddKeyOptionVariable and EfiBootManagerDeleteKeyOptionVariable
//
#define EFI_BOOT_MANAGER_SHIFT_PRESSED    0x00000001
#define EFI_BOOT_MANAGER_CONTROL_PRESSED  0x00000002
#define EFI_BOOT_MANAGER_ALT_PRESSED      0x00000004
#define EFI_BOOT_MANAGER_LOGO_PRESSED     0x00000008
#define EFI_BOOT_MANAGER_MENU_KEY_PRESSED 0x00000010
#define EFI_BOOT_MANAGER_SYS_REQ_PRESSED  0x00000020

/**
  Add the key option.
  It adds the key option variable and the key option takes affect immediately.

  @param AddedOption      Return the added key option.
  @param BootOptionNumber The boot option number for the key option.
  @param Modifier         Key shift state.
  @param ...              Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS         The key option is added.
  @retval EFI_ALREADY_STARTED The hot key is already used by certain key option.
**/
EFI_STATUS
EFIAPI
EfiBootManagerAddKeyOptionVariable (
  OUT EFI_BOOT_MANAGER_KEY_OPTION *AddedOption,   OPTIONAL
  IN UINT16                       BootOptionNumber,
  IN UINT32                       Modifier,
  ...
  );

/**
  Delete the Key Option variable and unregister the hot key

  @param DeletedOption  Return the deleted key options.
  @param Modifier       Key shift state.
  @param ...            Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS   The key option is deleted.
  @retval EFI_NOT_FOUND The key option cannot be found.
**/
EFI_STATUS
EFIAPI
EfiBootManagerDeleteKeyOptionVariable (
  IN EFI_BOOT_MANAGER_KEY_OPTION *DeletedOption, OPTIONAL
  IN UINT32                      Modifier,
  ...
  );

/**
  Register the key option to exit the waiting of the Boot Manager timeout.
  Platform should ensure that the continue key option isn't conflict with
  other boot key options.

  @param Modifier     Key shift state.
  @param  ...         Parameter list of pointer of EFI_INPUT_KEY.

  @retval EFI_SUCCESS         Successfully register the continue key option.
  @retval EFI_ALREADY_STARTED The continue key option is already registered.
**/
EFI_STATUS
EFIAPI
EfiBootManagerRegisterContinueKeyOption (
  IN UINT32           Modifier,
  ...
  );

/**
  Try to boot the boot option triggered by hot key.
**/
VOID
EFIAPI
EfiBootManagerHotkeyBoot (
  VOID
  );
//
// Boot Manager boot library functions.
//

/**
  For the result returned from EfiBootManagerEnumerateBootOptions, the function adds
  the missing ones and removes the invalid ones.
  
  The function won't delete the boot option not added by itself.
**/
VOID
EFIAPI
EfiBootManagerRefreshAllBootOption (
  VOID
  );

/**
  Attempt to boot the EFI boot option. This routine sets L"BootCurent", clears L"BootNext",
  and signals the EFI ready to boot event. If the device path for the option
  starts with a BBS device path a legacy boot is attempted. Short form device paths are 
  also supported via this rountine. A device path starting with 
  MEDIA_HARDDRIVE_DP, MSG_USB_WWID_DP, MSG_USB_CLASS_DP gets expaned out
  to find the first device that matches. If the BootOption Device Path 
  fails the removable media boot algorithm is attempted (\EFI\BOOTIA32.EFI,
  \EFI\BOOTX64.EFI,... only one file type is tried per processor type)

  @param  BootOption    Boot Option to try and boot.
                        On return, BootOption->Status contains the boot status:
                        EFI_SUCCESS     BootOption was booted
                        EFI_UNSUPPORTED BootOption isn't supported.
                        EFI_NOT_FOUND   The BootOption was not found on the system
                        Others          BootOption failed with this error status

**/
VOID
EFIAPI
EfiBootManagerBoot (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  );

/**
  Emuerate all possible bootable medias in the following order:
  1. Removable BlockIo            - The boot option only points to the removable media
                                    device, like USB key, DVD, Floppy etc.
  2. Fixed BlockIo                - The boot option only points to a Fixed blockIo device,
                                    like HardDisk.
  3. Non-BlockIo SimpleFileSystem - The boot option points to a device supporting
                                    SimpleFileSystem Protocol, but not supporting BlockIo
                                    protocol.
  4. LoadFile                     - The boot option points to the media supporting 
                                    LoadFile protocol.
  Reference: UEFI Spec chapter 3.3 Boot Option Variables Default Boot Behavior
**/
EFI_BOOT_MANAGER_LOAD_OPTION *
EFIAPI
EfiBootManagerEnumerateBootOptions (
  UINTN                                 *BootOptionCount
  );

/**
  Return the Boot Manager Menu.
  TODO: Expose or not? Alternative way is to add new field IsBootManagerMenu in EFI_BOOT_MANAGER_LOAD_OPTION.
 
  @param BootOption    Return the Boot Manager Menu.

  @retval EFI_SUCCESS   The Boot Manager Menu is successfully returned.
  @retval EFI_NOT_FOUND The Boot Manager Menu is not found.
**/
EFI_STATUS
EFIAPI
EfiBootManagerGetBootManagerMenu (
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOption
  );

/**
  The function enumerates all the legacy boot options, creates them and registers them in the BootOrder variable.
**/
typedef
VOID
(EFIAPI *EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION) (
  VOID
  );

/**
  The function boots a legacy boot option.
**/
typedef
VOID
(EFIAPI *EFI_BOOT_MANAGER_LEGACY_BOOT) (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption
  );

/**
  The function registers the legacy boot support capabilities.

  @param RefreshLegacyBootOption The function pointer to create all the legacy boot options.
  @param LegacyBoot              The function pointer to boot the legacy boot option.
**/
VOID
EFIAPI
EfiBootManagerRegisterLegacyBootSupport (
  EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION   RefreshLegacyBootOption,
  EFI_BOOT_MANAGER_LEGACY_BOOT                  LegacyBoot
  );


//
// Boot Manager connect and disconnect library functions
//

/**
  This function will connect all the system driver to controller
  first, and then special connect the default console, this make
  sure all the system controller available and the platform default
  console connected.
**/
VOID
EFIAPI
EfiBootManagerConnectAll (
  VOID
  );

/**
  This function will create all handles associate with every device
  path node. If the handle associate with one device path node can not
  be created successfully, then still give one chance to do the dispatch,
  which load the missing drivers if possible.

  @param  DevicePathToConnect   The device path which will be connected, it CANNOT be
                                a multi-instance device path
  @param  MatchingHandle        Return the controller handle closest to the DevicePathToConnect

  @retval EFI_INVALID_PARAMETER DevicePathToConnect is NULL.
  @retval EFI_NOT_FOUND         Failed to create all handles associate with every device path node.
  @retval EFI_SUCCESS           Successful to create all handles associate with every device path node.

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect,
  OUT EFI_HANDLE                *MatchingHandle          OPTIONAL
  );

/**
  This function will disconnect all current system handles. 
  
  gBS->DisconnectController() is invoked for each handle exists in system handle buffer.
  If handle is a bus type handle, all childrens also are disconnected recursively by
  gBS->DisconnectController().
**/
VOID
EFIAPI
EfiBootManagerDisconnectAll (
  VOID
  );


//
// Boot Manager console library functions
//

typedef enum {
  ConIn,
  ConOut,
  ErrOut,
  ConInDev,
  ConOutDev,
  ErrOutDev,
  ConsoleTypeMax
} CONSOLE_TYPE;

/**
  This function will connect all the console devices base on the console
  device variable ConIn, ConOut and ErrOut.
**/
VOID
EFIAPI
EfiBootManagerConnectAllDefaultConsoles (
  VOID
  );

/**
  This function updates the console variable based on ConVarName. It can
  add or remove one specific console device path from the variable

  @param  ConsoleType              ConIn, ConOut, ErrOut, ConInDev, ConOutDev or ErrOutDev.
  @param  CustomizedConDevicePath  The console device path to be added to
                                   the console variable. Cannot be multi-instance.
  @param  ExclusiveDevicePath      The console device path to be removed
                                   from the console variable. Cannot be multi-instance.

  @retval EFI_UNSUPPORTED          The added device path is the same as a removed one.
  @retval EFI_SUCCESS              Successfully added or removed the device path from the
                                   console variable.

**/
EFI_STATUS
EFIAPI
EfiBootManagerUpdateConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  );

/**
  Connect the console device base on the variable ConVarName, if
  device path of the ConVarName is multi-instance device path, if
  anyone of the instances is connected success, then this function
  will return success.

  @param  ConsoleType              ConIn, ConOut or ErrOut.
  @param  NeedDispatch             Whether need to dispatch.

  @retval EFI_NOT_FOUND            There is not any console devices connected
                                   success
  @retval EFI_SUCCESS              Success connect any one instance of the console
                                   device path base on the variable ConVarName.

**/
EFI_STATUS
EFIAPI
EfiBootManagerConnectConsoleVariable (
  IN  CONSOLE_TYPE              ConsoleType,
  IN  BOOLEAN                   NeedDispatch
  );

/**
  Query all the children of VideoController and return the device paths of all the 
  children that support GraphicsOutput protocol.

  @param VideoController       PCI handle of video controller.

  @return  Device paths of all the children that support GraphicsOutput protocol.
**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiBootManagerGetGopDevicePath (
  IN  EFI_HANDLE               VideoController
  );

//
// Boot Manager device path library functions
//
/**
  Delete the instance in Multi that overlaps with Single. 
TODO: Add to UefiDevicePathLib

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @return This function removes the device path instances in Multi that overlap
          Single, and returns the resulting device path. If there is no
          remaining device path as a result, this function will return NULL.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
EfiBootManagerDelPartMatchInstance (
  IN     EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN     EFI_DEVICE_PATH_PROTOCOL  *Single
  );

/**
  This function compares a device path data structure to that of all the nodes of a
  second device path instance.
TODO: Add to UefiDevicePathLib

  @param  Multi                 A pointer to a multi-instance device path data
                                structure.
  @param  Single                A pointer to a single-instance device path data
                                structure.

  @retval TRUE                  If the Single device path is contained within a 
                                Multi device path.
  @retval FALSE                 The Single device path is not contained within a 
                                Multi device path.

**/
BOOLEAN
EFIAPI
EfiBootManagerMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single
  );

//
// Boot Manager misc library functions.
//

/**
  This function reads the EFI variable (VendorGuid/Name) and returns a dynamically allocated
  buffer and the size of the buffer. If it fails, return NULL.
TODO: Add to UefiLib

  @param  Name                  The string part of the EFI variable name.
  @param  VendorGuid            The GUID part of the EFI variable name.
  @param  VariableSize          Returns the size of the EFI variable that was read.

  @retval !NULL                 Dynamically allocated memory that contains a copy 
                                of the EFI variable. The caller is responsible to 
                                free the buffer.
  @retval NULL                  The variable was not found.
**/
VOID *
EFIAPI
EfiBootManagerGetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  );

/**

  This routine is called to see if there are any capsules we need to process.
  If the boot mode is not UPDATE, then we do nothing. Otherwise find the
  capsule HOBS and produce firmware volumes for them via the DXE service.
  Then call the dispatcher to dispatch drivers from them. Finally, check
  the status of the updates.

  This function should be called by BDS in case we need to do some
  sort of processing even if there is no capsule to process. We
  need to do this if an earlier update went away and we need to
  clear the capsule variable so on the next reset PEI does not see it and
  think there is a capsule available.

  @retval EFI_INVALID_PARAMETER   boot mode is not correct for an update
  @retval EFI_SUCCESS             There is no error when processing capsule

**/
EFI_STATUS
EFIAPI
EfiBootManagerProcessCapsules (
  VOID
  );

#endif
