/** @file
  Provide capsule services to this component.

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
#include "PlatformEarlyInit.h"

/**
  Check every capsule header.

  @param CapsuleHeader   The pointer to EFI_CAPSULE_HEADER

  @retval FALSE  Capsule is OK
  @retval TRUE   Capsule is corrupted 

**/
STATIC
BOOLEAN
IsCapsuleCorrupted (
  IN EFI_CAPSULE_HEADER       *CapsuleHeader
  )
{
  EFI_STATUS                  Status;

  //
  // Check capsule header supported by our platform.
  //
  Status = PlatformSupportCapsuleImage (CapsuleHeader);
  if (EFI_ERROR(Status)) {
    return TRUE;
  }

  //
  // A capsule to be updated across a system reset should contain CAPSULE_FLAGS_PERSIST_ACROSS_RESET.
  //
  if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) == 0) {
    return TRUE;
  }
  //
  // Make sure the flags combination is supported by the platform.
  //
  if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)) == CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) {
    return TRUE;
  }
  if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_INITIATE_RESET)) == CAPSULE_FLAGS_INITIATE_RESET) {
    return TRUE;
  }
  return FALSE;
}

/**
  Build security headers hobs

  Build Hob for each secuity header found in descriptor list.

  @param BlockList       Pointer to the capsule descriptors.

  @retval 0 if no security headers found.
  @return number of security header hobs built.

**/
STATIC
UINTN
BuildSecurityHeaderHobs (
  IN EFI_CAPSULE_BLOCK_DESCRIPTOR    *BlockList
  )
{
  EFI_CAPSULE_HEADER             *CapsuleHeader;
  UINT64                         CapsuleSize;
  UINT32                         CapsuleCount;
  EFI_CAPSULE_BLOCK_DESCRIPTOR   *Ptr;
  UINT8                          *Found;
  UINTN                          HobCnt;

  //
  // Init locals.
  //
  CapsuleSize  = 0;
  CapsuleCount = 0;
  Ptr = BlockList;
  Found = NULL;
  HobCnt = 0;
  while ((Ptr->Length != 0) || (Ptr->Union.ContinuationPointer != (EFI_PHYSICAL_ADDRESS) (UINTN) NULL)) {
    //
    // Make sure the descriptor is aligned at UINT64 in memory.
    //
    if ((UINTN) Ptr & 0x07) {
      DEBUG ((EFI_D_ERROR, "PlatCapsule: BlockList address failed alignment check\n"));
      return 0;
    }

    if (Ptr->Length == 0) {
      //
      // Descriptor points to another list of block descriptors somewhere
      // else.
      //
      Ptr = (EFI_CAPSULE_BLOCK_DESCRIPTOR  *) (UINTN) Ptr->Union.ContinuationPointer;
    } else {
      //
      // To enhance the reliability of check-up, the first capsule's header is checked here.
      // More reliabilities check-up will do later.
      //
      if (CapsuleSize == 0) {
        //
        //Move to the first capsule to check its header.
        //
        CapsuleHeader = (EFI_CAPSULE_HEADER*)((UINTN)Ptr->Union.DataBlock);
        Found = (UINT8 *) CapsuleHeader;
        Found -= FixedPcdGet32 (PcdFvSecurityHeaderSize);

        if (IsCapsuleCorrupted (CapsuleHeader)) {
          return 0;
        }
        Found = BuildCapsuleSecurityHeaderHob (Found);
        DEBUG ((EFI_D_INFO, "PlatCapsule:SecHdrHob: 0x%08x\n", (UINTN) Found));
        HobCnt += (Found != NULL) ? 1 : 0;

        CapsuleCount ++;
        CapsuleSize = CapsuleHeader->CapsuleImageSize;
      }

      if (CapsuleSize >= Ptr->Length) {
        CapsuleSize = CapsuleSize - Ptr->Length;
      } else {
        CapsuleSize = 0;
      }

      //
      // Move to next BLOCK descriptor.
      //
      Ptr++;
    }
  }

  return HobCnt;
}

