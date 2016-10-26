/** @file

  CPU Configuration Library.

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

  Module Name:  CpuConfig.c

**/


#include "CpuConfig.h"

CPU_CONFIG_CONTEXT_BUFFER      *mCpuConfigLibConfigConextBuffer;

/**
  The constructor function of CPU Configuration Library.
  
  The constructor function caches the value of PCD entry PcdCpuConfigContextBuffer.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
CpuConfigLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Get the pointer to CPU Configuration Context Buffer from PcdCpuConfigContextBuffer.
  // This PCD entry is set by CPU driver.
  //
  mCpuConfigLibConfigConextBuffer = (CPU_CONFIG_CONTEXT_BUFFER *) (UINTN) PcdGet64 (PcdCpuConfigContextBuffer);

  return EFI_SUCCESS;
}

/**
  Set feature capability and related attribute.
  
  This function sets the feature capability and its attribute.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureID       The ID of the feature.
  @param  Attribute       Feature-specific data.

**/
VOID
EFIAPI
SetProcessorFeatureCapability (	
  IN  UINTN               ProcessorNumber,
  IN  CPU_FEATURE_ID      FeatureID,
  IN  VOID                *Attribute
  )
{
  CPU_COLLECTED_DATA          *CpuCollectedData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuCollectedData   = &mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber];

  //
  // Set the feature capability of the specified processor as TRUE,
  // and set feature-specified attribute for the feature.
  //
  CpuCollectedData->FeatureData[FeatureID].Capability = TRUE;
  CpuCollectedData->FeatureData[FeatureID].Attribute  = Attribute;
}

/**
  Clears feature capability and related attribute.
  
  This function clears the feature capability and its attribute.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureID       The ID of the feature.

**/
VOID
EFIAPI
ClearProcessorFeatureCapability (	
  IN  UINTN               ProcessorNumber,
  IN  CPU_FEATURE_ID      FeatureID
  )
{
  CPU_COLLECTED_DATA          *CpuCollectedData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuCollectedData   = &mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber];

  //
  // Set the feature capability of the specified processor as FALSE,
  // and clears feature-specified attribute for the feature.
  //
  CpuCollectedData->FeatureData[FeatureID].Capability = FALSE;
  CpuCollectedData->FeatureData[FeatureID].Attribute  = NULL;
}

/**
  Get feature capability and related attribute.
  
  This function gets the feature capability and its attribute.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureID       The ID of the feature.
  @param  Attribute       Pointer to the output feature-specific data.

  @retval TRUE            The feature is supported by the processor
  @retval FALSE           The feature is not supported by the processor

**/
BOOLEAN
EFIAPI
GetProcessorFeatureCapability (	
  IN  UINTN               ProcessorNumber,
  IN  CPU_FEATURE_ID      FeatureID,
  OUT VOID                **Attribute  OPTIONAL
  )
{
  CPU_COLLECTED_DATA          *CpuCollectedData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuCollectedData = &mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber];

  //
  // If Attribute is not NULL, set feature-specified attribute to it.
  //
  if (Attribute != NULL) {
    *Attribute = CpuCollectedData->FeatureData[FeatureID].Attribute;
  }

  return CpuCollectedData->FeatureData[FeatureID].Capability;
}

/**
  Get the number of CPUID leafs of various types.
  
  This function get the number of CPUID leafs of various types.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  CpuidType         The type of the cpu id.

  @return Maximal index of CPUID instruction for basic leafs.

**/
UINTN
EFIAPI
GetNumberOfCpuidLeafs (
  IN  UINTN               ProcessorNumber,
  IN  CPUID_TYPE          CpuidType
  )
{
  UINTN                NumberOfLeafs;
  CPU_CPUID_DATA       *CpuidData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuidData = &(mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber].CpuidData);

  switch (CpuidType) {
  case BasicCpuidLeaf:
    NumberOfLeafs = CpuidData->NumberOfBasicCpuidLeafs;
    break;
  case ExtendedCpuidLeaf:
    NumberOfLeafs = CpuidData->NumberOfExtendedCpuidLeafs;
    break;
  case CacheAndTlbCpuidLeafs:
    NumberOfLeafs = CpuidData->NumberOfCacheAndTlbCpuidLeafs;
    break;
  case DeterministicCacheParametersCpuidLeafs:
    NumberOfLeafs = CpuidData->NumberOfDeterministicCacheParametersCpuidLeafs;
    break;
  case ExtendedTopologyEnumerationCpuidLeafs:
    NumberOfLeafs = CpuidData->NumberOfExtendedTopologyEnumerationLeafs;
    break;
  default:
    NumberOfLeafs = 0;
    break;
  }

  return NumberOfLeafs;
}

