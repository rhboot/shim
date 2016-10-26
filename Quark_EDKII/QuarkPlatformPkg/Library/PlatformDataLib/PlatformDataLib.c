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

  PlatformDataLib.c

Abstract:

  Library producing routines for platform data functionality.

--*/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>

#include <PlatformData.h>
#include <Library/PlatformHelperLib.h>
#include <Library/PlatformDataLib.h>

//
// Routines defined in other source modules of this component.
//

//
// Routines local to this source module.
//

STATIC
PDAT_ITEM *
FindNext (
  IN CONST UINT64                         ItemIdFilter,
  IN CONST BOOLEAN                        IncrementBeforeMatchCheck,
  IN OUT PDAT_LIB_FINDCONTEXT             *FindContext,
  OUT VOID                                *ItemData OPTIONAL
  )
{
  PDAT_ITEM                         *CurrItem;

  if (IncrementBeforeMatchCheck) {
    FindContext->CurrMemLocation += sizeof(PDAT_ITEM_HEADER) + ((PDAT_ITEM *) FindContext->CurrMemLocation)->Header.Length;
  }

  do {

    //
    // Search through all platform data items.
    //
    if (FindContext->CurrMemLocation >= FindContext->Limit) {
      return NULL;  // End of list no more items.
    }
    CurrItem = (PDAT_ITEM *) FindContext->CurrMemLocation;

    ///
    /// Return Current item if user wants all items.
    ///
    if (ItemIdFilter == PDAT_FIND_ANY_ITEM_FILTER) {
      break;
    }

    if (CurrItem->Header.Identifier < 64) {
      ///
      /// Only ids < 64 can be found with filter routines.
      ///
      if (PDAT_IS_FILTER_MATCH (ItemIdFilter, CurrItem->Header.Identifier)) {
        break;
      }
    }
    FindContext->CurrMemLocation += (sizeof(PDAT_ITEM_HEADER) + CurrItem->Header.Length);
  } while (TRUE);

  //
  // Found Item, update optional outputs and return pointer to item.
  //
  if (ItemData != NULL) {
     CopyMem (
       ItemData,
       (VOID *) &CurrItem->Data[0],
       CurrItem->Header.Length
       );
  }
  return CurrItem;
}

//
// Routines exported by this source module.
//