/**
  Checks for the presence of capsule descriptors.
  Get capsule descriptors from variable CapsuleUpdateData, CapsuleUpdateData1, CapsuleUpdateData2...
  and save to DescriptorBuffer.

  @param DescriptorBuffer        Pointer to the capsule descriptors

  @retval EFI_SUCCESS     a valid capsule is present
  @retval EFI_NOT_FOUND   if a valid capsule is not present
**/
STATIC
EFI_STATUS
GetCapsuleDescriptors (
  IN EFI_PHYSICAL_ADDRESS     *DescriptorBuffer
  )
{
  EFI_STATUS                       Status;
  UINTN                            Size;
  UINTN                            Index;
  UINTN                            TempIndex;
  UINTN                            ValidIndex;
  BOOLEAN                          Flag;
  CHAR16                           CapsuleVarName[30];
  CHAR16                           *TempVarName;
  EFI_PHYSICAL_ADDRESS             CapsuleDataPtr64;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *PPIVariableServices;

  Index             = 0;
  TempVarName       = NULL;
  CapsuleVarName[0] = 0;
  ValidIndex        = 0;
  
  Status = PeiServicesLocatePpi (
              &gEfiPeiReadOnlyVariable2PpiGuid,
              0,
              NULL,
              (VOID **) &PPIVariableServices
              );
  if (Status == EFI_SUCCESS) {
    StrCpy (CapsuleVarName, EFI_CAPSULE_VARIABLE_NAME);
    TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
    Size = sizeof (CapsuleDataPtr64);
    while (TRUE) {
      if (Index == 0) {
        //
        // For the first Capsule Image
        //
        Status = PPIVariableServices->GetVariable (
                                        PPIVariableServices,
                                        CapsuleVarName,
                                        &gEfiCapsuleVendorGuid,
                                        NULL,
                                        &Size,
                                        (VOID *) &CapsuleDataPtr64
                                        );
        if (EFI_ERROR (Status)) {
          DEBUG ((EFI_D_ERROR, "PlatCapsule: capsule variable not set (%r)\n", Status));
          return EFI_NOT_FOUND;
        }
        //
        // We have a chicken/egg situation where the memory init code needs to
        // know the boot mode prior to initializing memory. For this case, our
        // validate function will fail. We can detect if this is the case if blocklist
        // pointer is null. In that case, return success since we know that the
        // variable is set.
        //
        if (DescriptorBuffer == NULL) {
          return EFI_SUCCESS;
        }
      } else {
        UnicodeValueToString (TempVarName, 0, Index, 0);
        Status = PPIVariableServices->GetVariable (
                                        PPIVariableServices,
                                        CapsuleVarName,
                                        &gEfiCapsuleVendorGuid,
                                        NULL,
                                        &Size,
                                        (VOID *) &CapsuleDataPtr64
                                        );
        if (EFI_ERROR (Status)) {
          break;
        }
        
        //
        // If this BlockList has been linked before, skip this variable
        //
        Flag = FALSE;
        for (TempIndex = 0; TempIndex < ValidIndex; TempIndex++) {
          if (DescriptorBuffer[TempIndex] == CapsuleDataPtr64)  {
            Flag = TRUE;
            break;
          }
        }
        if (Flag) {
          Index ++;
          continue;
        }
      }
      
      //
      // Cache BlockList which has been processed
      //
      DescriptorBuffer[ValidIndex++] = CapsuleDataPtr64;
      Index ++;
    }
  }
  
  return EFI_SUCCESS;
}

/**
  Build capsule security header hob.

  @param SecHdr  Pointer to security header.

  @retval NULL if failure to build HOB.
  @return pointer to built hob.
**/
VOID *
EFIAPI
BuildCapsuleSecurityHeaderHob (
  IN VOID                                 *SecHdr
  )
{
  QuarkSecurityHeader_t *SecHdrFields;

  if (SecHdr != NULL) {
    SecHdrFields = (QuarkSecurityHeader_t *) SecHdr;
    if (SecHdrFields->CSH_Identifier == QUARK_CSH_IDENTIFIER) {
      return BuildGuidDataHob(
               &gEfiQuarkCapsuleSecurityHeaderGuid,
               SecHdr,
               FixedPcdGet32 (PcdFvSecurityHeaderSize)
               );
    }
  }
  return NULL;
}