/**
  Get the pointer to specified CPUID leaf.
  
  This function gets the pointer to specified CPUID leaf.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  Index             Index of the CPUID leaf.

  @return Pointer to specified CPUID leaf

**/
EFI_CPUID_REGISTER*
EFIAPI
GetProcessorCpuid (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               Index
)
{
  CPU_CPUID_DATA       *CpuidData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuidData = &(mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber].CpuidData);

  //
  // If Index specifies basic CPUID leafs, then locate basic leaf to return.
  //
  if (Index < CpuidData->NumberOfBasicCpuidLeafs) {
    return (&CpuidData->CpuIdLeaf[Index]);
  }

  //
  // If Index specifies extended CPUID leafs, then locate extended leaf to return.
  //
  if (Index >= 0x80000000 && Index < 0x80000000 + CpuidData->NumberOfExtendedCpuidLeafs) {
    return (&CpuidData->CpuIdLeaf[Index - 0x80000000 + CpuidData->NumberOfBasicCpuidLeafs]);
  }

  //
  // If Index specifies invalid CPUID index, then return NULL.
  //
  return NULL;
}

/**
  Get the pointer to specified CPUID leaf of cache and TLB parameters.
  
  This function gets the pointer to specified CPUID leaf of cache and TLB parameters.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  Index             Index of the CPUID leaf.

  @return Pointer to specified CPUID leaf.

**/
EFI_CPUID_REGISTER*
EFIAPI
GetCacheAndTlbCpuidLeaf (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               Index
)
{
  UINTN                StartIndex;
  CPU_CPUID_DATA       *CpuidData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuidData = &(mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber].CpuidData);

  //
  // The first leaf resides in basic leaf 2
  //
  if (Index == 0) {
    return (&CpuidData->CpuIdLeaf[2]);
  }

  //
  // If index is greater than zero, calculate the start index of deterministic cache CPUID leafs.
  //
  StartIndex = CpuidData->NumberOfBasicCpuidLeafs + CpuidData->NumberOfExtendedCpuidLeafs;

  return (&CpuidData->CpuIdLeaf[Index + StartIndex - 1]);
}

/**
  Get the pointer to specified CPUID leaf of deterministic cache parameters.
  
  This function gets the pointer to specified CPUID leaf of deterministic cache parameters.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  Index             Index of the CPUID leaf.

  @return Pointer to specified CPUID leaf.

**/
EFI_CPUID_REGISTER*
EFIAPI
GetDeterministicCacheParametersCpuidLeaf (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               Index
)
{
  UINTN                StartIndex;
  CPU_CPUID_DATA       *CpuidData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuidData = &(mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber].CpuidData);

  //
  // Calculate the start index of deterministic cache CPUID leafs.
  //
  StartIndex =  CpuidData->NumberOfBasicCpuidLeafs;
  StartIndex += CpuidData->NumberOfExtendedCpuidLeafs;
  StartIndex += CpuidData->NumberOfCacheAndTlbCpuidLeafs - 1;

  return (&CpuidData->CpuIdLeaf[Index + StartIndex]);
}

