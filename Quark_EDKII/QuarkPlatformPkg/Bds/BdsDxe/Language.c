/** @file
  Language settings

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

#include "Bds.h"
#define ISO_639_2_ENTRY_SIZE 3

/**
  Determine the current language that will be used
  based on language related EFI Variables.

  @param LangCodesSettingRequired - If required to set LangCode variable

**/
VOID
InitializeLanguage (
  BOOLEAN LangCodesSettingRequired
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  CHAR8       *Lang;
  CHAR8       LangCode[ISO_639_2_ENTRY_SIZE + 1];
  CHAR8       *LangCodes;
  CHAR8       *PlatformLang;
  CHAR8       *PlatformLangCodes;
  UINTN       Index;
  BOOLEAN     Invalid;

  LangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultLangCodes);
  if (LangCodesSettingRequired) {
    if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
      //
      // UEFI 2.1 depricated this variable so we support turning it off
      //
      Status = gRT->SetVariable (
                      L"LangCodes",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      AsciiStrSize (LangCodes),
                      LangCodes
                      );
    }


    PlatformLangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes);
    Status = gRT->SetVariable (
                    L"PlatformLangCodes",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (PlatformLangCodes),
                    PlatformLangCodes
                    );
  }

  if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
    //
    // UEFI 2.1 depricated this variable so we support turning it off
    //

    //
    // Find current LangCode from Lang NV Variable
    //
    Size = ISO_639_2_ENTRY_SIZE + 1;
    Status = gRT->GetVariable (
                    L"Lang",
                    &gEfiGlobalVariableGuid,
                    NULL,
                    &Size,
                    &LangCode
                    );
    if (!EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
      for (Index = 0; LangCodes[Index] != 0; Index += ISO_639_2_ENTRY_SIZE) {
        if (CompareMem (&LangCodes[Index], LangCode, ISO_639_2_ENTRY_SIZE) == 0) {
          Status = EFI_SUCCESS;
          break;
        }
      }
    }

    //
    // If we cannot get language code from Lang variable,
    // or LangCode cannot be found from language table,
    // set the mDefaultLangCode to Lang variable.
    //
    if (EFI_ERROR (Status)) {
      Lang = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultLang);
      Status = gRT->SetVariable (
                      L"Lang",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      ISO_639_2_ENTRY_SIZE + 1,
                      Lang
                      );
    }
  }

  Invalid = FALSE;
  PlatformLang = EfiBootManagerGetVariableAndSize (L"PlatformLang", &gEfiGlobalVariableGuid, &Size);
  if (PlatformLang != NULL) {
    //
    // Check Current PlatformLang value against PlatformLangCode. Need a library that is TBD
    // Set Invalid based on state of PlatformLang.
    //

    FreePool (PlatformLang);
  } else {
    // No valid variable is set
    Invalid = TRUE;
  }

  if (Invalid) {
    PlatformLang = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLang);
    Status = gRT->SetVariable (
                    L"PlatformLang",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (PlatformLang),
                    PlatformLang
                    );
  }
}