/** Validate Platform Data Area.

  @param[in]       Area          Area to Validate.
  @param[in]       DoCrcCheck    Do CRC Check on area.

  @retval  EFI_SUCCESS           Area validated.
  @retval  EFI_NOT_FOUND         Signature not found in area.
  @retval  EFI_BAD_BUFFER_SIZE   Area has invalid length.
  @retval  EFI_CRC_ERROR         Area CRC Fail.
  @retval  EFI_INVALID_PARAMETER Area parameter invalid.
  @retval  Others                Unexpected error happened.

**/
EFI_STATUS
EFIAPI
PDatLibValidateArea (
  IN CONST PDAT_AREA                      *Area,
  IN CONST BOOLEAN                        DoCrcCheck
  )
{
  UINT32                            CalcCrc;
  EFI_STATUS                        Status;

  if (Area == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (Area->Header.Identifier != PDAT_IDENTIFIER) {
    return EFI_NOT_FOUND;
  }
  if (Area->Header.Length > PcdGet32(PcdPlatformDataMaxLen)) {
    return EFI_BAD_BUFFER_SIZE;
  }
  Status = EFI_SUCCESS;
  if (DoCrcCheck) {
    Status = PlatformCalculateCrc32 (
               (UINT8 *) &Area->Body[0],
               (UINTN) Area->Header.Length,
               &CalcCrc
               );
    if (CalcCrc != Area->Header.Crc) {
      return EFI_CRC_ERROR;
    }
  }
  return Status;
}

/** Get pointer to system Platform Data Area.

  @param[in]       DoCrcCheck    Do CRC Check on area.
  @param[out]      AreaPtr       Pointer to pointer to receive address of
                                 platform data area.

  @retval   EFI_SUCCESS          Area validated.
  @retval   EFI_NOT_FOUND        Signature not found in area.
  @retval   EFI_BAD_BUFFER_SIZE  Area has invalid length.
  @retval   EFI_CRC_ERROR        Area CRC Fail.
  @retval   Others               Unexpected error happened.

**/
EFI_STATUS
EFIAPI
PDatLibGetSystemAreaPointer (
  IN CONST BOOLEAN                        DoCrcCheck,
  OUT PDAT_AREA                           **AreaPtr
  )
{
  PDAT_AREA                         *Area;
  EFI_STATUS                        Status;

  Area = (PDAT_AREA *) PcdGet32(PcdPlatformDataBaseAddress);

  *AreaPtr = NULL;
  Status = PDatLibValidateArea (Area, DoCrcCheck);
  if (!EFI_ERROR(Status)) {
    *AreaPtr = Area;
  }
  return Status;
}

/** Find platform data item.

  @param[in]       UserArea             Platform Data Area to search
                                        if == NULL then use system area.
  @param[in]       ItemId               One of PDAT_ITEM_ID_XXXX constants
                                        indicating item to find.
  @param[in]       DoCrcCheck           Do CRC Check on area.
  @param[out]      ItemData             Optional pointer to buffer to receive
                                        Item Data, caller reponsible that buffer
                                        is large enough.

  @return pointer to PDAT_ITEM struct for found item.
  @retval NULL if item not found or validate fails.
**/
PDAT_ITEM *
EFIAPI
PDatLibFindItem (
  IN PDAT_AREA                            *UserArea, OPTIONAL
  IN CONST UINT16                         ItemId,
  IN CONST BOOLEAN                        DoCrcCheck,
  OUT VOID                                *ItemData OPTIONAL
  )
{
  PDAT_ITEM                         *CurrItem;
  PDAT_AREA                         *Area;
  UINT8                             *CurrMemLocation;
  UINT8                             *Limit;
  EFI_STATUS                        Status;

  if (UserArea == NULL) {
    Status = PDatLibGetSystemAreaPointer (
               DoCrcCheck,
               &Area
               );
    ASSERT (Area != NULL);
  } else {
    Area = UserArea;
    Status = PDatLibValidateArea (Area, DoCrcCheck);
  }
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  CurrMemLocation = ((UINT8 *) Area) + sizeof (PDAT_AREA_HEADER);
  Limit = ((UINT8 *) Area) + sizeof (PDAT_AREA_HEADER) + Area->Header.Length;
  while (CurrMemLocation < Limit) {
    CurrItem = (PDAT_ITEM *) CurrMemLocation;

    if (CurrItem->Header.Identifier == ItemId) {

      //
      // Found Item, update optional outputs and return pointer to item.
      //
      if (ItemData != NULL) {
         CopyMem (
           ItemData,
           (VOID *) &CurrItem->Data[0],
           CurrItem->Header.Length
           );
      }
      return CurrItem;
    }
    CurrMemLocation += (sizeof(PDAT_ITEM_HEADER) + CurrItem->Header.Length);
  }

  return NULL;
}

/** Find First Item using item id filter.

  This routine will alway validate the platform data area (including CRC).

  WARNING:
    FindFirst / FindNext limited to item ids 0x0000 - 0x0003f.

  @param[in]       UserArea             Platform Data Area to search
                                        if == NULL then use system area.
  @param[in]       ItemIdFilter         Filter of item types to search for.
  @param[in out]   FindContext          Pointer to caller allocated storage for
                                        find context.
  @param[out]      ItemData             Optional pointer to buffer to receive
                                        Item Data, caller reponsible that buffer
                                        is large enough.

  @return pointer to PDAT_ITEM struct for found item.
  @retval NULL if item not found, area corrupt or reached end of platform data area.
**/
PDAT_ITEM *
EFIAPI
PDatLibFindFirstWithFilter (
  IN PDAT_AREA                            *UserArea, OPTIONAL
  IN CONST UINT64                         ItemIdFilter,
  IN OUT PDAT_LIB_FINDCONTEXT             *FindContext,
  OUT VOID                                *ItemData OPTIONAL
  )
{
  EFI_STATUS                        Status;

  if (UserArea == NULL) {
    Status = PDatLibGetSystemAreaPointer (
               TRUE,
               &FindContext->Area
               );
    if (EFI_ERROR (Status)) {
      return NULL;
    }
    ASSERT (FindContext->Area != NULL);
  } else {
    FindContext->Area = UserArea;
  }
  FindContext->CurrMemLocation =
    ((UINT8 *) FindContext->Area) + sizeof (PDAT_AREA_HEADER);

  FindContext->Limit = FindContext->CurrMemLocation + FindContext->Area->Header.Length;

  return FindNext (ItemIdFilter, FALSE, FindContext, ItemData);
}

/** Find next Item using item id filter.

  WARNING:
    FindFirst / FindNext limited to item ids 0x0000 - 0x0003f.

  @param[in]       ItemIdFilter         Filter of item types to search for.
  @param[in out]   FindContext          Pointer to caller allocated storage for
                                        find context.
  @param[out]      ItemData             Optional pointer to buffer to receive
                                        Item Data, caller reponsible that buffer
                                        is large enough.

  @return pointer to PDAT_ITEM struct for found item.
  @retval NULL if reached end of platform data area.
**/
PDAT_ITEM *
EFIAPI
PDatLibFindNextWithFilter (
  IN CONST UINT64                         ItemIdFilter,
  IN OUT PDAT_LIB_FINDCONTEXT             *FindContext,
  OUT VOID                                *ItemData OPTIONAL
  )
{
  return FindNext (ItemIdFilter, TRUE, FindContext, ItemData);
}

/** Copy platform data area and filter out specified items.

  Only able to filter out item ids 0x0000 - 0x0003f.

  @param[out]      DestArea             User allocated buffer to receive
                                        SrcArea with ItemIdFilter items
                                        removed.
  @param[in]       SrcArea              Area to copy.
  @param[in]       ItemIdFilter         Filter of items not to copy into DestArea.

  @retval  EFI_SUCCESS                  Operation success.
  @retval  EFI_INVALID_PARAMETER        Invalid parameter.
  @retval  Others                       Unexpected error happened.
**/
EFI_STATUS
EFIAPI
PDatLibCopyAreaWithFilterOut (
  OUT PDAT_AREA                           *DestArea,
  IN PDAT_AREA                            *SrcArea,
  IN CONST UINT64                         ItemIdFilter
  )
{
  EFI_STATUS                        Status;
  PDAT_ITEM                         *Item;
  PDAT_LIB_FINDCONTEXT              PDatFindContext;
  UINTN                             FilteredCount;
  UINTN                             ItemTotalLen;
  UINT8                             *CurrMemLocation;

  if (DestArea == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = PDatLibValidateArea (SrcArea, TRUE);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  Item = PDatLibFindFirstWithFilter (SrcArea, PDAT_FIND_ANY_ITEM_FILTER, &PDatFindContext, NULL);
  FilteredCount = 0;
  CopyMem (((UINT8 *) DestArea), ((UINT8 *) SrcArea), sizeof (PDAT_AREA_HEADER));
  CurrMemLocation = ((UINT8 *) DestArea) + sizeof (PDAT_AREA_HEADER);
  if (Item != NULL) {
    do {
      ItemTotalLen = sizeof(PDAT_ITEM_HEADER) + Item->Header.Length;
      if (Item->Header.Identifier < 64) {
        ///
        /// Only ids < 64 can be filtered out.
        ///
        if (PDAT_IS_FILTER_MATCH (ItemIdFilter, Item->Header.Identifier)) {
          DestArea->Header.Length -= ItemTotalLen;
          FilteredCount++;
          ItemTotalLen = 0;
        }
      }
      if (ItemTotalLen > 0) {
        CopyMem (CurrMemLocation, (UINT8 *) Item, ItemTotalLen);
        CurrMemLocation += ItemTotalLen;
      }
      Item = PDatLibFindNextWithFilter(PDAT_FIND_ANY_ITEM_FILTER, &PDatFindContext, NULL);
    } while (Item != NULL);
  }
  if (FilteredCount > 0) {
    Status = PlatformCalculateCrc32 (
           (UINT8 *) &DestArea->Body[0],
           (UINTN) DestArea->Header.Length,
           &DestArea->Header.Crc
           );
  }
  return PDatLibValidateArea (DestArea, FALSE);
}