/**
  Get the pointer to specified CPUID leaf of Extended Topology Enumeration.
  
  This function gets the pointer to specified CPUID leaf of Extended Topology Enumeration.

  @param  ProcessorNumber   Handle number of specified logical processor.
  @param  Index             Index of the CPUID leaf.

  @return Pointer to specified CPUID leaf.

**/
EFI_CPUID_REGISTER*
EFIAPI
GetExtendedTopologyEnumerationCpuidLeafs (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               Index
  )
{
  UINTN                StartIndex;
  CPU_CPUID_DATA       *CpuidData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuidData = &(mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber].CpuidData);

  //
  // Calculate the start index of Extended Topology Enumeration CPUID leafs.
  //
  StartIndex =  CpuidData->NumberOfBasicCpuidLeafs;
  StartIndex += CpuidData->NumberOfExtendedCpuidLeafs;
  StartIndex += CpuidData->NumberOfCacheAndTlbCpuidLeafs - 1;
  StartIndex += CpuidData->NumberOfDeterministicCacheParametersCpuidLeafs;

  return (&CpuidData->CpuIdLeaf[Index + StartIndex]);
}

/**
  Get the version information of specified logical processor.
  
  This function gets the version information of specified logical processor,
  including family ID, model ID, stepping ID and processor type.

  @param  ProcessorNumber   Handle number of specified logical processor
  @param  DisplayedFamily   Pointer to family ID for output
  @param  DisplayedModel    Pointer to model ID for output
  @param  SteppingId        Pointer to stepping ID for output
  @param  ProcessorType     Pointer to processor type for output

**/
VOID
EFIAPI
GetProcessorVersionInfo (
  IN  UINTN               ProcessorNumber,
  OUT UINT32              *DisplayedFamily OPTIONAL,
  OUT UINT32              *DisplayedModel  OPTIONAL,       
  OUT UINT32              *SteppingId      OPTIONAL,
  OUT UINT32              *ProcessorType   OPTIONAL
  )
{
  EFI_CPUID_REGISTER      *VersionInfo;
  UINT32                  RegEax;
  UINT32                  FamilyId;
  UINT32                  ExtendedFamilyId;
  UINT32                  ExtendedModelId;

  //
  // Get CPUID(1).EAX
  //
  VersionInfo = GetProcessorCpuid (ProcessorNumber, EFI_CPUID_VERSION_INFO);
  ASSERT (VersionInfo != NULL);
  RegEax      = VersionInfo->RegEax;

  //
  // Processor Type is CPUID(1).EAX[12:13]
  //
  if (ProcessorType != NULL) {
    *ProcessorType = BitFieldRead32 (RegEax, 12, 13);
  }

  //
  // Stepping ID is CPUID(1).EAX[0:3]
  //
  if (SteppingId != NULL) {
    *SteppingId = BitFieldRead32 (RegEax, 0, 3);
  }

  //
  // The Extended Family ID needs to be examined only when the Family ID is 0FH.
  // If Family ID is 0FH, Displayed Family ID = Family ID + Extended Family ID.
  // Otherwise, Displayed Family ID is Family ID
  //
  FamilyId = BitFieldRead32 (RegEax, 8, 11);
  if (DisplayedFamily != NULL) {
    *DisplayedFamily = FamilyId;
    if (FamilyId == 0x0f) {
      ExtendedFamilyId = BitFieldRead32 (RegEax, 20, 27);
      *DisplayedFamily += ExtendedFamilyId;
    }
  }

  //
  // The Extended Model ID needs to be examined only when the Family ID is 06H or 0FH.
  // If Family ID is 06H or 0FH, Displayed Model ID = Model ID + (Extended Model ID << 4).
  // Otherwise, Displayed Model ID is Model ID.
  //
  if (DisplayedModel != NULL) {
    *DisplayedModel = BitFieldRead32 (RegEax, 4, 7);
    if (FamilyId == 0x06 || FamilyId == 0x0f) {
      ExtendedModelId = BitFieldRead32 (RegEax, 16, 19);
      *DisplayedModel += (ExtendedModelId << 4);
    }
  }
}

