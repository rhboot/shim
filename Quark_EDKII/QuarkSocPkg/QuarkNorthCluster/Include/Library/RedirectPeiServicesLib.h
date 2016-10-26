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

  RedirectPeiServicesLib.h

Abstract:

  Header file for RedirectPeiServicesLib.

--*/

#ifndef __REDIRECT_PEI_SERVICES_LIB_H__
#define __REDIRECT_PEI_SERVICES_LIB_H__

/**

  Init RedirectedPeiServices Lib.

**/
VOID
EFIAPI
RedirectServicesInit (
  VOID
  );

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
  );

/**

  Enable memory allocations from redirected memory pool.

**/
VOID
EFIAPI
RedirectMemoryServicesEnable (
  VOID
  );

/**

  Disable memory allocations from redirected memory pool.

**/
VOID
EFIAPI
RedirectMemoryServicesDisable (
  VOID
  );

#endif // #ifndef __REDIRECT_PEI_SERVICES_LIB_H__
