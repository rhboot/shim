/** @file
  Library implementation for Flash Device Library based on Multiple Flash Support
  library intance.

  BaseLib for PEI & DXE so Memory Allocation routine pointers have to be passed in.

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
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/QuarkBootRomLib.h>

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
  )
{
  BOOLEAN                 (*ValidateModuleCallBack)(QuarkSecurityHeader_t*, RSA2048PublicKey_t*, ScratchMemory_t*);  // bootrom code validate entry function pointer
  UINT8                   MemoryBufferArray[QUARK_CRYPTO_HEAP_SIZE_BYTES];        // Allocate a heap to be used by the BootRom crypto functions
  UINT32                  *ValidateModuleCallBackAddress;
  ScratchMemory_t         *scratchAreaInfo;
  RSA2048PublicKey_t      *OemRsaKeyAddress;
  QuarkSecurityHeader_t   *SignedKeyModule;         // Location in memory of key module to be validated.
  QuarkSecurityHeader_t   *SignedImageBaseAddress;  // Location in memory of image to be validated
  EFI_STATUS              Status;
  BOOLEAN                 RomResult;
  UINT8                   KeyBankNumber;

  ValidateModuleCallBackAddress = NULL;
  ValidateModuleCallBack = NULL;

  //
  // Function requires both or none of memory resource routines.
  //
  if (QAllocPool != NULL && QFreePool == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine SoC Key bank number to use.
  //
  if (KeyBankNumberPtr == NULL) {
    KeyBankNumber = QUARK_DEFAULT_INTEL_KEY_FUSE_BANK;
  } else {
    KeyBankNumber = *KeyBankNumberPtr;
  }

  //
  // Get the address of the OEM RSA Public Key And Authenticate Key if asked.
  //
  if (CallerSignedKeyModule == NULL) {
    SignedKeyModule = (QuarkSecurityHeader_t *) QUARK_KEY_MODULE_BASE_ADDRESS;
  } else {
    SignedKeyModule = CallerSignedKeyModule;
  }
  if (AuthKeyModule) {
    Status = SecurityAuthenticateKeyModule (
               SignedKeyModule,
               KeyBankNumber,
               MemoryBufferArray,
               QUARK_CRYPTO_HEAP_SIZE_BYTES,
               QAllocPool,
               QFreePool
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  OemRsaKeyAddress = (RSA2048PublicKey_t *) (((UINT32)SignedKeyModule->ModuleHeaderSizeBytes) + (UINT32) QUARK_KEY_MODULE_BASE_ADDRESS);

  //
  // Get the address of the image to be authenticated (including the CSH)
  //
  SignedImageBaseAddress = (QuarkSecurityHeader_t *)((UINT32)ImageBaseAddress - PcdGet32 (PcdFvSecurityHeaderSize));

  //
  // Initialise memory to pass as heap to Validate function to all zeros
  //
  ZeroMem (MemoryBufferArray, QUARK_CRYPTO_HEAP_SIZE_BYTES);

  //
  // Init the Scratch Memory Structure.
  //
  scratchAreaInfo = (ScratchMemory_t *)&MemoryBufferArray;
  scratchAreaInfo->HeapStart = (UINT8 *)(scratchAreaInfo + 1);                         // Next address after the structure itself - use pointer++ to figure that address for us.
  scratchAreaInfo->HeapEnd = (UINT8 *)(scratchAreaInfo) + QUARK_CRYPTO_HEAP_SIZE_BYTES;
  scratchAreaInfo->NextFreeMem = scratchAreaInfo->HeapStart;                           // Reset our next free memory pointer to the base address.
  scratchAreaInfo->FatalCode = 0;
  scratchAreaInfo->DebugCode = 0;

  //
  // Validate signed image using Oem Key.
  //
  ValidateModuleCallBackAddress = (UINT32*) QUARK_BOOTROM_VALIDATE_FUNCTION_ENTRYPOINT_ADDRESS;
  ValidateModuleCallBack =  (BOOLEAN(*)(QuarkSecurityHeader_t*, RSA2048PublicKey_t*, ScratchMemory_t*))((UINT32)(*ValidateModuleCallBackAddress)); // actual address here to jump to make sure to get it each time!

  RomResult = ValidateModuleCallBack (
                (QuarkSecurityHeader_t*) SignedImageBaseAddress,
                (RSA2048PublicKey_t*) OemRsaKeyAddress,
                (ScratchMemory_t*) scratchAreaInfo
                );

  DEBUG ((EFI_D_INFO, "SecurityAuthenticateImage RomResult:%d DebugCode:0x%08X: FatalCode:0x%08X\n",
    (UINTN) RomResult,
    (UINTN) scratchAreaInfo->DebugCode,
    (UINTN) scratchAreaInfo->FatalCode
    ));

  if (RomResult) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_SECURITY_VIOLATION;
  }

  return Status;
}

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
  IN QuarkSecurityHeader_t                *CallerSignedKeyModule,
  IN CONST UINT8                          KeyBankNumber,
  IN UINT8                                *CallerMemBuf OPTIONAL,
  IN UINTN                                CallerMemBufSize OPTIONAL,
  IN QUARK_AUTH_ALLOC_POOL                QAllocPool OPTIONAL,
  IN QUARK_AUTH_FREE_POOL                 QFreePool OPTIONAL
  )
{
  EFI_STATUS                        Status;
  UINT8                             *MemoryBuffer;
  ScratchMemory_t                   *scratchAreaInfo;
  UINT8                             *KeyAlloc;
  UINT8                             *BufAlloc;
  VALIDATE_KEY_MODULE               ValidateKeyModule;
  QuarkRomFatalErrorCode            ValidateKeyResult;
  UINT8                             *ModuleData;
  RSA2048PublicKey_t                *ModuleKey;
  UINT8                             *Signature;
  UINTN                             MemBufSize;
  QuarkSecurityHeader_t             *SignedKeyModule;

  if (CallerSignedKeyModule == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  SignedKeyModule = CallerSignedKeyModule;

  //
  // Assume no mem allocations. If != NULL before function exit then call
  // QFreePool with non NULL values.
  //
  KeyAlloc = NULL;
  BufAlloc = NULL;

  //
  // Function requires both or none of memory resource routines.
  //
  if (QAllocPool != NULL && QFreePool == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get Heap to be used by the BootRom crypto functions.
  //
  if (CallerMemBuf == NULL) {
    if (QAllocPool == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    MemBufSize = QUARK_CRYPTO_HEAP_SIZE_BYTES;
    BufAlloc = QAllocPool (MemBufSize);
    if (BufAlloc == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    MemoryBuffer = BufAlloc;
  } else {
    MemoryBuffer = CallerMemBuf;
    MemBufSize = CallerMemBufSize;
  }

  //
  // Initialise memory to pass as heap to Validate function to all zeros
  //
  ZeroMem (MemoryBuffer, MemBufSize);

  //
  // Init the Scratch Memory Structure.
  //
  scratchAreaInfo = (ScratchMemory_t *)MemoryBuffer;
  scratchAreaInfo->HeapStart = (UINT8 *)(scratchAreaInfo + 1);                         // Next address after the structure itself - use pointer++ to figure that address for us.
  scratchAreaInfo->HeapEnd = (UINT8 *)(scratchAreaInfo) + MemBufSize;
  scratchAreaInfo->NextFreeMem = scratchAreaInfo->HeapStart;                           // Reset our next free memory pointer to the base address.
  scratchAreaInfo->FatalCode = 0;
  scratchAreaInfo->DebugCode = 0;

  //
  // Do work from now on if no error detected.
  //
  Status = EFI_SUCCESS;

  if (QAllocPool != NULL && SignedKeyModule->ModuleSizeBytes > 0 && SignedKeyModule->ModuleSizeBytes <= QUARK_KEY_MODULE_MAX_SIZE) {
    KeyAlloc = (UINT8 *) QAllocPool (SignedKeyModule->ModuleSizeBytes);
    if (KeyAlloc != NULL) {
      CopyMem (
        (VOID *) KeyAlloc,
        (VOID *) SignedKeyModule,
        SignedKeyModule->ModuleSizeBytes
        );

      //
      // Override local pointer to allocated signed key module.
      //
      SignedKeyModule = (QuarkSecurityHeader_t *) KeyAlloc;
    } else {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // Get address of ValidateKeyModule routine in rom.
    //
    ValidateKeyModule = *((VALIDATE_KEY_MODULE *) QUARK_BOOTROM_VALIDATE_KEY_ENTRYPOINT_ADDRESS);

    //
    // Determine indivdual params and call rom routine.
    //
    ModuleData = ((UINT8 *) SignedKeyModule) + SignedKeyModule->ModuleHeaderSizeBytes;                    // Data is after indicated size of header.
    ModuleKey = (RSA2048PublicKey_t *) (((UINT8 *)SignedKeyModule)+ sizeof(QuarkSecurityHeader_t)); // Key is right after CSH
    Signature = ((UINT8 *) ModuleKey) + SignedKeyModule->KeySizeBytes;                              // Signature is after indicated size of key.
    ValidateKeyResult = ValidateKeyModule (
                            SignedKeyModule,
                            ModuleData,
                            ModuleKey,
                            Signature,
                            scratchAreaInfo,
                            KeyBankNumber
                            );

    DEBUG ((EFI_D_INFO, "SecurityAuthenticateKeyModule Result:%d DebugCode:0x%08X: FatalCode:0x%08X\n",
      (UINTN) ValidateKeyResult,
      (UINTN) scratchAreaInfo->DebugCode,
      (UINTN) scratchAreaInfo->FatalCode
      ));

    if (ValidateKeyResult != QUARK_ROM_NO_ERROR) {
      Status = EFI_SECURITY_VIOLATION;
    }
  }

  //
  // Free allocated memory before exit.
  //
  if (KeyAlloc != NULL) {
    QFreePool (KeyAlloc);
  }
  if (BufAlloc != NULL) {
    QFreePool (BufAlloc);
  }

  return Status;
}