/**
  Get initial local APIC ID of specified logical processor
  
  This function gets initial local APIC ID of specified logical processor.

  @param  ProcessorNumber Handle number of specified logical processor

  @return Initial local APIC ID of specified logical processor

**/
UINT32
EFIAPI
GetInitialLocalApicId (
  UINTN    ProcessorNumber
  )
{
  CPU_COLLECTED_DATA          *CpuCollectedData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuCollectedData   = &mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber];

  return (CpuCollectedData->CpuidData.CpuIdLeaf[1].RegEbx >> 24) & 0xff;
}

/**
  Get the location of specified processor.
  
  This function gets the location of specified processor, including
  package number, core number within package, thread number within core.

  @param  ProcessorNumber Handle number of specified logical processor.
  @param  PackageNumber   Pointer to the output package number.
  @param  CoreNumber      Pointer to the output core number.
  @param  ThreadNumber    Pointer to the output thread number.

**/
VOID
EFIAPI
GetProcessorLocation (
  IN    UINTN     ProcessorNumber,
  OUT   UINT32    *PackageNumber   OPTIONAL,
  OUT   UINT32    *CoreNumber      OPTIONAL,
  OUT   UINT32    *ThreadNumber    OPTIONAL
)
{
  CPU_COLLECTED_DATA          *CpuCollectedData;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  CpuCollectedData   = &mCpuConfigLibConfigConextBuffer->CollectedDataBuffer[ProcessorNumber];

  //
  // If PackageNumber is not NULL, set the package number of the specified processor to it.
  //
  if (PackageNumber != NULL) {
    *PackageNumber = CpuCollectedData->ProcessorLocation.Package;
  }

  //
  // If CoreNumber is not NULL, set the core number within package to it.
  //
  if (CoreNumber != NULL) {
    *CoreNumber    = CpuCollectedData->ProcessorLocation.Core;
  }

  //
  // If ThreadNumber is not NULL, set the thread number within core to it.
  //
  if (ThreadNumber != NULL) {
    *ThreadNumber  = CpuCollectedData->ProcessorLocation.Thread;
  }
}

/**
  Get the Feature entry at specified position in a feature list.
  
  This function gets the Feature entry at specified position in a feature list.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureIndex    The index of the node in feature list.
  @param  Attribute       Pointer to output feature-specific attribute

  @return Feature ID of specified feature. CpuFeatureMaximum means not found

**/
CPU_FEATURE_ID
EFIAPI
GetProcessorFeatureEntry (
  IN        UINTN       ProcessorNumber,
  IN        UINTN       FeatureIndex,
  OUT       VOID        **Attribute  OPTIONAL
  )
{
  CPU_FEATURE_ENTRY           *FeatureEntry;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);

  if (Attribute != NULL) {
    *Attribute = NULL;
  }

  //
  // Search for specified entry. If not found, return CpuFeatureMaximum.
  //
  FeatureEntry = SearchFeatureEntry (ProcessorNumber, FeatureIndex);
  if (FeatureEntry == NULL) {
    return CpuFeatureMaximum;
  }

  //
  // If Attribute is not NULL, set the feature-specified attribute to it.
  //
  if (Attribute != NULL) {
    *Attribute = FeatureEntry->Attribute;
  }

  return (FeatureEntry->FeatureID);
}

