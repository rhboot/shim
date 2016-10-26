/** @file
  BDS routines to handle capsules.

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
#include "InternalBdsLib.h"

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
  )
{
  EFI_STATUS                  Status;
  EFI_PEI_HOB_POINTERS        HobPointer;
  EFI_CAPSULE_HEADER          *CapsuleHeader;
  UINT32                      Size;
  UINT32                      CapsuleNumber;
  UINT32                      CapsuleTotalNumber;
  EFI_CAPSULE_TABLE           *CapsuleTable;
  UINT32                      Index;
  UINT32                      CacheIndex;
  UINT32                      CacheNumber;
  VOID                        **CapsulePtr;
  VOID                        **CapsulePtrCache;
  EFI_GUID                    *CapsuleGuidCache; 

  CapsuleNumber = 0;
  CapsuleTotalNumber = 0;
  CacheIndex   = 0;
  CacheNumber  = 0;
  CapsulePtr        = NULL;
  CapsulePtrCache   = NULL;
  CapsuleGuidCache  = NULL;

  //
  // We don't do anything else if the boot mode is not flash-update
  //
  ASSERT (GetBootModeHob () == BOOT_ON_FLASH_UPDATE);
  
  Status = EFI_SUCCESS;
  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList ();
  while ((HobPointer.Raw = GetNextHob (EFI_HOB_TYPE_UEFI_CAPSULE, HobPointer.Raw)) != NULL) {
    CapsuleTotalNumber ++;
    HobPointer.Raw = GET_NEXT_HOB (HobPointer);
  }
  
  if (CapsuleTotalNumber == 0) {
    //
    // We didn't find a hob, so had no errors.
    //
    return EFI_SUCCESS;
  }
  
  //
  // Init temp Capsule Data table.
  //
  CapsulePtr       = (VOID **) AllocateZeroPool (sizeof (VOID *) * CapsuleTotalNumber);
  ASSERT (CapsulePtr != NULL);
  CapsulePtrCache  = (VOID **) AllocateZeroPool (sizeof (VOID *) * CapsuleTotalNumber);
  ASSERT (CapsulePtrCache != NULL);
  CapsuleGuidCache = (EFI_GUID *) AllocateZeroPool (sizeof (EFI_GUID) * CapsuleTotalNumber);
  ASSERT (CapsuleGuidCache != NULL);
  
  //
  // Find all capsule images from hob
  //
  HobPointer.Raw = GetHobList ();
  while ((HobPointer.Raw = GetNextHob (EFI_HOB_TYPE_UEFI_CAPSULE, HobPointer.Raw)) != NULL) {
    CapsulePtr [CapsuleNumber++] = (VOID *) (UINTN) HobPointer.Capsule->BaseAddress;
    HobPointer.Raw = GET_NEXT_HOB (HobPointer);
  }

  //
  //Check the capsule flags,if contains CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE, install
  //capsuleTable to configure table with EFI_CAPSULE_GUID
  //

  //
  // Capsules who have CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE always are used for operating
  // System to have information persist across a system reset. EFI System Table must 
  // point to an array of capsules that contains the same CapsuleGuid value. And agents
  // searching for this type capsule will look in EFI System Table and search for the 
  // capsule's Guid and associated pointer to retrieve the data. Two steps below describes
  // how to sorting the capsules by the unique guid and install the array to EFI System Table. 
  // Firstly, Loop for all coalesced capsules, record unique CapsuleGuids and cache them in an 
  // array for later sorting capsules by CapsuleGuid.
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index++) {
    CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0) {
      //
      // For each capsule, we compare it with known CapsuleGuid in the CacheArray.
      // If already has the Guid, skip it. Whereas, record it in the CacheArray as 
      // an additional one.
      //
      CacheIndex = 0;
      while (CacheIndex < CacheNumber) {
        if (CompareGuid(&CapsuleGuidCache[CacheIndex],&CapsuleHeader->CapsuleGuid)) {
          break;
        }
        CacheIndex++;
      }
      if (CacheIndex == CacheNumber) {
        CopyMem(&CapsuleGuidCache[CacheNumber++],&CapsuleHeader->CapsuleGuid,sizeof(EFI_GUID));
      }
    }
  }

  //
  // Secondly, for each unique CapsuleGuid in CacheArray, gather all coalesced capsules
  // whose guid is the same as it, and malloc memory for an array which preceding
  // with UINT32. The array fills with entry point of capsules that have the same
  // CapsuleGuid, and UINT32 represents the size of the array of capsules. Then install
  // this array into EFI System Table, so that agents searching for this type capsule
  // will look in EFI System Table and search for the capsule's Guid and associated
  // pointer to retrieve the data.
  //
  CacheIndex = 0;
  while (CacheIndex < CacheNumber) {
    CapsuleNumber = 0;  
    for (Index = 0; Index < CapsuleTotalNumber; Index++) {
      CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
      if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0) {
        if (CompareGuid (&CapsuleGuidCache[CacheIndex], &CapsuleHeader->CapsuleGuid)) {
          //
          // Cache Caspuleheader to the array, this array is uniqued with certain CapsuleGuid.
          //
          CapsulePtrCache[CapsuleNumber++] = (VOID*)CapsuleHeader;
        }
      }
    }
    if (CapsuleNumber != 0) {
      Size = sizeof(EFI_CAPSULE_TABLE) + (CapsuleNumber - 1) * sizeof(VOID*);  
      CapsuleTable = AllocateRuntimePool (Size);
      ASSERT (CapsuleTable != NULL);
      CapsuleTable->CapsuleArrayNumber =  CapsuleNumber;
      CopyMem(&CapsuleTable->CapsulePtr[0], CapsulePtrCache, CapsuleNumber * sizeof(VOID*));
      Status = gBS->InstallConfigurationTable (&CapsuleGuidCache[CacheIndex], (VOID*)CapsuleTable);
      ASSERT_EFI_ERROR (Status);
    }
    CacheIndex++;
  }

  //
  // Besides ones with CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flag, all capsules left are
  // recognized by platform with CapsuleGuid. For general platform driver, UpdateFlash 
  // type is commonly supported, so here only deal with encapsuled FVs capsule. Additional
  // type capsule transaction could be extended. It depends on platform policy.
  //
  for (Index = 0; Index < CapsuleTotalNumber; Index++) {
    CapsuleHeader = (EFI_CAPSULE_HEADER*) CapsulePtr [Index];
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) {
      //
      // Call capsule library to process capsule image.
      //
      ProcessCapsuleImage (CapsuleHeader);
    }
  }
  
  //
  // Free the allocated temp memory space.
  //
  FreePool (CapsuleGuidCache);
  FreePool (CapsulePtrCache);
  FreePool (CapsulePtr);

  return Status;
}

