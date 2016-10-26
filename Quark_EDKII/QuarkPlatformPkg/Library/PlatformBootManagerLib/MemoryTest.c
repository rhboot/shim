/** @file
  Perform the platform memory test

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

#include "BdsPlatform.h"

//
// BDS Platform Functions
//

EFI_HII_HANDLE    mHiiHandle = NULL;

VOID
InitializeStringSupport (
  VOID
  )
{
  mHiiHandle = HiiAddPackages (
                 &gEfiCallerIdGuid,
                 gImageHandle,
                 PlatformBootManagerLibStrings,
                 NULL
                 );
  ASSERT (mHiiHandle != NULL);
}

VOID
PrintBootPrompt (
  VOID
  )
{
  CHAR16          *BootPrompt;

  BootPrompt = HiiGetString (mHiiHandle, STRING_TOKEN (STR_BOOT_PROMPT), NULL);
  if (BootPrompt != NULL) {
    Print (BootPrompt);
    FreePool (BootPrompt);
  }
}

VOID
PrintMfgModePrompt (
  VOID
  )
{
  CHAR16          *MfgPrompt;

  MfgPrompt = HiiGetString (mHiiHandle, STRING_TOKEN (STR_MFG_MODE_PROMPT), NULL);
  if (MfgPrompt != NULL) {
    Print (MfgPrompt);
    FreePool (MfgPrompt);
  }
}

VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16          TimeoutRemain
  )
{
  CHAR16                        *TmpStr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Color;
  UINT16                        Timeout;
  
  Timeout = PcdGet16 (PcdPlatformBootTimeOut);
  //
  // Show progress
  //
  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color,      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);

  TmpStr = HiiGetString (mHiiHandle, STRING_TOKEN (STR_START_BOOT_OPTION), NULL);
  if (TmpStr != NULL) {
    ShowProgress (
      Foreground,
      Background,
      TmpStr,
      Color,
      (Timeout - TimeoutRemain) * 100 / Timeout,
      0
      );
    FreePool (TmpStr);
  }
}

/**
  Perform the memory test base on the memory test intensive level,
  and update the memory resource.

  @param  Level         The memory test intensive level.

  @retval EFI_STATUS    Success test all the system memory and update
                        the memory resource

**/
EFI_STATUS
MemoryTest (
  IN EXTENDMEM_COVERAGE_LEVEL Level
  )
{
  EFI_STATUS                        Status;
  EFI_STATUS                        KeyStatus;
  EFI_STATUS                        InitStatus;
  EFI_STATUS                        ReturnStatus;
  BOOLEAN                           RequireSoftECCInit;
  EFI_GENERIC_MEMORY_TEST_PROTOCOL  *GenMemoryTest;
  UINT64                            TestedMemorySize;
  UINT64                            TotalMemorySize;
  UINTN                             TestPercent;
  UINT64                            PreviousValue;
  BOOLEAN                           ErrorOut;
  BOOLEAN                           TestAbort;
  EFI_INPUT_KEY                     Key;
  CHAR16                            StrPercent[80];
  CHAR16                            *StrTotalMemory;
  CHAR16                            *Pos;
  CHAR16                            *TmpStr;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     Foreground;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     Background;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL     Color;
  BOOLEAN                           IsFirstBoot;
  UINT32                            TempData;

  //
  // Use a DynamicHii type pcd to save the boot status, which is used to
  // control configuration mode, such as FULL/MINIMAL/NO_CHANGES configuration.
  //
  IsFirstBoot = PcdGetBool(PcdBootState);
  if (IsFirstBoot) {
    PcdSetBool(PcdBootState, FALSE);
  }

  ReturnStatus = EFI_SUCCESS;
  ZeroMem (&Key, sizeof (EFI_INPUT_KEY));

  Pos = AllocatePool (128);

  if (Pos == NULL) {
    return ReturnStatus;
  }

  StrTotalMemory    = Pos;

  TestedMemorySize  = 0;
  TotalMemorySize   = 0;
  PreviousValue     = 0;
  ErrorOut          = FALSE;
  TestAbort         = FALSE;
  TestPercent       = 0;

  SetMem (&Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);
  SetMem (&Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);
  SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0xff);

  RequireSoftECCInit = FALSE;

  Status = gBS->LocateProtocol (
                  &gEfiGenericMemTestProtocolGuid,
                  NULL,
                  (VOID **) &GenMemoryTest
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Pos);
    return EFI_SUCCESS;
  }

  InitStatus = GenMemoryTest->MemoryTestInit (
                                GenMemoryTest,
                                Level,
                                &RequireSoftECCInit
                                );
  if (InitStatus == EFI_NO_MEDIA) {
    //
    // The PEI codes also have the relevant memory test code to check the memory,
    // it can select to test some range of the memory or all of them. If PEI code
    // checks all the memory, this BDS memory test will has no not-test memory to
    // do the test, and then the status of EFI_NO_MEDIA will be returned by
    // "MemoryTestInit". So it does not need to test memory again, just return.
    //
    FreePool (Pos);
    return EFI_SUCCESS;
  }
  
  TmpStr = HiiGetString (mHiiHandle, STRING_TOKEN (STR_ESC_TO_SKIP_MEM_TEST), NULL);
    
  if (TmpStr != NULL) {
    PrintXY (10, 10, NULL, NULL, TmpStr);
    FreePool (TmpStr);
  }

  do {
    Status = GenMemoryTest->PerformMemoryTest (
                              GenMemoryTest,
                              &TestedMemorySize,
                              &TotalMemorySize,
                              &ErrorOut,
                              TestAbort
                              );
    if (ErrorOut && (Status == EFI_DEVICE_ERROR)) {
      TmpStr = HiiGetString (mHiiHandle, STRING_TOKEN (STR_SYSTEM_MEM_ERROR), NULL);
      if (TmpStr != NULL) {
        PrintXY (10, 10, NULL, NULL, TmpStr);
        FreePool (TmpStr);
      }

      ASSERT (0);
    }
    
    TempData = (UINT32) DivU64x32 (TotalMemorySize, 16);
    TestPercent = (UINTN) DivU64x32 (
                            DivU64x32 (MultU64x32 (TestedMemorySize, 100), 16),
                            TempData
                            );
    if (TestPercent != PreviousValue) {
      UnicodeValueToString (StrPercent, 0, TestPercent, 0);
      TmpStr = HiiGetString (mHiiHandle, STRING_TOKEN (STR_MEMORY_TEST_PERCENT), NULL);
      if (TmpStr != NULL) {
        //
        // TmpStr size is 64, StrPercent is reserved to 16.
        //
        StrCat (StrPercent, TmpStr);
        PrintXY (10, 10, NULL, NULL, StrPercent);
        FreePool (TmpStr);
      }
      
      TmpStr = HiiGetString (mHiiHandle, STRING_TOKEN (STR_PERFORM_MEM_TEST), NULL);
      if (TmpStr != NULL) {
        ShowProgress (
          Foreground,
          Background,
          TmpStr,
          Color,
          TestPercent,
          (UINTN) PreviousValue
          );
        FreePool (TmpStr);
      }
    }

    PreviousValue = TestPercent;

    KeyStatus     = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (!EFI_ERROR (KeyStatus) && (Key.ScanCode == SCAN_ESC)) {
      if (!RequireSoftECCInit) {
        TmpStr = HiiGetString (mHiiHandle, STRING_TOKEN (STR_PERFORM_MEM_TEST), NULL);
        if (TmpStr != NULL) {
          ShowProgress (
            Foreground,
            Background,
            TmpStr,
            Color,
            100,
            (UINTN) PreviousValue
            );
          FreePool (TmpStr);
        }
          
        PrintXY (10, 10, NULL, NULL, L"100");
        Status = GenMemoryTest->Finished (GenMemoryTest);
        goto Done;
      }

      TestAbort = TRUE;
    }
  } while (Status != EFI_NOT_FOUND);

  Status = GenMemoryTest->Finished (GenMemoryTest);

Done:
  UnicodeValueToString (StrTotalMemory, COMMA_TYPE, TotalMemorySize, 0);
  if (StrTotalMemory[0] == L',') {
    StrTotalMemory++;
  }
    
  TmpStr = HiiGetString (mHiiHandle, STRING_TOKEN (STR_MEM_TEST_COMPLETED), NULL);
  if (TmpStr != NULL) {
    StrCat (StrTotalMemory, TmpStr);
    FreePool (TmpStr);
  }
    
  PrintXY (10, 10, NULL, NULL, StrTotalMemory);
  ShowProgress (
    Foreground,
    Background,
    StrTotalMemory,
    Color,
    100,
    (UINTN) PreviousValue
    );
  FreePool (Pos);


  return ReturnStatus;
}
