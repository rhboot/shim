/** @file
  Definition of Pei Core Structures and Services
  
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

#ifndef _QUARK_BOOT_ROM_LIB_H_
#define _QUARK_BOOT_ROM_LIB_H_

#define QUARK_DEFAULT_INTEL_KEY_FUSE_BANK                   0             // There are 3 fuse banks - we use 0 as a default.
#define QUARK_KEY_MODULE_MAX_SIZE                           0x00008000    // 32K
#define QUARK_KEY_MODULE_BASE_ADDRESS                       0xFFFD8000
#define QUARK_CRYPTO_HEAP_SIZE_BYTES                        0x1900
#define QUARK_BOOTROM_VALIDATE_FUNCTION_ENTRYPOINT_ADDRESS  0xFFFFFFE0
#define QUARK_BOOTROM_VALIDATE_KEY_ENTRYPOINT_ADDRESS       0xFFFE1D3A
#define QUARK_CSH_IDENTIFIER                                SIGNATURE_32 ('H', 'S', 'C', '_')
#define QUARK_RSA_2048_MODULUS_SIZE_BYTES                   256           // 256 Bytes Modulus size = 2048 bits.
#define QUARK_RSA_2048_EXPONENT_SIZE_BYTES                  4

//
// Quark ROM error type and error values.
//
typedef UINT32                                              QuarkRomFatalErrorCode;
#define QUARK_ROM_NO_ERROR                                  0
#define QUARK_ROM_FATAL_NO_VALID_MODULES                    1
#define QUARK_ROM_FATAL_RMU_DMA_TIMEOUT                     2
#define QUARK_ROM_FATAL_MEM_TEST_FAIL                       3
#define QUARK_ROM_FATAL_ACPI_TIMER_ERROR                    4
#define QUARK_ROM_FATAL_UNEX_RETURN_FROM_RUNTIME            5
#define QUARK_ROM_FATAL_ERROR_DMA_TIMEOUT                   6
#define QUARK_ROM_FATAL_OUT_OF_BOUNDS_MODULE_ENTRY          7
#define QUARK_ROM_FATAL_MODULE_SIZE_EXCEEDS_MEMORY          8
#define QUARK_ROM_FATAL_KEY_MODULE_FUSE_COMPARE_FAIL        9
#define QUARK_ROM_FATAL_KEY_MODULE_VALIDATION_FAIL          10
#define QUARK_ROM_FATAL_STACK_CORRUPT                       11
#define QUARK_ROM_FATAL_INVALID_KEYBANK_NUM                 12

//
// Rom lib has one .inf with MODULE_TYPE=BASE (for inclusion in PEI or DXE)
// so Mem Alloc Routines passed to its entry points.
//
typedef
VOID *
(EFIAPI *QUARK_AUTH_ALLOC_POOL) (
  IN UINTN                                AllocationSize
  );

typedef
VOID
(EFIAPI *QUARK_AUTH_FREE_POOL) (
  IN VOID                                 *Buffer
  );

typedef struct QuarkSecurityHeader_t
{
  UINT32  CSH_Identifier;             // Unique identifier to sanity check signed module presence
  UINT32  Version;                    // Current version of CSH used. Should be one for Quark A0.
  UINT32  ModuleSizeBytes;            // Size of the entire module including the module header and payload
  UINT32  SecurityVersionNumberIndex; // Index of SVN to use for validation of signed module
  UINT32  SecurityVersionNumber;      // Used to prevent against roll back of modules
  UINT32  RsvdModuleID;               // Currently unused for Clanton. Maintaining for future use
  UINT32  RsvdModuleVendor;           // Vendor Identifier. For Intel products value is 0x00008086
  UINT32  RsvdDate;                   // BCD representation of build date as yyyymmdd, where yyyy=4 digit year, mm=1-12, dd=1-31
  UINT32  ModuleHeaderSizeBytes;      // Total length of the header including including any padding optionally added by the signing tool.
  UINT32  HashAlgorithm;              // What Hash is used in the module signing.
  UINT32  CryptoAlgorithm;            // What Crypto is used in the module signing.
  UINT32  KeySizeBytes;               // Total length of the key data including including any padding optionally added by the signing tool.
  UINT32  SignatureSizeBytes;         // Total length of the signature including including any padding optionally added by the signing tool.
  UINT32  RsvdNextHeaderPointer;      // 32-bit pointer to the next Secure Boot Module in the chain, if there is a next header.
  UINT32  Reserved[8/sizeof(UINT32)]; // Bytes reserved for future use, padding structure to required size.
} QuarkSecurityHeader_t;

typedef struct RSA2048PublicKey_t
{
  UINT32  ModulusSizeBytes;           // Size of the RSA public key modulus
  UINT32  ExponentSizeBytes;          // Size of the RSA public key exponent
  UINT32  Modulus[QUARK_RSA_2048_MODULUS_SIZE_BYTES/sizeof(UINT32)];// Key Modulus, 256 bytes = 2048 bits
  UINT32  Exponent[QUARK_RSA_2048_EXPONENT_SIZE_BYTES/sizeof(UINT32)]; // Exponent Key, can be up to 4 bytes in our case.
} RSA2048PublicKey_t;

typedef struct ScratchMemory_t
{
  UINT8   *HeapStart;                 // Address of memory from which we can start to allocate.
  UINT8   *HeapEnd;                   // Address of final byte of memory we can allocate up to.
  UINT8   *NextFreeMem;               // Pointer to the next free address in the heap.
  UINT32  DebugCode;                  // A progress code, updated as we go along.
  UINT32  FatalCode;                  // An indicator of why we failed to boot.
}ScratchMemory_t;

/** Validate key module routine.

  Checks the SHA256 digest of the Oem Key in a module header against
  that of the key held in fuse bank and then uses that key to validate
  key module.

  @param[in]       Header         A pointer to a header structure of a key
                                  module in memory to be validated..
  @param[in]       Data           Pointer to the signed data associated with
                                  this module.
  @param[in]       ModuleKey      The key used to sign the data.
  @param[in]       Signature      The signature of the data.
  @param[in]       MemoryBuffer   Pointer to crypto heap management structure.
  @param[in]       KeyBankNumber  Which fuse bank are we pulling the Intel key from.

  @retval  QUARK_ROM_NO_ERROR                            If Key Module Validates 
                                                         Successfully.
  @retval  QUARK_ROM_FATAL_KEY_MODULE_FUSE_COMPARE_FAIL  if the Digest of the Intel Key
                                                         in the Header does not match
                                                         that stored in the fuse.
  @retval  QUARK_ROM_FATAL_KEY_MODULE_VALIDATION_FAIL    if the RSA signature
                                                         is not successfully validated.
  @retval  QUARK_ROM_FATAL_INVALID_KEYBANK_NUM           If key bank number too large.
  @return  Other                                         Unexpected error.

**/
typedef
QuarkRomFatalErrorCode
(*VALIDATE_KEY_MODULE) (
  const QuarkSecurityHeader_t * const Header,
  const UINT8 * const Data,
  const RSA2048PublicKey_t * const ModuleKey,
  const UINT8 * const Signature,
  ScratchMemory_t * const MemoryBuffer,
  const UINT8 KeyBankNumber
  );

