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

  MfhLib.h

Abstract:

  MfhLib function prototype definitions.

--*/

#ifndef __MFH_LIB_H__
#define __MFH_LIB_H__

#include "Mfh.h"

//
// Get bitmask for Flash Item Type (FIT) to be used in find filter routines.
//
#define MFH_FIT_BM(type) (LShiftU64 (1, (UINTN) (type)))

//
// Is type match in filter bitmap.
//
#define MFH_IS_FILTER_MATCH(filter, type)  \
          (((MFH_FIT_BM(type)) & (filter)) != 0)

//
// Some common find filters.
//
#define MFH_FIND_ANY_FIT_FILTER             ((UINT64) -1)  // Find any flash item.
#define MFH_FIND_ALL_STAGE1_FILTER  \
          ((UINT64) ((MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE1)) | (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE1_SIGNED))))

#define MFH_FIND_ALL_STAGE2_FILTER  \
          ((UINT64) ((MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE2)) | (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE2_SIGNED))))

#define MFH_FIND_ALL_BOOTLOADER_FILTER  \
          ((UINT64) ((MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_BOOTLOADER)) | (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_BOOTLOADER_SIGNED))))

#define MFH_FIND_IMAGE_VERSION_FILTER  \
          ((UINT64) (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_IMAGE_VERSION)))

//
// Filter to find any firmware flash item.
//
#define MFH_FIND_ANY_FW_FIT_FILTER           ((UINT64) ( \
          (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE1)) | \
          (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE1_SIGNED)) | \
          (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE2)) | \
          (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE2_SIGNED)) | \
          (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE2_CFG)) | \
          (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_STAGE2_CFG_SIGNED)) | \
          (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_PARAMETERS)) | \
          (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_RECOVERY)) | \
          (MFH_FIT_BM(MFH_FLASH_ITEM_TYPE_FW_RECOVERY_SIGNED)) \
          ))


///
/// Context structure to be used with FindFirst & FindNext routines
///
typedef struct {
  MFH_HEADER       *MfhHeader;                // Pointer to MFH Data structure in flash, filled out by FindFirst.
  UINT32           *BootPriorityList;         // Pointer to boot priority list.
  MFH_FLASH_ITEM   *FlashItemList;            // Pointer to flash item list.
  UINT32           CurrBootPriorityListIndex; // Current found boot priority index.
  UINT32           CurrFlashItemIndex;        // Current found flash item index.
  BOOLEAN          OnlyBootFlashItems;        // Only finding items refed in boot priority list.
} MFH_LIB_FINDCONTEXT;

/** Find First flash Item using item type filter.

  The bit position for a particular type is (1 << MFH_FLASH_ITEM_TYPE_XXX)

  @param[in]       ItemTypeFilter       Filter of item types to search for.
  @param[in]       OnlyBootFlashItems   Only find flash items referenced in boot
                                        priority list.
  @param[in out]   FindContext          Pointer to caller allocated storage for
                                        find context.

  @return pointer to MFH_FLASH_ITEM struct for found item.
  @retval NULL if MFH not found, corrupt or no items found for specified type.
**/
MFH_FLASH_ITEM *
EFIAPI
MfhLibFindFirstWithFilter (
  IN CONST UINT64                         ItemTypeFilter,
  IN CONST BOOLEAN                        OnlyBootFlashItems,
  IN OUT MFH_LIB_FINDCONTEXT              *FindContext
  );

/** Find next flash Item using item type filter.

  @param[in]       ItemTypeFilter       Filter of item types to search for.
  @param[in out]   FindContext          Pointer to caller allocated storage for
                                        find context.

  @return pointer to MFH_FLASH_ITEM struct for found item.
  @retval NULL if no items match filter.
**/
MFH_FLASH_ITEM *
EFIAPI
MfhLibFindNextWithFilter (
  IN CONST UINT64                         ItemTypeFilter,
  IN OUT MFH_LIB_FINDCONTEXT              *FindContext
  );

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
  );

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
  );

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
  );

/** Return string for printing flash item type.

  @param[in]       FlashItemType        One of MFH_FLASH_ITEM defs in Mfh.h.

  @return string for printing flash item type.
**/
CHAR16 *
EFIAPI
MfhLibFlashItemTypePrintString (
  IN CONST UINT32                         FlashItemType
  );

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
  );

/** Check if Address within firmware flash item.

  @param[in]       CheckAddress  Address to check.

  @retval TRUE if check address with FW flash item.
  @retval FALSE if check address outside FW flash item.
**/
BOOLEAN
EFIAPI
MfhLibIsAddressWithinFwFlashItem (
  IN CONST UINT32                         CheckAddress
  );

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
  );

#endif // #ifndef __MFH_LIB_H__
