/** @file

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

  MfhLib.c

Abstract:

  Library producing routines for Master Flash Header (MFH) functionality.

--*/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>

#include <Mfh.h>
#include <Library/MfhLib.h>

//
// Routines local to this source module.
//

STATIC
MFH_FLASH_ITEM *
FindNext (
  IN CONST UINT64                         ItemTypeFilter,
  IN CONST BOOLEAN                        IncrementBeforeMatchCheck,
  IN OUT MFH_LIB_FINDCONTEXT              *FindContext
  )
{
  UINT32                            *IdxPtr;
  UINT32                            FlashItemCount;
  UINT32                            BootPriorityListCount;
  MFH_FLASH_ITEM                    *FlashItem;

  if (IncrementBeforeMatchCheck) {
    if (FindContext->OnlyBootFlashItems) {
      FindContext->CurrBootPriorityListIndex++;
    } else {
      FindContext->CurrFlashItemIndex++;
    }
  }

  IdxPtr = &FindContext->CurrFlashItemIndex;
  FlashItemCount = FindContext->MfhHeader->FlashItemCount;
  BootPriorityListCount = FindContext->MfhHeader->BootPriorityListCount;

  do {
    if (FindContext->OnlyBootFlashItems) {

      //
      // Search for items referenced by BootPriorityList
      //
      if (FindContext->CurrBootPriorityListIndex >= BootPriorityListCount) {
        return NULL;  // End of list no more boot items.
      }

      *IdxPtr = FindContext->BootPriorityList[FindContext->CurrBootPriorityListIndex];
      FlashItem = &FindContext->FlashItemList[*IdxPtr];
      if (*IdxPtr >= FlashItemCount || FlashItem->Type > 63) {
        //
        // BootItems must have Index < FlashItemCount
        // and type <= 63
        //
        ASSERT (FALSE);
      } else {
        if (MFH_IS_FILTER_MATCH (ItemTypeFilter, FlashItem->Type)) {
          break;
        }
      }
      FindContext->CurrBootPriorityListIndex++;
      continue;
    }

    //
    // Search through all flash items.
    //
    if (*IdxPtr >= FlashItemCount) {
      return NULL;  // End of list no more items.
    }

    FlashItem = &FindContext->FlashItemList[*IdxPtr];
    //
    // Since filter is UINT64 then only check types with value < 64.
    //
    if (FlashItem->Type < 64 && MFH_IS_FILTER_MATCH (ItemTypeFilter, FlashItem->Type)) {
      break;
    }
    (*IdxPtr)++;
  } while (TRUE);
  return FlashItem;
}

//
// Routines exported by this source module.
//

/** Find First flash Item using item type filter.

  The bit position for a particular type is (1 << MFH_FLASH_ITEM_TYPE_XXX)

  @param[in]       ItemTypeFilter       Filter of item types to search for.
  @param[in]       OnlyBootFlashItems   Only find flash items referenced in boot
                                        priority list.
  @param[in out]   FindContext          Pointer to caller allocated storage for
                                        find context.

  @return pointer to MFH_FLASH_ITEM struct for found item.
  @retval NULL if MFH not found, corrupt or no items match filter.
**/
MFH_FLASH_ITEM *
EFIAPI
MfhLibFindFirstWithFilter (
  IN CONST UINT64                         ItemTypeFilter,
  IN CONST BOOLEAN                        OnlyBootFlashItems,
  IN OUT MFH_LIB_FINDCONTEXT              *FindContext
  )
{
  BOOLEAN                            IsValidMfh;

  IsValidMfh = MfhLibValidateMfhBaseAddress (
                 PcdGet32 (PcdFlashNvMfh),
                 &FindContext->MfhHeader,
                 &FindContext->BootPriorityList,
                 &FindContext->FlashItemList
                 );

  if (!IsValidMfh) {
    return NULL;
  }

  FindContext->CurrFlashItemIndex = 0;
  FindContext->CurrBootPriorityListIndex = 0;
  FindContext->OnlyBootFlashItems = OnlyBootFlashItems;
  return FindNext (ItemTypeFilter, FALSE, FindContext);
}