/**
  Append a feature entry at the end of a feature list.
  
  This function appends a feature entry at the end of a feature list.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureID       ID of the specified feature.
  @param  Attribute       Feature-specific attribute.

  @retval EFI_SUCCESS     This function always return EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
AppendProcessorFeatureIntoList (
  IN  UINTN               ProcessorNumber,
  IN  CPU_FEATURE_ID      FeatureID,
  IN  VOID                *Attribute
  )
{
  LIST_ENTRY                  *List;
  CPU_FEATURE_ENTRY           *NewEntry;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);
  List = &mCpuConfigLibConfigConextBuffer->FeatureLinkListEntry[ProcessorNumber];

  //
  // Create a new feature node with given Feature ID and feature-specific attribute
  //
  NewEntry = CreateFeatureEntry (FeatureID, Attribute);

  //
  // Append the new node at the end of the list
  //
  InsertTailList (List, &(NewEntry->Link));

  return EFI_SUCCESS;
}

/**
  Delete a feature entry in a feature list.
  
  This function deletes a feature entry in a feature list.

  @param  ProcessorNumber        Handle number of specified logical processor
  @param  FeatureIndex           The index of the node in feature list.

  @retval EFI_SUCCESS            The feature node successfully removed.
  @retval EFI_INVALID_PARAMETER  FeatureIndex surpasses the length of list.

**/
EFI_STATUS
EFIAPI
DeleteProcessorFeatureFromList (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               FeatureIndex
  )
{
  CPU_FEATURE_ENTRY           *FeatureEntry;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);

  //
  // Search for specified entry. If SearchFeatureEntry() returns NULL, then
  // it means FeatureIndex surpasses the length of list. 
  // Return EFI_INVALID_PARAMETER.
  //
  FeatureEntry = SearchFeatureEntry (ProcessorNumber, FeatureIndex);
  if (FeatureEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Remove the entry from list, and free its memory region.
  //
  RemoveEntryList (&(FeatureEntry->Link));
  FreePool (FeatureEntry);

  return EFI_SUCCESS;
}

/**
  Insert a feature entry into a feature list.
  
  This function insert a feature entry into a feature list before a node specified by Feature Index.

  @param  ProcessorNumber        Handle number of specified logical processor
  @param  FeatureIndex           The index of the new node in feature list.
  @param  FeatureID              ID of the specified feature.
  @param  Attribute              Feature-specific attribute.

  @retval EFI_SUCCESS            The feature node successfully inserted.
  @retval EFI_INVALID_PARAMETER  FeatureIndex surpasses the length of list.

**/
EFI_STATUS
EFIAPI
InsertProcessorFeatureIntoList (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               FeatureIndex,
  IN  CPU_FEATURE_ID      FeatureID,
  IN  VOID                *Attribute
  )
{
  CPU_FEATURE_ENTRY           *FeatureEntry;
  CPU_FEATURE_ENTRY           *NewEntry;

  //
  // Search for specified entry. If SearchFeatureEntry() returns NULL, then
  // it means FeatureIndex surpasses the length of list. 
  // Return EFI_INVALID_PARAMETER.
  //
  FeatureEntry = SearchFeatureEntry (ProcessorNumber, FeatureIndex);
  if (FeatureEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Create a new feature node with given Feature ID and feature-specific attribute
  //
  NewEntry = CreateFeatureEntry (FeatureID, Attribute);

  //
  // Insert the new node before specified feature entry.
  //
  InsertHeadList (&(FeatureEntry->Link), &(NewEntry->Link));

  return EFI_SUCCESS;
}

/**
  Add an entry in specified register table.
  
  This function adds an entry in specified register table, with given register type,
  register index, bit section and value.

  @param  PreSmmInit         Specify the target register table.
                             If TRUE, the target is the pre-SMM-init register table.
                             If FALSE, the target is the post-SMM-init register table.
  @param  ProcessorNumber    Handle number of specified logical processor
  @param  RegisterType       Type of the register to program
  @param  Index              Index of the register to program
  @param  ValidBitStart      Start of the bit section
  @param  ValidBitLength     Length of the bit section
  @param  Value              Value to write

**/
VOID
EFIAPI
WriteRegisterTableEx (
  IN BOOLEAN              PreSmmInit,
  IN UINTN                ProcessorNumber,
  IN REGISTER_TYPE        RegisterType,
  IN UINT32               Index,
  IN UINT8                ValidBitStart,
  IN UINT8                ValidBitLength,
  IN UINT64               Value
  )
{
  EFI_STATUS            Status;
  CPU_REGISTER_TABLE    *RegisterTable;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 AllocatePages;

  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);

  if (PreSmmInit) {
    RegisterTable = &mCpuConfigLibConfigConextBuffer->PreSmmInitRegisterTable[ProcessorNumber];
  } else {
    RegisterTable = &mCpuConfigLibConfigConextBuffer->RegisterTable[ProcessorNumber];
  }

  //
  // If register table is full, allocate one more page for it.
  // It must be in ACPI NVS memory under 4G, for consumption by S3 resume.
  //
  if (RegisterTable->TableLength == RegisterTable->AllocatedSize / sizeof (CPU_REGISTER_TABLE_ENTRY)) {
    Address = 0xffffffff;
    AllocatePages = RegisterTable->AllocatedSize / EFI_PAGE_SIZE;
    Status  = gBS->AllocatePages (
                     AllocateMaxAddress,
                     EfiACPIMemoryNVS,
                     AllocatePages + 1,
                     &Address
                     );
    ASSERT_EFI_ERROR (Status);

    //
    // If there are records existing in the register table, then copy its contents
    // to new region and free the old one.
    //
    if (RegisterTable->AllocatedSize > 0) {
      CopyMem (
        (VOID *) (UINTN) Address,
        RegisterTable->RegisterTableEntry,
        RegisterTable->AllocatedSize
        );
      //
      // RegisterTableEntry is allocated by gBS AllocatePages() service.
      // So, gBS FreePages() servcie is used to free it.
      //
      gBS->FreePages (
        (EFI_PHYSICAL_ADDRESS) (UINTN) (VOID *) RegisterTable->RegisterTableEntry,
        AllocatePages
        );
    }

    //
    // Adjust the allocated size and register table base address.
    //
    RegisterTable->AllocatedSize += EFI_PAGE_SIZE;
    RegisterTable->RegisterTableEntry = (CPU_REGISTER_TABLE_ENTRY *) (UINTN) Address;
  }

  //
  // Append entry in the register table.
  //
  RegisterTable->RegisterTableEntry[RegisterTable->TableLength].RegisterType   = RegisterType;
  RegisterTable->RegisterTableEntry[RegisterTable->TableLength].Index          = Index;
  RegisterTable->RegisterTableEntry[RegisterTable->TableLength].ValidBitStart  = ValidBitStart;
  RegisterTable->RegisterTableEntry[RegisterTable->TableLength].ValidBitLength = ValidBitLength;
  RegisterTable->RegisterTableEntry[RegisterTable->TableLength].Value          = Value;

  RegisterTable->TableLength++;
}