/**
  Security library to authenticate a signed image.

  @param[in]       ImageBaseAddress       Pointer to the current image under
                                          consideration
  @param[in]       CallerSignedKeyModule  Pointer to signed key module to
                                          validate image against. If NULL use
                                          default.
  @param[in]       AuthKeyModule          If TRUE authenticate key module
                                          as well as image.
  @param[in]       KeyBankNumberPtr       Pointer to key bank number. If NULL
                                          use default bank.
  @param[in]       QAllocPool             Pointer allocate pool routine for
                                          UEFI emviroment.
  @param[in]       QFreePool              Pointer free pool routine for
                                          UEFI emviroment.

  @retval EFI_SUCCESS             Image is legal
  @retval EFI_SECURITY_VIOLATION  Image is NOT legal.
  @retval EFI_INVALID_PARAMETER   Param options required QAllocPool
                                  and QFreePool to be provided.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to complete
                                  operation.

**/
EFI_STATUS
EFIAPI
SecurityAuthenticateImage (
  IN VOID                                 *ImageBaseAddress,
  IN QuarkSecurityHeader_t                *CallerSignedKeyModule OPTIONAL,
  IN CONST BOOLEAN                        AuthKeyModule,
  IN UINT8                                *KeyBankNumberPtr OPTIONAL,
  IN QUARK_AUTH_ALLOC_POOL                QAllocPool OPTIONAL,
  IN QUARK_AUTH_FREE_POOL                 QFreePool OPTIONAL
  );

/**
  Security library to authenticate a signed key module.

  @param[in]       SignedKetModule  Pointer to key module to authenticate.
  @param[in]       KeyBankNumber    Key bank number to use.
  @param[in]       CallerMemBuf     Heap to be used by the BootRom crypto
                                    functions. If == NULL alloc a heap.
  @param[in]       QAllocPool       Pointer allocate pool routine for
                                    UEFI emviroment.
  @param[in]       QFreePool        Pointer free pool routine for
                                    UEFI emviroment.

  @retval EFI_SUCCESS             Key module is legal
  @retval EFI_SECURITY_VIOLATION  Key module is NOT legal.
  @retval EFI_INVALID_PARAMETER   Param options required QAllocPool
                                  and QFreePool to be provided.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to complete
                                  operation.

**/
EFI_STATUS
EFIAPI
SecurityAuthenticateKeyModule (
  IN QuarkSecurityHeader_t                *SignedKeyModule,
  IN CONST UINT8                          KeyBankNumber,
  IN UINT8                                *CallerMemBuf OPTIONAL,
  IN UINTN                                CallerMemBufSize OPTIONAL,
  IN QUARK_AUTH_ALLOC_POOL                QAllocPool OPTIONAL,
  IN QUARK_AUTH_FREE_POOL                 QFreePool OPTIONAL
  );

#endif