/** Find next flash Item using item type filter.

  @param[in]       ItemTypeFilter       Filter of item types to search for.
  @param[in out]   FindContext          Pointer to caller allocated storage for
                                        find context.

  @return pointer to MFH_FLASH_ITEM struct for found item.
  @retval NULL if no more items found for specified type.
**/
MFH_FLASH_ITEM *
EFIAPI
MfhLibFindNextWithFilter (
  IN CONST UINT64   ItemTypeFilter,
  IN OUT MFH_LIB_FINDCONTEXT              *FindContext
  )
{
  return FindNext (ItemTypeFilter, TRUE, FindContext);
}

/** Find First flash Item of a specific type.

  @param[in]       FlashItemType        One of MFH_FLASH_ITEM defs in Mfh.h.
  @param[in]       OnlyBootFlashItems   Only find flash items referenced in boot
                                        priority list.
  @param[in out]   FindContext          Pointer to caller allocated storage for
                                        find context.

  @return pointer to MFH_FLASH_ITEM struct for found item.
  @retval NULL if MFH not found, corrupt or no items found for specified type.
**/
MFH_FLASH_ITEM *
EFIAPI
MfhLibFindFirstOfType (
  IN CONST UINT32                         FlashItemType,
  IN CONST BOOLEAN                        OnlyBootFlashItems,
  IN OUT MFH_LIB_FINDCONTEXT              *FindContext
  )
{
  return MfhLibFindFirstWithFilter (
           MFH_FIT_BM (FlashItemType),
           OnlyBootFlashItems,
           FindContext
           );
}

/** Find next flash Item of a specific type.

  @param[in]       FlashItemType        One of MFH_FLASH_ITEM defs in Mfh.h.
  @param[in out]   FindContext          Pointer to caller allocated storage for
                                        find context.

  @return pointer to MFH_FLASH_ITEM struct for found item.
  @retval NULL if no more items found for specified type.
**/
MFH_FLASH_ITEM *
EFIAPI
MfhLibFindNextOfType (
  IN CONST UINT32                         FlashItemType,
  IN OUT MFH_LIB_FINDCONTEXT              *FindContext
  )
{
  return MfhLibFindNextWithFilter (
           MFH_FIT_BM (FlashItemType),
           FindContext
           );
}

/** Validate if MFH at specified base address is valid.

  Optional return of pointers to resources within MFH.

  @param[in]       MfhBaseAddress       Check memory at this location
                                        points to a valid MFH.
  @param[out]      MfhHeaderPtr         Optional return header part of MFH.
  @param[out]      BootPriorityListPtr  Optional return of boot priority list.
                                        points to a valid MFH.
  @param[out]      FlashItemListPtr     Optional return of flash item list.

  @retval TRUE     if MfhBaseAddress points to a valid MFH.
  @retval FALSE    if MfhBaseAddress points to an invalid MFH.
**/
BOOLEAN
EFIAPI
MfhLibValidateMfhBaseAddress (
  IN CONST UINT32                         MfhBaseAddress,
  OUT MFH_HEADER                          **MfhHeaderPtr OPTIONAL,
  OUT UINT32                              **BootPriorityListPtr OPTIONAL,
  OUT MFH_FLASH_ITEM                      **FlashItemListPtr OPTIONAL
  )
{
  UINT8                             *MemPtr;
  UINT32                            BootPriorityListCount;
  MFH_HEADER                        *MfhHeader;

  MfhHeader = (MFH_HEADER *) MfhBaseAddress;

  //
  // Do some basic sanity checks.
  //
  if (MfhHeader->QuarkMFHIdentifier != MFH_IDENTIFIER) {
    return FALSE;
  }
  if (MfhHeader->BootPriorityListCount > MFH_MAX_NUM_BOOT_ITEMS) {
    return FALSE;
  }
  if (MfhHeader->FlashItemCount > MFH_MAX_NUM_FLASH_ITEMS) {
    return FALSE;
  }

  MemPtr = (UINT8 *) MfhHeader;

  if (MfhHeaderPtr != NULL) {
    *MfhHeaderPtr = (MFH_HEADER *) MemPtr;
  }
  MemPtr += sizeof (MFH_HEADER);

  if (BootPriorityListPtr != NULL) {
    *BootPriorityListPtr = (UINT32 *) MemPtr;
  }

  if (FlashItemListPtr != NULL) {
    BootPriorityListCount = MfhHeader->BootPriorityListCount;
    MemPtr += (sizeof (UINT32) * BootPriorityListCount);
    *FlashItemListPtr = (MFH_FLASH_ITEM *) MemPtr;
  }

  return TRUE;
}