/**
  Add an entry in the post-SMM-init register table.
  
  This function adds an entry in the post-SMM-init register table, with given register type,
  register index, bit section and value.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  RegisterType    Type of the register to program
  @param  Index           Index of the register to program
  @param  ValidBitStart   Start of the bit section
  @param  ValidBitLength  Length of the bit section
  @param  Value           Value to write

**/
VOID
EFIAPI
WriteRegisterTable (
  IN  UINTN               ProcessorNumber,
  IN  REGISTER_TYPE       RegisterType,
  IN  UINT32              Index,
  IN  UINT8               ValidBitStart,
  IN  UINT8               ValidBitLength,
  IN  UINT64              Value
  )
{
  WriteRegisterTableEx (
    FALSE,
    ProcessorNumber,
    RegisterType,
    Index,
    ValidBitStart,
    ValidBitLength,
    Value
    );
}

/**
  Add an entry in the pre-SMM-init register table.
  
  This function adds an entry in the pre-SMM-init register table, with given register type,
  register index, bit section and value.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  RegisterType    Type of the register to program
  @param  Index           Index of the register to program
  @param  ValidBitStart   Start of the bit section
  @param  ValidBitLength  Length of the bit section
  @param  Value           Value to write

**/
VOID
EFIAPI
WritePreSmmInitRegisterTable (
  IN  UINTN               ProcessorNumber,
  IN  REGISTER_TYPE       RegisterType,
  IN  UINT32              Index,
  IN  UINT8               ValidBitStart,
  IN  UINT8               ValidBitLength,
  IN  UINT64              Value
  )
{
  WriteRegisterTableEx (
    TRUE,
    ProcessorNumber,
    RegisterType,
    Index,
    ValidBitStart,
    ValidBitLength,
    Value
    );
}

