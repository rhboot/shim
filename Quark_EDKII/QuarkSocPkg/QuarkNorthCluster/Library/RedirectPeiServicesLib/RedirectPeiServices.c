/** @file
  Implmentation module for redirect PEI services.

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

#include <PiPei.h>

#include <Guid/RedirectServicesHob.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/RedirectPeiServicesLib.h>

//
// Context structure for Redirect PEI services, must be allocated in memory
// shared by all PEI executables.
//
typedef struct {
  EFI_PEI_SERVICES                  OriginalPeiServices;
  UINTN                             AllocCount;
  UINTN                             NextAllocLoc;
  VOID                              *PoolBuffer;
  UINTN                             PoolBufferLength;
} REDIRECT_CONTEXT;

STATIC
REDIRECT_CONTEXT *
GetRedirectContext (
  IN CONST EFI_PEI_SERVICES               **PeiServices
  )
{
  return (REDIRECT_CONTEXT *) GetFirstGuidHob (&gRedirectServicesHobGuid);
}

STATIC
REDIRECT_CONTEXT *
CreateRedirectContext (
  IN CONST EFI_PEI_SERVICES               **PeiServices
  )
{
  REDIRECT_CONTEXT                  Context;
  VOID                              *BuiltHob;

  //
  // Use HOB to allow context to be shared by all PEI executables.
  //

  ZeroMem (&Context, sizeof(Context));
  BuiltHob = BuildGuidDataHob (
               &gRedirectServicesHobGuid,
               (VOID *) &Context,
               sizeof(Context)
               );
  ASSERT (BuiltHob != NULL);

  return GetRedirectContext (PeiServices);
}

STATIC
CHAR16 *
MemoryTypePrintString (
  IN EFI_MEMORY_TYPE                MemoryType
  )
{
  CHAR16                            *Str;

  if (MemoryType == EfiLoaderCode) {
    Str = L"LoaderCode";
  } else if (MemoryType == EfiLoaderData) {
    Str = L"LoaderData";
  } else if (MemoryType == EfiRuntimeServicesCode) {
    Str = L"RuntimeServicesCode";
  } else if (MemoryType == EfiRuntimeServicesData) {
   Str = L"RuntimeServicesData";
  } else if (MemoryType == EfiBootServicesCode) {
    Str = L"BootServicesCode";
  } else if (MemoryType == EfiBootServicesData) {
    Str = L"BootServicesData";
  } else if (MemoryType == EfiACPIReclaimMemory) {
    Str = L"ACPIReclaimMemory";
  } else if (MemoryType == EfiACPIMemoryNVS) {
    Str = L"ACPIMemoryNVS";
  } else {
    Str = L"MemTypeUnknown";
  }
  return Str;
}

STATIC
EFI_STATUS
RedirectAllocCommon (
  IN REDIRECT_CONTEXT                     *Context,
  IN CONST UINTN                          Pages,
  OUT UINTN                               *ThisAllocLocPtr
  )
{
  UINTN                             MaxLimit;
  UINTN                             ThisAllocLimit;
  UINTN                             ThicAllocInc;

  if (Context->PoolBufferLength == 0) {
    return EFI_NOT_READY;
  }

  MaxLimit =
    ((UINTN) Context->PoolBuffer) +
    (Context->PoolBufferLength - 1);

  ThicAllocInc = (Pages * EFI_PAGE_SIZE);
  ThisAllocLimit = Context->NextAllocLoc + (ThicAllocInc - 1);
  if (ThisAllocLimit <= MaxLimit) {
    *ThisAllocLocPtr = Context->NextAllocLoc;
    Context->NextAllocLoc += ThicAllocInc;
    Context->AllocCount = Context->AllocCount + 1;
    return EFI_SUCCESS;
  }
  return EFI_OUT_OF_RESOURCES;
}

/**
  Allocates pages from redirected memory pool.

  @param  Type                   The type of allocation to perform (ignored,
                                 left here for compatibility with core services)
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into
  @param  NumberOfPages          The number of pages to allocate
  @param  Memory                 A pointer to receive the base allocated memory
                                 address

  @return Status. On success, Memory is filled in with the base address allocated
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_NOT_READY          Redirected memory pool not setup.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
RedirectPeiAllocatePages (
  IN CONST EFI_PEI_SERVICES               **PeiServices,
  IN       EFI_MEMORY_TYPE                MemoryType,
  IN       UINTN                          Pages,
  OUT      EFI_PHYSICAL_ADDRESS           *Memory
  )
{
  REDIRECT_CONTEXT                  *Context;
  UINTN                             ThisAllocLoc;
  EFI_STATUS                        Status;

  if (Memory == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get PEI executable shared context structure.
  //
  Context = GetRedirectContext (GetPeiServicesTablePointer ());
  ASSERT (Context != NULL);

  //
  // Allocate memory from redirected memory pool and return
  // memory location to caller.
  //
  Status = RedirectAllocCommon (Context, Pages, &ThisAllocLoc);
  if (!EFI_ERROR(Status)) {
   *Memory = (EFI_PHYSICAL_ADDRESS) ThisAllocLoc;
  }
  return Status;
}

/**

  Memory allocation service for memory from redirected memory pool.

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Size               Amount of memory required
  @param Buffer             Address of pointer to the buffer

  @retval EFI_SUCCESS              The allocation was successful
  @retval EFI_OUT_OF_RESOURCES     There is not enough memory to satisfy the requirement
                                   to allocate the requested size.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
  @retval EFI_NOT_READY            Redirected memory pool not setup.

**/
EFI_STATUS
EFIAPI
RedirectPeiAllocatePool (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN       UINTN                Size,
  OUT      VOID                 **Buffer
  )
{
  REDIRECT_CONTEXT                  *Context;
  UINTN                             Pages;
  UINTN                             ThisAllocLoc;
  EFI_STATUS                        Status;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get PEI executable shared context structure.
  //
  Context = GetRedirectContext (GetPeiServicesTablePointer ());
  ASSERT (Context != NULL);

  //
  // Always allocated memory in PAGE SIZE chunks.
  //
  Pages = Size / EFI_PAGE_SIZE;
  if ((Size % EFI_PAGE_SIZE) != 0) {
    Pages++;
  }

  //
  // Allocate memory from redirected memory pool and return
  // memory location to caller.
  //
  Status = RedirectAllocCommon (Context, Pages, &ThisAllocLoc);
  if (!EFI_ERROR(Status)) {
   *Buffer = (VOID *) ThisAllocLoc;
  }
  return Status;
}