/**
  Find security headers using EFI_CAPSULE_VARIABLE_NAME variables and build Hobs.

  @param PeiServices  General purpose services available to every PEIM.

  @retval 0 if no security headers found.
  @return number of security header hobs built.
**/
UINTN
EFIAPI
FindCapsuleSecurityHeadersAndBuildHobs (
  IN EFI_PEI_SERVICES                     **PeiServices
  )
{
  UINTN                                Index;
  UINTN                                Size;
  UINTN                                VariableCount;
  CHAR16                               CapsuleVarName[30];
  CHAR16                               *TempVarName;
  EFI_PHYSICAL_ADDRESS                 CapsuleDataPtr64;
  EFI_STATUS                           Status;
  EFI_BOOT_MODE                        BootMode;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI      *PPIVariableServices;
  EFI_PHYSICAL_ADDRESS                 *VariableArrayAddress;
  UINTN                                HobCnt;

  HobCnt                  = 0;
  Index                   = 0;
  VariableCount           = 0;
  CapsuleVarName[0]       = 0;

  //
  // Someone should have already ascertained the boot mode. If it's not
  // capsule update, then return normally.
  //
  Status = PeiServicesGetBootMode (&BootMode);
  if (EFI_ERROR (Status) || (BootMode != BOOT_ON_FLASH_UPDATE)) {
    DEBUG ((EFI_D_ERROR, "PlatCapsule: Boot mode is not correct for finding capsule security header.\n"));    
    return 0;
  }

  //
  // User may set the same ScatterGatherList with several different variables,
  // so cache all ScatterGatherList for check later.
  //
  Status = PeiServicesLocatePpi (
              &gEfiPeiReadOnlyVariable2PpiGuid,
              0,
              NULL,
              (VOID **) &PPIVariableServices
              );
  ASSERT_EFI_ERROR (Status);

  Size = sizeof (CapsuleDataPtr64);
  StrCpy (CapsuleVarName, EFI_CAPSULE_VARIABLE_NAME);
  TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
  while (TRUE) {
    if (Index > 0) {
      UnicodeValueToString (TempVarName, 0, Index, 0);
    }
    Status = PPIVariableServices->GetVariable (
                                    PPIVariableServices,
                                    CapsuleVarName,
                                    &gEfiCapsuleVendorGuid,
                                    NULL,
                                    &Size,
                                    (VOID *) &CapsuleDataPtr64
                                    );
    if (EFI_ERROR (Status)) {
      //
      // There is no capsule variables, quit
      //
      break;
    }
    VariableCount++;
    Index++;
  }

  //
  // The last entry is the end flag.
  //
  Status = PeiServicesAllocatePool (
             (VariableCount + 1) * sizeof (EFI_PHYSICAL_ADDRESS),
             (VOID **)&VariableArrayAddress
             );

  ASSERT_EFI_ERROR (Status);

  ZeroMem (VariableArrayAddress, (VariableCount + 1) * sizeof (EFI_PHYSICAL_ADDRESS));

  //
  // Find out if we actually have a capsule.
  // GetCapsuleDescriptors depends on variable PPI, so it should run in 32-bit environment.
  //
  Status = GetCapsuleDescriptors (VariableArrayAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "PlatCapsule: Fail to find capsule variables.\n"));
  } else {
    //
    // Run through desciptors looking for capsule security headers.
    //

    Index             = 0;
    while (VariableArrayAddress[Index] != 0) {
      HobCnt += BuildSecurityHeaderHobs ((EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)VariableArrayAddress[Index]);
      Index ++;
    }
  }

  return HobCnt;
}