/**
  Set the sequence of processor setting.
  
  This function sets the a processor setting at the position in
  setting sequence specified by Index.

  @param  Index                  The zero-based index in the sequence.
  @param  ProcessorNumber        Handle number of the processor to set.

  @retval EFI_SUCCESS            The sequence successfully modified.
  @retval EFI_INVALID_PARAMETER  Index surpasses the boundary of sequence.
  @retval EFI_NOT_FOUND          Processor specified by ProcessorNumber does not exist. 

**/
EFI_STATUS
SetSettingSequence (
  IN UINTN  Index,
  IN UINTN  ProcessorNumber
  )
{
  ASSERT (mCpuConfigLibConfigConextBuffer != NULL);

  //
  // Check whether parameter Index is valid.
  //
  if (Index >= mCpuConfigLibConfigConextBuffer->NumberOfProcessors) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether parameter ProcessorNumber is valid.
  //
  if (ProcessorNumber >= mCpuConfigLibConfigConextBuffer->NumberOfProcessors) {
    return EFI_NOT_FOUND;
  }

  mCpuConfigLibConfigConextBuffer->SettingSequence[Index] = ProcessorNumber;

  return EFI_SUCCESS;
}

/**
  Set PcdCpuCallbackSignal, and then read the value back.
  
  This function sets PCD entry PcdCpuCallbackSignal. If there is callback
  function registered on it, the callback function will be triggered, and
  it may change the value of PcdCpuCallbackSignal. This function then reads
  the value of PcdCpuCallbackSignal back, the check whether it has been changed.

  @param  Value  The value to set to PcdCpuCallbackSignal.

  @return The value of PcdCpuCallbackSignal read back.

**/
UINT8
SetAndReadCpuCallbackSignal (
  IN UINT8  Value
  )
{
  //
  // Set PcdCpuCallbackSignal, and trigger callback function
  //
  PcdSet8 (PcdCpuCallbackSignal, Value);
  //
  // Read the value of PcdCpuCallbackSignal back. It may have been
  // changed by callback function.
  //
  return (PcdGet8 (PcdCpuCallbackSignal));
}

/**
  Worker function to create a new feature entry.
  
  This is a worker function to create a new feature entry. The new entry is added to
  the feature list by other functions in the library.
  
  @param  FeatureID   The ID of the feature.
  @param  Attribute   Feature-specific data.
  
  @return Pointer to the created entry.

**/
CPU_FEATURE_ENTRY *
CreateFeatureEntry (
  IN  CPU_FEATURE_ID      FeatureID,
  IN  VOID                *Attribute
  )
{
  CPU_FEATURE_ENTRY  *Node;

  Node = AllocateZeroPool (sizeof (CPU_FEATURE_ENTRY));
  ASSERT (Node != NULL);

  Node->Signature = EFI_CPU_FEATURE_ENTRY_SIGNATURE;
  Node->FeatureID = FeatureID;
  Node->Attribute = Attribute;

  return Node;
}

/**
  Worker function to search for a feature entry in processor feature list.
  
  This is a worker function to search for a feature entry in processor feature list.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureIndex    The index of the node in feature list.
  
  @return Pointer to the feature node. If FeatureIndex surpasses the length of list, NULL is returned.

**/
CPU_FEATURE_ENTRY *
SearchFeatureEntry (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               FeatureIndex
  )
{
  LIST_ENTRY                  *List;
  LIST_ENTRY                  *Link;
  CPU_FEATURE_ENTRY           *FeatureEntry;
  UINTN                       Index;

  List = &mCpuConfigLibConfigConextBuffer->FeatureLinkListEntry[ProcessorNumber];

  //
  // Get the first node in list. If list is empty, return NULL.
  //
  Link = GetFirstNode (List);
  if (IsNull (List, Link)) {
    return NULL;
  }

  //
  // Try to get the node specified by Index. If not found, return NULL.
  //
  for (Index = 1; Index < FeatureIndex; Index++) {
    Link = GetNextNode (List, Link);
    if (IsNull (List, Link)) {
      return NULL;
    }
  }

  FeatureEntry = CPU_FEATURE_ENTRY_FROM_LINK (Link);
  return FeatureEntry;
}
