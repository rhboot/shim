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

  SecureBootCrypto.h

Abstract:

  Header file for SecureBootCrypto.c.

--*/

#ifndef __SECUREBOOT_HELPER_CRYPTO_HEADER_H_
#define __SECUREBOOT_HELPER_CRYPTO_HEADER_H_

//
//
// PKCS7 Certificate definition.
//
typedef struct {
  WIN_CERTIFICATE Hdr;
  UINT8           CertData[1];
} WIN_CERTIFICATE_EFI_PKCS;

/**
  Retrieves the size, in bytes, of the context buffer required for hash operations.

  @return  The size, in bytes, of the context buffer required for hash operations.

**/
typedef
UINTN
(EFIAPI *HASH_GET_CONTEXT_SIZE)(
  VOID
  );

/**
  Initializes user-supplied memory pointed by HashContext as hash context for
  subsequent use.

  If HashContext is NULL, then ASSERT().

  @param[in, out]  HashContext  Pointer to  Context being initialized.

  @retval TRUE   HASH context initialization succeeded.
  @retval FALSE  HASH context initialization failed.

**/
typedef
BOOLEAN
(EFIAPI *HASH_INIT)(
  IN OUT  VOID  *HashContext
  );


/**
  Performs digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If HashContext is NULL, then ASSERT().

  @param[in, out]  HashContext  Pointer to the MD5 context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength   Length of Data buffer in bytes.

  @retval TRUE     HASH data digest succeeded.
  @retval FALSE    Invalid HASH context. After HashFinal function has been called, the
                   HASH context cannot be reused.

**/
typedef
BOOLEAN
(EFIAPI *HASH_UPDATE)(
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  );

/**
  Completes hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the context cannot be used again.

  If HashContext is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  HashContext  Pointer to the MD5 context
  @param[out]      HashValue    Pointer to a buffer that receives the HASH digest
                                value.

  @retval TRUE   HASH digest computation succeeded.
  @retval FALSE  HASH digest computation failed.

**/
typedef
BOOLEAN
(EFIAPI *HASH_FINAL)(
  IN OUT  VOID   *HashContext,
  OUT     UINT8  *HashValue
  );


//
// Hash Algorithm Table
//
typedef struct {
  //
  // Name for Hash Algorithm.
  //
  CHAR16                   *Name;
  //
  // Digest Length.
  //
  UINTN                    DigestLength;
  //
  // Hash Algorithm OID ASN.1 Value.
  //
  UINT8                    *OidValue;
  //
  // Length of Hash OID Value.
  //
  UINTN                    OidLength;
  //
  // Pointer to Hash GetContentSize function.
  //
  HASH_GET_CONTEXT_SIZE    GetContextSize;
  //
  // Pointer to Hash Init function.
  //
  HASH_INIT                HashInit;
  //
  // Pointer to Hash Update function.
  //
  HASH_UPDATE              HashUpdate;
  //
  // Pointer to Hash Final function.
  //
  HASH_FINAL               HashFinal;
} HASH_TABLE;

extern UINT8 mHashOidValueArray[];
extern HASH_TABLE mHashTable[];

//
// Services functions for export in protocols.
//

EFI_STATUS
EFIAPI
HashMultipleBuffers (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST UINT32                         HashAlg,
  IN OUT UINTN                            *DigestSize,
  OUT UINT8                               *DigestBuf,
  ...
  );

#endif  // __SECUREBOOT_HELPER_CRYPTO_HEADER_H_
