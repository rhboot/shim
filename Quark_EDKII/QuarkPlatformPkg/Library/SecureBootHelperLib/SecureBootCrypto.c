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

  SecureBootCrypto.c

Abstract:

  Secure boot crypto. services for Secure Boot Helper Protocol.

--*/

#include "CommonHeader.h"

//
// OID ASN.1 Value for Hash Algorithms.
//
UINT8 mHashOidValueArray[] = {
  0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05,         // OBJ_md5.
  0x2B, 0x0E, 0x03, 0x02, 0x1A,                           // OBJ_sha1.
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04,   // OBJ_sha224.
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,   // OBJ_sha256.
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02,   // OBJ_sha384.
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03,   // OBJ_sha512.
  };

HASH_TABLE mHashTable[] = {
  { L"SHA1",   20, &mHashOidValueArray[8],  5, Sha1GetContextSize,  Sha1Init,   Sha1Update,    Sha1Final  },
  { L"SHA224", 28, &mHashOidValueArray[13], 9, NULL,                NULL,       NULL,          NULL       },
  { L"SHA256", 32, &mHashOidValueArray[22], 9, Sha256GetContextSize,Sha256Init, Sha256Update,  Sha256Final},
  { L"SHA384", 48, &mHashOidValueArray[31], 9, NULL,                NULL,       NULL,          NULL       },
  { L"SHA512", 64, &mHashOidValueArray[40], 9, NULL,                NULL,       NULL,          NULL       }
};

/**
  Caculate hash of multiple buffers using the specified Hash algorithm.

  @param[in]       This          Pointer to located SECUREBOOT_HELPER_PROTOCOL.
  @param[in]       HashAlg       Hash algorithm type see HASHALG_"XXX" defs.
  @param[in, out]  DigestSize    Size of Digest, on out set to bytes used in Digest.
  @param[out]      Digest        Hash digest written to this buffer.
  @param          ...            Pairs of parmas, Buffer Base pointer (A NULL
                                 terminates the list) followed by buffer len.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER DigestBuf == NULL or DigestSize == NULL.
  @retval EFI_BUFFER_TOO_SMALL  DigestBuf buffer is too small.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory to complete operation.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.
  @retval EFI_UNSUPPORTED       The specified algorithm is not supported.

  **/
EFI_STATUS
EFIAPI
HashMultipleBuffers (
  IN CONST SECUREBOOT_HELPER_PROTOCOL     *This,
  IN CONST UINT32                         HashAlg,
  IN OUT UINTN                            *DigestSize,
  OUT UINT8                               *DigestBuf,
  ...
  )
{
  BOOLEAN                           Result;
  EFI_GUID                          CertType;
  VOID                              *HashCtx;
  UINTN                             CtxSize;
  UINT8                             *HashBase;
  UINTN                             HashSize;
  VA_LIST                           Args;
  UINTN                             SumOfBytesHashed;
  SECUREBOOT_HELPER_PRIVATE_DATA    *PrivateData;

  //
  // Verify This pointer.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  PrivateData = SECUREBOOT_HELPER_PRIVATE_FROM_SBHP (This);
  if (PrivateData->Signature != SECUREBOOT_HELPER_PRIVATE_DATA_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check params.
  //
  if (DigestBuf == NULL || DigestSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  ZeroMem (DigestBuf, *DigestSize);
  if (HashAlg == HASHALG_SHA1) {
    if (*DigestSize < SHA1_DIGEST_SIZE) {
      return EFI_BUFFER_TOO_SMALL;
    }
    *DigestSize       = SHA1_DIGEST_SIZE;
    CertType         = gEfiCertSha1Guid;
  } else if (HashAlg == HASHALG_SHA256) {
    if (*DigestSize < SHA256_DIGEST_SIZE) {
      return EFI_BUFFER_TOO_SMALL;
    }
    *DigestSize       = SHA256_DIGEST_SIZE;
    CertType          = gEfiCertSha256Guid;
  } else {
    return EFI_UNSUPPORTED;
  }

  CtxSize   = mHashTable[HashAlg].GetContextSize();

  HashCtx = AllocatePool (CtxSize);
  if (HashCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize a SHA hash context.
  //
  Result = mHashTable[HashAlg].HashInit(HashCtx);

  VA_START (Args, DigestBuf);

  SumOfBytesHashed = 0;
  while (Result == TRUE) {
    //
    // If HashBase is NULL, then it's the end of the list.
    //
    HashBase = VA_ARG (Args, UINT8 *);
    if (HashBase == NULL) {
      break;
    }
    HashSize = VA_ARG (Args, UINTN);
    Result  = mHashTable[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
    if (Result) {
      SumOfBytesHashed += HashSize;
    }
  }
  VA_END (Args);

  //
  // Finalize result.
  //
  if (SumOfBytesHashed > 0) {
    Result = mHashTable[HashAlg].HashFinal(HashCtx, DigestBuf);
  }
  else {
    Result = FALSE;
  }

  //
  // Free used resources.
  //
  if (HashCtx != NULL) {
    FreePool (HashCtx);
  }

  return Result ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