/** Find First stage1 flash module info or fixed recovery module info.

  @param[out]      FlashItemBuffer      Caller storage to receive flash item
                                        fields.
  @param[out]      GotFixedRecovery     TRUE if buffer filled with fixed
                                        recovery module info.

  @retval TRUE if found flash module.
  @retval FALSE if no flash module found.
**/
BOOLEAN
EFIAPI
MfhLibFindFirstStage1OrFixedRecovery (
  OUT MFH_FLASH_ITEM                     *FlashItemBuffer,
  OUT BOOLEAN                            *GotFixedRecovery OPTIONAL
  )
{
  MFH_LIB_FINDCONTEXT               FindContext;
  MFH_FLASH_ITEM                    *Item;

  Item = MfhLibFindFirstWithFilter (
           MFH_FIND_ALL_STAGE1_FILTER,
           TRUE,
           &FindContext
           );
  if (Item == NULL) {
    if (GotFixedRecovery != NULL) {
      *GotFixedRecovery = TRUE;
    }
    FlashItemBuffer->Type = MFH_FLASH_ITEM_TYPE_FW_RECOVERY;
    FlashItemBuffer->FlashAddress = PcdGet32 (PcdFlashFvFixedStage1AreaBase);
    FlashItemBuffer->LengthBytes = PcdGet32 (PcdFlashFvFixedStage1AreaSize);
    FlashItemBuffer->TypeSpecific.Reserved = 0;
  } else {
    if (GotFixedRecovery != NULL) {
      *GotFixedRecovery = FALSE;
    }
    FlashItemBuffer->Type = Item->Type;
    FlashItemBuffer->FlashAddress = Item->FlashAddress;
    FlashItemBuffer->LengthBytes = Item->LengthBytes;
    FlashItemBuffer->TypeSpecific.Reserved = Item->TypeSpecific.Reserved;
  }

  return TRUE;
}

