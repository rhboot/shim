/** @file
  Misc BDS library function

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

#include "InternalBdsLib.h"

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
  )
{
  
  UINTN         OptionNumber;
  UINTN         Index;
  UINT16        *OptionOrder;
  UINTN         OptionOrderSize;
  UINT16        *BootNext;

  if (FreeOptionNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OptionOrder = EfiBootManagerGetVariableAndSize (
                  OrderVariableName,
                  &gEfiGlobalVariableGuid,
                  &OptionOrderSize
                  );

  BootNext = NULL;
  if (*OrderVariableName == L'B') {
    GetEfiGlobalVariable2 (L"BootNext", (VOID**)&BootNext, NULL);
  }

  for (OptionNumber = 0; 
       OptionNumber < OptionOrderSize / sizeof (UINT16)
                    + ((BootNext != NULL) ? 1 : 0); 
       OptionNumber++
       ) {
    //
    // Search in OptionOrder whether the OptionNumber exists
    //
    for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
      if (OptionNumber == OptionOrder[Index]) {
        break;
      }
    }

    //
    // We didn't find it in the ****Order array and it doesn't equal to BootNext 
    // Otherwise, OptionNumber equals to OptionOrderSize / sizeof (UINT16) + 1
    //
    if ((Index == OptionOrderSize / sizeof (UINT16)) && 
        ((BootNext == NULL) || (OptionNumber != *BootNext))
        ) {
      break;
    }
  }
  if (OptionOrder != NULL) {
    FreePool (OptionOrder);
  }

  if (BootNext != NULL) {
    FreePool (BootNext);
  }

  //
  // When BootOrder & BootNext conver all numbers in the range [0 ... 0xffff],
  //   OptionNumber equals to 0x10000 which is not valid.
  //
  ASSERT (OptionNumber <= 0x10000);
  if (OptionNumber == 0x10000) {
    return EFI_OUT_OF_RESOURCES;
  } else {
    *FreeOptionNumber = (UINT16) OptionNumber;
    return EFI_SUCCESS;
  }
}

/**
  Create the Boot#### or Driver#### variable from the load option.
  
  @param  LoadOption      Pointer to the load option.

  @retval EFI_SUCCESS     The variable was created.
  @retval Others          Error status returned by RT->SetVariable.
**/
EFI_STATUS
EFIAPI
EfiBootManagerLoadOptionToVariable (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION     *Option
  )
{
  UINTN                            VariableSize;
  UINT8                            *Variable;
  UINT8                            *Ptr;
  CHAR16                           OptionName[sizeof ("Driver####")];
  CHAR16                           *Description;
  CHAR16                           NullChar;

  if ((Option->OptionNumber == LoadOptionNumberUnassigned) ||
      (Option->FilePath == NULL) ||
      (Option->OptionType >= LoadOptionTypeMax)
     ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Convert NULL description to empty description
  //
  NullChar    = L'\0';
  Description = Option->Description;
  if (Description == NULL) {
    Description = &NullChar;
  }

  /*
  UINT32                      Attributes;
  UINT16                      FilePathListLength;
  CHAR16                      Description[];
  EFI_DEVICE_PATH_PROTOCOL    FilePathList[];
  UINT8                       OptionalData[];
TODO: FilePathList[] IS:
A packed array of UEFI device paths.  The first element of the 
array is a device path that describes the device and location of the 
Image for this load option.  The FilePathList[0] is specific 
to the device type.  Other device paths may optionally exist in the 
FilePathList, but their usage is OSV specific. Each element 
in the array is variable length, and ends at the device path end 
structure.
  */
  VariableSize = sizeof (Option->Attributes)
               + sizeof (UINT16)
               + StrSize (Description)
               + GetDevicePathSize (Option->FilePath)
               + Option->OptionalDataSize;

  Variable     = AllocatePool (VariableSize);
  ASSERT (Variable != NULL);
  
  Ptr             = Variable;
  *(UINT32 *) Ptr = Option->Attributes;
  Ptr            += sizeof (Option->Attributes);
  *(UINT16 *) Ptr = (UINT16) GetDevicePathSize (Option->FilePath);
  Ptr            += sizeof (UINT16);
  CopyMem (Ptr, Description, StrSize (Description));
  Ptr            += StrSize (Description);
  CopyMem (Ptr, Option->FilePath, GetDevicePathSize (Option->FilePath));
  Ptr            += GetDevicePathSize (Option->FilePath);
  CopyMem (Ptr, Option->OptionalData, Option->OptionalDataSize);

  UnicodeSPrint (
    OptionName,
    sizeof (OptionName),
    (Option->OptionType == LoadOptionTypeBoot) ? L"Boot%04x" : L"Driver%04x",
    Option->OptionNumber
    );

  return gRT->SetVariable (
                OptionName,
                &gEfiGlobalVariableGuid,
                EFI_VARIABLE_BOOTSERVICE_ACCESS
                | EFI_VARIABLE_RUNTIME_ACCESS
                | EFI_VARIABLE_NON_VOLATILE,
                VariableSize,
                Variable
                );
}

/**
  This function will register the new boot#### or driver#### option.
  After the boot#### or driver#### updated, the BootOrder or DriverOrder will also be updated.

  @param  Option            Pointer to load option to add.
  @param  Position          Position of the new load option to put in the ****Order variable.

  @retval EFI_SUCCESS           The boot#### or driver#### have been successfully registered.
  @retval EFI_INVALID_PARAMETER The option number exceeds 0xFFFF.
  @retval EFI_ALREADY_STARTED   The option number of Option is being used already.
                                Note: this API only adds new load option, no replacement support.
  @retval EFI_OUT_OF_RESOURCES  There is no free option number that can be used when the
                                option number specified in the Option is LoadOptionNumberUnassigned.
  @retval EFI_STATUS            Return the status of gRT->SetVariable ().

**/
EFI_STATUS
EFIAPI
EfiBootManagerAddLoadOptionVariable (
  IN EFI_BOOT_MANAGER_LOAD_OPTION *Option,
  IN UINTN                        Position
  )
{
  EFI_STATUS                      Status;
  UINT16                          OptionNumber;

  if (Option == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the free option number if the option number is unassigned
  //
  if (Option->OptionNumber == LoadOptionNumberUnassigned) {
    Status = GetFreeOptionNumber (
               Option->OptionType == LoadOptionTypeBoot ? L"BootOrder" : L"DriverOrder",
               &OptionNumber
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Option->OptionNumber = OptionNumber;
  }

  if (Option->OptionNumber >= LoadOptionNumberMax) {
    return EFI_INVALID_PARAMETER;
  }

  Status = AddOptionNumberToOrderVariable (
             Option->OptionType == LoadOptionTypeBoot ? L"BootOrder" : L"DriverOrder",
             (UINT16) Option->OptionNumber,
             Position
             );
  if (!EFI_ERROR (Status)) {
    //
    // Save the Boot#### or Driver#### variable
    //
    Status = EfiBootManagerLoadOptionToVariable (Option);
  }

  return Status;
}

EFI_STATUS
AddOptionNumberToOrderVariable (
  IN CHAR16               *OrderVariableName,
  IN UINT16               OptionNumber,
  IN UINTN                Position
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  UINT16                  *OptionOrder;
  UINT16                  *NewOptionOrder;
  UINTN                   OptionOrderSize;
  //
  // Update the option order variable
  //
  OptionOrder = EfiBootManagerGetVariableAndSize (
                  OrderVariableName,
                  &gEfiGlobalVariableGuid,
                  &OptionOrderSize
                  );

  Status = EFI_SUCCESS;
  for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
    if (OptionOrder[Index] == OptionNumber) {
      Status = EFI_ALREADY_STARTED;
      break;
    }
  }

  if (!EFI_ERROR (Status)) {
    Position       = MIN (Position, OptionOrderSize / sizeof (UINT16));

    NewOptionOrder = AllocatePool (OptionOrderSize + sizeof (UINT16));
    ASSERT (NewOptionOrder != NULL);
    if (OptionOrderSize != 0) {
      CopyMem (NewOptionOrder, OptionOrder, Position * sizeof (UINT16));
      CopyMem (&NewOptionOrder[Position + 1], &OptionOrder[Position], OptionOrderSize - Position * sizeof (UINT16));
    }
    NewOptionOrder[Position] = OptionNumber;

    Status = gRT->SetVariable (
                    OrderVariableName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    OptionOrderSize + sizeof (UINT16),
                    NewOptionOrder
                    );
    ASSERT_EFI_ERROR (Status);
    FreePool (NewOptionOrder);
  }

  if (OptionOrder != NULL) {
    FreePool (OptionOrder);
  }

  return Status;
}

/**
  Sort the load option. The DriverOrder or BootOrder will be re-created to 
  reflect the new order.

  @param OptionType        Load option type
  @param Comparator        The comparator
**/
VOID
EFIAPI
EfiBootManagerSortLoadOptionVariable (
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE        OptionType,
  EFI_BOOT_MANAGER_LOAD_OPTION_COMPARATOR  Comparator
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION   *LoadOption;
  EFI_BOOT_MANAGER_LOAD_OPTION   TempOption;
  UINTN                          LoadOptionCount;
  UINTN                          IndexI;
  UINTN                          IndexJ;
  UINT16                         *OptionOrder;
  UINTN                          Start;

  LoadOption = EfiBootManagerGetLoadOptions (&LoadOptionCount, OptionType);
  Start = 0;

  //
  // Always put the BootNext as the first
  //
  if ((LoadOptionCount > 0) && LoadOption[0].BootNext) {
    Start = 1;
  }

  //
  // Insertion sort algorithm
  //
  for (IndexI = Start + 1; IndexI < LoadOptionCount; IndexI++) {

    //
    // for IndexI, find a proper position in [Start .. IndexI)
    //
    for (IndexJ = Start; IndexJ < IndexI; IndexJ++) {
      if (Comparator (&LoadOption[IndexI], &LoadOption[IndexJ])) {
        CopyMem (&TempOption, &LoadOption[IndexI], sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
        CopyMem (&LoadOption[IndexJ + 1], &LoadOption[IndexJ], (IndexI - IndexJ) * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
        CopyMem (&LoadOption[IndexJ], &TempOption, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
        break;
      }
    }
  }

  //
  // Create new ****Order variable
  //
  OptionOrder = AllocatePool ((LoadOptionCount - Start) * sizeof (UINT16));
  ASSERT (OptionOrder != NULL);
  for (IndexI = Start; IndexI < LoadOptionCount; IndexI++) {
    OptionOrder[IndexI - Start] = (UINT16) LoadOption[IndexI].OptionNumber;
  }
  
  gRT->SetVariable (
         OptionType == LoadOptionTypeBoot ? L"BootOrder" : L"DriverOrder",
         &gEfiGlobalVariableGuid,
         EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
         (LoadOptionCount - Start)* sizeof (UINT16),
         OptionOrder
         );

  FreePool (OptionOrder);
  EfiBootManagerFreeLoadOptions (LoadOption, LoadOptionCount);
}

/**
  Initialize the load option.

  @param OptionNumber  Point to option number and NULL requests to auto-generate the option number
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
  IN  UINT8                             *OptionalData,   OPTIONAL
  IN  UINT32                            OptionalDataSize
  )
{
  if ((Option == NULL) || (Description == NULL) || (FilePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((OptionalData != NULL) && (OptionalDataSize == 0)) ||
      ((OptionalData == NULL) && (OptionalDataSize != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Option, sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
  Option->OptionNumber       = OptionNumber;
  Option->OptionType         = OptionType;
  Option->Attributes         = Attributes;
  Option->Description        = AllocateCopyPool (StrSize (Description), Description);
  Option->FilePath           = DuplicateDevicePath (FilePath);
  if (OptionalData != NULL) {
    Option->OptionalData     = AllocateCopyPool (OptionalDataSize, OptionalData);
    Option->OptionalDataSize = OptionalDataSize;
  }

  return EFI_SUCCESS;
}

/**
  Return the index of the load option in the load option array.

  The function compares the OptionType, Attributes, Description, FilePath, OptionalData.
**/
INTN
EFIAPI
EfiBootManagerFindLoadOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Array,
  IN UINTN                              Count
  )
{
  UINTN                             Index;

  for (Index = 0; Index < Count; Index++) {
    if ((Key->OptionType == Array[Index].OptionType) &&
        (Key->Attributes == Array[Index].Attributes) &&
        (StrCmp (Key->Description, Array[Index].Description) == 0) &&
        (CompareMem (Key->FilePath, Array[Index].FilePath, GetDevicePathSize (Key->FilePath)) == 0) &&
        (Key->OptionalDataSize == Array[Index].OptionalDataSize) &&
        (CompareMem (Key->OptionalData, Array[Index].OptionalData, Key->OptionalDataSize) == 0)) {
      return (INTN) Index;
    }
  }

  return -1;
}

EFI_STATUS
DeleteOptionVariable (
  IN CHAR16                            *OptionOrderVariable,
  IN UINT16                            OptionNumber
  )
{
  UINT16           *OptionOrder;
  UINTN            OptionOrderSize;
  EFI_STATUS       Status;
  UINTN            Index;

  Status      = EFI_NOT_FOUND;
  OptionOrder = EfiBootManagerGetVariableAndSize (
                  OptionOrderVariable,
                  &gEfiGlobalVariableGuid,
                  &OptionOrderSize
                  );
  for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
    if (OptionOrder[Index] == OptionNumber) {
      OptionOrderSize -= sizeof (UINT16);
      CopyMem (&OptionOrder[Index], &OptionOrder[Index + 1], OptionOrderSize - Index * sizeof (UINT16));
      Status = gRT->SetVariable (
                      OptionOrderVariable,
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      OptionOrderSize,
                      OptionOrder
                      );
      ASSERT_EFI_ERROR (Status);
      break;
    }
  }
  if (OptionOrder != NULL) {
    FreePool (OptionOrder);
  }

  return Status;
}

/**
  Update the BootOrder or DriverOrder according to the OptionType to delete OptionNumber .
  
  @param  OptionNumber        Indicate the option number of load option
  @param  OptionType          Indicate the type of load option

  @retval EFI_INVALID_PARAMETER OptionType or OptionNumber is invalid.
  @retval EFI_NOT_FOUND         The load option cannot be found
  @retval EFI_SUCCESS           The load option was deleted
**/
EFI_STATUS
EFIAPI
EfiBootManagerDeleteLoadOptionVariable (
  IN UINTN                              OptionNumber,
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  OptionType
  )
{
  if ((OptionType >= LoadOptionTypeMax) || (OptionNumber >= LoadOptionNumberMax)) {
    return EFI_INVALID_PARAMETER;
  }

  return DeleteOptionVariable (
           OptionType == LoadOptionTypeBoot ? L"BootOrder" : L"DriverOrder",
           (UINT16) OptionNumber
           );
}

/**
  Convert a single character to number.
  It assumes the input Char is in the scope of L'0' ~ L'9' and L'A' ~ L'F'
**/
UINTN
CharToUint (
  IN CHAR16                           Char
  )
{
  if ((Char >= L'0') && (Char <= L'9')) {
    return (UINTN) (Char - L'0');
  }

  if ((Char >= L'A') && (Char <= L'F')) {
    return (UINTN) (Char - L'A' + 0xA);
  }

  ASSERT (FALSE);
  return 0;
}

/**
  Returns the size of a device path in bytes.

  This function returns the size, in bytes, of the device path data structure 
  specified by DevicePath including the end of device path node. If DevicePath 
  is NULL, then 0 is returned. If the length of the device path is bigger than
  MaxSize, also return 0 to indicate this is an invalidate device path.

  @param  DevicePath         A pointer to a device path data structure.
  @param  MaxSize            Max valid device path size. If big than this size, 
                             return error.
  
  @retval 0                  An invalid device path.
  @retval Others             The size of a device path in bytes.

**/
UINTN
GetDevicePathSizeEx (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINTN                           MaxSize
  )
{
  UINTN  Size;
  UINTN  NodeSize;

  if (DevicePath == NULL) {
    return 0;
  }

  //
  // Search for the end of the device path structure
  //
  Size = 0;
  while (!IsDevicePathEnd (DevicePath)) {
    NodeSize = DevicePathNodeLength (DevicePath);
    if (NodeSize == 0) {
      return 0;
    }
    Size += NodeSize;
    if (Size > MaxSize) {
      return 0;
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }
  Size += DevicePathNodeLength (DevicePath);
  if (Size > MaxSize) {
    return 0;
  }

  return Size;
}

/**
  Returns the length of a Null-terminated Unicode string. If the length is 
  bigger than MaxStringLen, return length 0 to indicate that this is an 
  invalidate string.

  This function returns the number of Unicode characters in the Null-terminated
  Unicode string specified by String. 

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().

  @param  String           A pointer to a Null-terminated Unicode string.
  @param  MaxStringLen     Max string len in this string.

  @retval 0                An invalid string.
  @retval Others           The length of String.

**/
UINTN
StrSizeEx (
  IN      CONST CHAR16              *String,
  IN      UINTN                     MaxStringLen
  )
{
  UINTN                             Length;

  ASSERT (String != NULL && MaxStringLen != 0);
  ASSERT (((UINTN) String & BIT0) == 0);

  for (Length = 0; *String != L'\0' && MaxStringLen != Length; String++, Length+=2);

  if (*String != L'\0' && MaxStringLen == Length) {
    return 0;
  }

  return Length + 2;
}

/**
  Validate the EFI Boot#### variable (VendorGuid/Name)

  @param  Variable              Boot#### variable data.
  @param  VariableSize          Returns the size of the EFI variable that was read

  @retval TRUE                  The variable data is correct.
  @retval FALSE                 The variable data is corrupted.

**/
BOOLEAN 
ValidateOption (
  UINT8                     *Variable,
  UINTN                     VariableSize
  )
{
  UINT16                    FilePathSize;
  UINT8                     *TempPtr;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     TempSize;

  if (VariableSize <= sizeof (UINT16) + sizeof (UINT32)) {
    return FALSE;
  }

  //
  // Skip the option attribute
  //
  TempPtr    = Variable;
  TempPtr   += sizeof (UINT32);

  //
  // Get the option's device path size
  //
  FilePathSize  = *(UINT16 *) TempPtr;
  TempPtr      += sizeof (UINT16);

  //
  // Get the option's description string size
  //
  TempSize = StrSizeEx ((CHAR16 *) TempPtr, VariableSize - sizeof (UINT16) - sizeof (UINT32));
  TempPtr += TempSize;

  //
  // Get the option's device path
  //
  DevicePath =  (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;
  TempPtr   += FilePathSize;

  //
  // Validation boot option variable.
  //
  if ((FilePathSize == 0) || (TempSize == 0)) {
    return FALSE;
  }

  if (TempSize + FilePathSize + sizeof (UINT16) + sizeof (UINT32) > VariableSize) {
    return FALSE;
  }

  return (BOOLEAN) (GetDevicePathSizeEx (DevicePath, FilePathSize) != 0);
}

/**
  Build the Boot#### or Driver#### option from the VariableName.

  @param  VariableName          EFI Variable name indicate if it is Boot#### or
                                Driver####

  @retval EFI_SUCCESS     Get the option just been created
  @retval EFI_NOT_FOUND   Failed to get the new option

**/
EFI_STATUS
EFIAPI
EfiBootManagerVariableToLoadOption (
  IN  CHAR16                          *VariableName,
  IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *Option  
  )
{
  EFI_STATUS                         Status;
  UINT32                             Attribute;
  UINT16                             FilePathSize;
  UINT8                              *Variable;
  UINT8                              *TempPtr;
  UINTN                              VariableSize;
  EFI_DEVICE_PATH_PROTOCOL           *FilePath;
  UINT8                              *OptionalData;
  UINT32                             OptionalDataSize;
  CHAR16                             *Description;
  UINT8                              NumOff;
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  OptionType;
  UINT16                             OptionNumber;

  if ((VariableName == NULL) || (Option == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read the variable
  //
  Variable = EfiBootManagerGetVariableAndSize (
              VariableName,
              &gEfiGlobalVariableGuid,
              &VariableSize
              );
  if (Variable == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Validate Boot#### variable data.
  //
  if (!ValidateOption(Variable, VariableSize)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Notes: careful defined the variable of Boot#### or
  // Driver####, consider use some macro to abstract the code
  //
  //
  // Get the option attribute
  //
  TempPtr   =  Variable;
  Attribute =  *(UINT32 *) Variable;
  TempPtr   += sizeof (UINT32);

  //
  // Get the option's device path size
  //
  FilePathSize =  *(UINT16 *) TempPtr;
  TempPtr     += sizeof (UINT16);

  //
  // Get the option's description string
  //
  Description  = (CHAR16 *) TempPtr;

  //
  // Get the option's description string size
  //
  TempPtr     += StrSize ((CHAR16 *) TempPtr);

  //
  // Get the option's device path
  //
  FilePath     =  (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;
  TempPtr     += FilePathSize;

  OptionalDataSize = (UINT32) (VariableSize - (UINTN) (TempPtr - Variable));
  if (OptionalDataSize == 0) {
    OptionalData = NULL;
  } else {
    OptionalData = TempPtr;
  }

  if (*VariableName == L'B') {
    OptionType = LoadOptionTypeBoot;
    NumOff = (UINT8) (sizeof (L"Boot") / sizeof (CHAR16) - 1);
  } else {
    OptionType = LoadOptionTypeDriver;
    NumOff = (UINT8) (sizeof (L"Driver") / sizeof (CHAR16) - 1);
  }
  
  //
  // Get the value from VariableName Unicode string
  // since the ISO standard assumes ASCII equivalent abbreviations, we can be safe in converting this
  // Unicode stream to ASCII without any loss in meaning.
  //  
  OptionNumber = (UINT16) (CharToUint (VariableName[NumOff+0]) * 0x1000) 
               + (UINT16) (CharToUint (VariableName[NumOff+1]) * 0x100)
               + (UINT16) (CharToUint (VariableName[NumOff+2]) * 0x10)
               + (UINT16) (CharToUint (VariableName[NumOff+3]) * 0x1);

  Status = EfiBootManagerInitializeLoadOption (
             Option,
             OptionNumber,
             OptionType,
             Attribute,
             Description,
             FilePath,
             OptionalData,
             OptionalDataSize
             );
  ASSERT_EFI_ERROR (Status);
  
  FreePool (Variable);
  return Status;
}

/**
  Process BootOrder, or DriverOrder variables, by calling
  BdsLibVariableToOption () for each UINT16 in the variables.

  @param  BdsCommonOptionList   The header of the option list base on variable
                                VariableName
  @param  VariableName          EFI Variable name indicate the BootOrder or
                                DriverOrder

  @retval EFI_SUCCESS           Success create the boot option or driver option
                                list
  @retval EFI_OUT_OF_RESOURCES  Failed to get the boot option or driver option list

**/
EFI_BOOT_MANAGER_LOAD_OPTION *
EFIAPI
EfiBootManagerGetLoadOptions (
  OUT UINTN                             *OptionCount,
  IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  LoadOptionType
  )
{
  EFI_STATUS                   Status;
  UINT16                       *BootNext;
  UINT16                       *OptionOrder;
  UINTN                        OptionOrderSize;
  UINTN                        Index;
  UINTN                        OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION *Option;
  CHAR16                       OptionName[sizeof ("Driver####")];
  UINT16                       OptionNumber;

  *OptionCount = 0;
  BootNext     = NULL;

  if (LoadOptionType == LoadOptionTypeBoot) {
    GetEfiGlobalVariable2 (L"BootNext", (VOID**)&BootNext, NULL);
  }

  //
  // Read the BootOrder, or DriverOrder variable.
  //
  OptionOrder = EfiBootManagerGetVariableAndSize (
                  (LoadOptionType == LoadOptionTypeBoot) ? L"BootOrder" : L"DriverOrder",
                  &gEfiGlobalVariableGuid,
                  &OptionOrderSize
                  );
  if (OptionOrder == NULL && BootNext == NULL) {
    return NULL;
  }

  *OptionCount = ((BootNext != NULL) ? 1 : 0) + OptionOrderSize / sizeof (UINT16);

  Option = AllocatePool (*OptionCount * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION));
  ASSERT (Option != NULL);

  OptionIndex = 0;
  for (Index = 0; Index < *OptionCount; Index++) {
    if (BootNext == NULL) {
      OptionNumber = OptionOrder[Index];
    } else {
      OptionNumber = ((Index == 0) ? *BootNext : OptionOrder[Index - 1]);
    }
    if (LoadOptionType == LoadOptionTypeBoot) {
      UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", OptionNumber);
    } else {
      UnicodeSPrint (OptionName, sizeof (OptionName), L"Driver%04x", OptionNumber);
    }

    Status = EfiBootManagerVariableToLoadOption (OptionName, &Option[OptionIndex]);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "[Bds] %s doesn't exist - Update ****Order variable to remove the reference!!", OptionName));
      EfiBootManagerDeleteLoadOptionVariable (OptionNumber, LoadOptionTypeBoot);
    } else {
      ASSERT (Option[OptionIndex].OptionNumber == OptionNumber);

      if ((BootNext != NULL) && (*BootNext == Option[OptionIndex].OptionNumber)) {
        Option[OptionIndex].BootNext = TRUE;
      }
      OptionIndex++;
    }
  }

  if (OptionOrder != NULL) {
    FreePool (OptionOrder);
  }
  if (BootNext != NULL) {
    FreePool (BootNext);
  }

  if (OptionIndex < *OptionCount) {
    Option = ReallocatePool (
               *OptionCount * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION),
               OptionIndex * sizeof (EFI_BOOT_MANAGER_LOAD_OPTION),
               Option
               );
    *OptionCount = OptionIndex;
  }

  return Option;
}

/**
  Free an EFI_BOOT_MANGER_LOAD_OPTION entry that was allocate by the library.

  @param  Option   Pointer to boot option to Free.

  @return EFI_SUCCESS   BootOption was freed 
  @return EFI_INVALID_PARAMETER BootOption == NULL 

**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeLoadOption (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOption
  )
{
  if (LoadOption == NULL) {
    return EFI_NOT_FOUND;
  }

  if (LoadOption->Description != NULL) {
    FreePool (LoadOption->Description);
  }
  if (LoadOption->FilePath != NULL) {
    FreePool (LoadOption->FilePath);
  }
  if (LoadOption->OptionalData != NULL) {
    FreePool (LoadOption->OptionalData);
  }

  return EFI_SUCCESS;
}

/**
  Free an EFI_BOOT_MANGER_LOAD_OPTION array that was allocated by 
  EfiBootManagerGetLoadOptions().

  @param  BootOption       Pointer to boot option array to free.
  @param  BootOptionCount  Number of array entries in BootOption

  @return EFI_SUCCESS   BootOption was freed 
  @return EFI_NOT_FOUND BootOption == NULL 

**/
EFI_STATUS
EFIAPI
EfiBootManagerFreeLoadOptions (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION  *Option,
  IN  UINTN                         OptionCount
  )
{
  UINTN   Index;

  if (Option == NULL) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0;Index < OptionCount; Index++) {
    EfiBootManagerFreeLoadOption (&Option[Index]);
  }

  FreePool (Option);

  return EFI_SUCCESS;
}