/**

  Init RedirectedPeiServices Lib.

**/
VOID
EFIAPI
RedirectServicesInit (
  VOID
  )
{
  CONST EFI_PEI_SERVICES            **PeiServices;
  REDIRECT_CONTEXT                  *Context;

  PeiServices = GetPeiServicesTablePointer ();
  Context = GetRedirectContext (PeiServices);
  if (Context != NULL) {
    DEBUG ((EFI_D_INFO, "RedirectServicesInit: already initialised\n"));
    return;
  }

  //
  // Create PEI executable shared Context struct.
  //
  Context = CreateRedirectContext (PeiServices);
  ASSERT (Context != NULL);
}

/**

  Set memory pool to be used for redirected PEI memory alloc. services.

  @param PoolBuffer               Address of memory pool.
  @param PoolBufferLength         Length of memory pool.

**/
VOID
EFIAPI
RedirectMemoryServicesSetPool (
  IN VOID                                 *PoolBuffer,
  IN CONST UINTN                          PoolBufferLength
  )
{
  REDIRECT_CONTEXT                  *Context;

  //
  // Get PEI executable shared context structure.
  //
  Context = GetRedirectContext (GetPeiServicesTablePointer ());
  ASSERT (Context != NULL);

  //
  // Init memory alloc fields in context structure.
  //
  Context->PoolBuffer = PoolBuffer;
  Context->PoolBufferLength = PoolBufferLength;
  Context->AllocCount = 0;
  Context->NextAllocLoc = (UINTN) Context->PoolBuffer;

  DEBUG ((EFI_D_INFO, "RedirectMemoryServicesSetPool Base:Size 0x%08X:0x%08X\n",
    (UINTN) Context->PoolBuffer,
    (UINTN) Context->PoolBufferLength
    ));
}

/**

  Enable memory allocations from redirected memory pool.

**/
VOID
EFIAPI
RedirectMemoryServicesEnable (
  VOID
  )
{
  CONST EFI_PEI_SERVICES            **PeiServices;
  EFI_PEI_SERVICES                  **WorkPeiServices;
  REDIRECT_CONTEXT                  *Context;

  PeiServices = GetPeiServicesTablePointer ();
  WorkPeiServices = (EFI_PEI_SERVICES **) PeiServices;

  //
  // Get PEI executable shared context structure.
  //
  Context = GetRedirectContext (PeiServices);
  ASSERT (Context != NULL);

  //
  // Save address of current memory allocation services.
  //
  Context->OriginalPeiServices.AllocatePool = (*PeiServices)->AllocatePool;
  Context->OriginalPeiServices.AllocatePages = (*PeiServices)->AllocatePages;

  //
  // Switch memory allocation services to use our routines.
  //
  (*WorkPeiServices)->AllocatePool = RedirectPeiAllocatePool;
  (*WorkPeiServices)->AllocatePages = RedirectPeiAllocatePages;
}

/**

  Disable memory allocations from redirected memory pool.

**/
VOID
EFIAPI
RedirectMemoryServicesDisable (
  VOID
  )
{
  CONST EFI_PEI_SERVICES            **PeiServices;
  EFI_PEI_SERVICES                  **WorkPeiServices;
  REDIRECT_CONTEXT                  *Context;

  PeiServices = GetPeiServicesTablePointer ();
  WorkPeiServices = (EFI_PEI_SERVICES **) PeiServices;

  //
  // Get PEI executable shared context structure.
  //
  Context = GetRedirectContext (PeiServices);
  ASSERT (Context != NULL);

  //
  // Restore address of original memory allocation services.
  //
  (*WorkPeiServices)->AllocatePool = Context->OriginalPeiServices.AllocatePool;
  (*WorkPeiServices)->AllocatePages = Context->OriginalPeiServices.AllocatePages;
}
