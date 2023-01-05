/** @file
  Base Memory Allocation Routines Wrapper for Crypto library over OpenSSL
  during PEI & DXE phases.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenSslSupport.h>

//
// -- Memory-Allocation Routines --
//

/* Prefix allocations with the allocation's total size.
 *
 * This is used to implement `realloc`, which needs to know the
 * allocation's current size so that the data can be copied over to the
 * new allocation.
 */
struct AllocWithSize {
  /* Size of the whole allocation, including this `size` field.
   *
   * Allocations returned by AllocatePool are always eight-byte aligned
   * (according to the UEFI spec), so use an eight-byte `size` field to
   * make the `data` field also eight-byte aligned. */
  UINT64 size;

  /* The beginning of the allocation returned to the caller of malloc.
   *
   * The pointer to this data is what will then get passed into realloc
   * and free; those functions use ptr_to_alloc_with_size to get the
   * pointer to the underlying pool allocation. */
  UINT8 data[0];
};

/* Convert a `ptr` (as returned by malloc) to an AllocWithSize by
 * subtracting eight bytes. */
static struct AllocWithSize *ptr_to_alloc_with_size (void *ptr) {
  UINT8 *cptr = (UINT8*) ptr;
  return (struct AllocWithSize*) (cptr - sizeof(struct AllocWithSize));
}

/* Allocates memory blocks */
void *malloc (size_t size)
{
  UINTN alloc_size = (UINTN) (size + sizeof(struct AllocWithSize));

  struct AllocWithSize *alloc = AllocatePool (alloc_size);
  if (alloc) {
    alloc->size = alloc_size;
    return alloc->data;
  } else {
    return NULL;
  }
}

/* Reallocate memory blocks */
void *realloc (void *ptr, size_t size)
{
  struct AllocWithSize *alloc = ptr_to_alloc_with_size (ptr);
  UINTN old_size = alloc->size;
  UINTN new_size = (UINTN) (size + sizeof(struct AllocWithSize));

  alloc = ReallocatePool (alloc, old_size, new_size);
  if (alloc) {
    alloc->size = new_size;
    return alloc->data;
  } else {
    return NULL;
  }
}

/* De-allocates or frees a memory block */
void free (void *ptr)
{
  //
  // In Standard C, free() handles a null pointer argument transparently. This
  // is not true of FreePool() below, so protect it.
  //
  if (ptr != NULL) {
    ptr = ptr_to_alloc_with_size (ptr);

    FreePool (ptr);
  }
}