/** Return string for printing flash item type.

  @param[in]       FlashItemType        One of MFH_FLASH_ITEM defs in Mfh.h.

  @return string for printing flash item type.
**/
CHAR16 *
EFIAPI
MfhLibFlashItemTypePrintString (
  IN CONST UINT32                         FlashItemType
  )
{
  CHAR16                            *Str;

  if (FlashItemType == MFH_FLASH_ITEM_TYPE_FW_STAGE1) {
    Str = L"Firmware stage1";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_FW_STAGE1_SIGNED) {
    Str = L"Firmware stage1 signed";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_RSVD_02H) {
    Str = L"Reserved 02Hex";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_FW_STAGE2) {
   Str = L"Firmware stage2";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_FW_STAGE2_SIGNED) {
    Str = L"Firmware stage2 signed";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_FW_STAGE2_CFG) {
    Str = L"Firmware stage2 config";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_FW_STAGE2_CFG_SIGNED) {
    Str = L"Firmware stage2 config signed";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_FW_PARAMETERS) {
    Str = L"Firmware parameters";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_FW_RECOVERY) {
   Str = L"Firmware recoverey";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_FW_RECOVERY_SIGNED) {
    Str = L"Firmware recoverey signed";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_RSVD_0AH) {
    Str = L"Reserved 0AHex item";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_BOOTLOADER) {
   Str = L"Bootloader";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_BOOTLOADER_SIGNED) {
    Str = L"Bootloader signed";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_BOOTLOADER_CFG) {
    Str = L"Bootloader config";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_BOOTLOADER_CFG_SIGNED) {
    Str = L"Bootloader config signed";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_RSVD_0F) {
    Str = L"Boot defender internal error";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_KERNEL) {
    Str = L"Kernel";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_KERNEL_SIGNED) {
    Str = L"Kernel signed";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_RAMDISK) {
    Str = L"Boot defender internal error";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_RAMDISK_SIGNED) {
    Str = L"RAMDISK";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_RSVD_14H) {
    Str = L"Reserved 14Hex item";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_PROGRAM) {
   Str = L"Program";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_PROGRAM_SIGNED) {
    Str = L"Program signed";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_RSVD_17H) {
    Str = L"Reserved 17Hex item";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_BUILD_INFORMATION) {
    Str = L"Build information";
  } else if (FlashItemType == MFH_FLASH_ITEM_TYPE_IMAGE_VERSION) {
    Str = L"Firmware image version";
  } else {
    Str = L"Flash Item Unknown";
  }
  return Str;
}

/** Check if Address within firmware flash item.

  @param[in]       CheckAddress  Address to check.

  @retval TRUE if check address with FW flash item.
  @retval FALSE if check address outside FW flash item.
**/
BOOLEAN
EFIAPI
MfhLibIsAddressWithinFwFlashItem (
  IN CONST UINT32                         CheckAddress
  )
{
  MFH_LIB_FINDCONTEXT               FindContext;
  MFH_FLASH_ITEM                    *FlashItem;
  UINT32                            LastItemAddress;
  UINT64                            Filter;

  Filter = MFH_FIND_ANY_FW_FIT_FILTER;

  FlashItem = MfhLibFindFirstWithFilter (
                Filter,
                FALSE,
                &FindContext
                );
  while (FlashItem != NULL) {
    if (CheckAddress > FlashItem->FlashAddress) {
      LastItemAddress = FlashItem->FlashAddress + (FlashItem->LengthBytes -1);
      if (CheckAddress <= LastItemAddress) {
        return TRUE;
      }
    }

    FlashItem = MfhLibFindNextWithFilter (
                  Filter,
                  &FindContext
                  );
  }
  return FALSE;
}

/** Find flash Item for boot priority index.

  @param[in]       BootPriorityIndex    Boot priority index of flash item
                                        we are looking for.

  @return pointer to MFH_FLASH_ITEM struct for found item.
  @retval NULL if MFH not found, corrupt or BootPriorityIndex >= ListCount.
**/
MFH_FLASH_ITEM *
EFIAPI
MfhLibFindFlashItemForBootPriorityIndex (
  IN CONST UINT32                         BootPriorityIndex
  )
{
  BOOLEAN                            IsValidMfh;
  MFH_LIB_FINDCONTEXT                FindContext;
  UINT32                             ItemIndex;

  IsValidMfh = MfhLibValidateMfhBaseAddress (
                 PcdGet32 (PcdFlashNvMfh),
                 &FindContext.MfhHeader,
                 &FindContext.BootPriorityList,
                 &FindContext.FlashItemList
                 );

  if ((IsValidMfh) && (BootPriorityIndex < FindContext.MfhHeader->BootPriorityListCount)) {
    ItemIndex = FindContext.BootPriorityList[BootPriorityIndex];
    return &FindContext.FlashItemList[ItemIndex];
  }
  return NULL;
}

