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

  PlatformDataLib.h

Abstract:

  PlatformDataLib function prototype definitions.

--*/

#ifndef __PLATFORM_DATA_LIB_H__
#define __PLATFORM_DATA_LIB_H__

#include "PlatformData.h"

///
/// Get bitmask for Platform Item ID to be used in find filter routines.
///
#define PDAT_ITEM_ID_MASK(id) (LShiftU64 ((UINT64)1, (UINTN) (id)))

///
/// Is id match in filter bitmap.
///
#define PDAT_IS_FILTER_MATCH(filter, id)  \
          (((PDAT_ITEM_ID_MASK(id)) & (filter)) != 0)

//
// Some common find filters.
//

#define PDAT_FIND_ANY_ITEM_FILTER             ((UINT64) -1)  // Find any flash item.
#define PDAT_FIND_ACCOUNTING_ITEMS  \
          ((UINT64) ((PDAT_ITEM_ID_MASK(PDAT_ITEM_ID_PLATFORM_ID)) | (PDAT_ITEM_ID_MASK(PDAT_ITEM_ID_SERIAL_NUM))))

#define PDAT_FIND_NETWORK_ITEMS  \
          ((UINT64) ((PDAT_ITEM_ID_MASK(PDAT_ITEM_ID_MAC0)) | (PDAT_ITEM_ID_MASK(PDAT_ITEM_ID_MAC1))))

#define PDAT_FIND_MEMORY_ITEMS  \
          ((UINT64) ((PDAT_ITEM_ID_MASK(PDAT_ITEM_ID_MEM_CFG)) | (PDAT_ITEM_ID_MASK(PDAT_ITEM_ID_MRC_VARS))))

//
// Type defs for data types specific to this library.
//

///
/// Context structure to be used with FindFirst & FindNext routines.
///
typedef struct {
  PDAT_AREA          *Area;                ///< Pointer to Platform Data structure in flash, filled out by FindFirst.
  UINT8              *Limit;               ///< Pointer to 1st memory location after Platform Data Area.
  UINT8              *CurrMemLocation;     ///< Pointer to current position in platform data area.
} PDAT_LIB_FINDCONTEXT;

//
// Function prototypes for routines exported by this library.
//

/** Validate Platform Data Area.

  @param[in]       Area          Area to Validate.
  @param[in]       DoCrcCheck    Do CRC Check on area.

  @retval   EFI_SUCCESS          Area validated.
  @retval   EFI_NOT_FOUND        Signature not found in area.
  @retval   EFI_BAD_BUFFER_SIZE  Area has invalid length.
  @retval   EFI_CRC_ERROR        Area CRC Fail.
  @retval   Others               Unexpected error happened.

**/
EFI_STATUS
EFIAPI
PDatLibValidateArea (
  IN CONST PDAT_AREA                      *Area,
  IN CONST BOOLEAN                        DoCrcCheck
  );

/** Get pointer to System Platform Data Area.

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
  );

/** Find platform data item.

  @param[in]       UserArea             Platform Data Area to search
                                        if == NULL then use System area.
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
  );

/** Find First Item using item id filter.

  This routine will alway validate the platform data area (including CRC).

  WARNING:
    FindFirst / FindNext limited to item ids 0x0000 - 0x0003f.

  @param[in]       UserArea             Platform Data Area to search
                                        if == NULL then use System area.
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
  );

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
  );

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
  );

#endif // #ifndef __PLATFORM_DATA_LIB_H__
