/** @file
  FrontPage routines to handle the callbacks and browser calls

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

#ifndef _UI_BOOT_MENU_H_
#define _UI_BOOT_MENU_H_

#include <Uefi.h>
#include <Guid/MdeModuleHii.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/BootLogo.h>

EFI_HII_HANDLE gStringPackHandle;

#define TITLE_TOKEN_COUNT   1
#define HELP_TOKEN_COUNT    3

typedef struct _BOOT_MENU_SCREEN {
  UINTN        StartCol;
  UINTN        StartRow;
  UINTN        Width;
  UINTN        Height;
} BOOT_MENU_SCREEN; 

typedef struct _BOOT_MENU_SCROLL_BAR_CONTROL {
  BOOLEAN      HasScrollBar;
  UINTN        ItemCountPerScreen;
  UINTN        FirstItem;
  UINTN        LastItem;
} BOOT_MENU_SCROLL_BAR_CONTROL; 

typedef struct _BOOT_MENU_POPUP_DATA {
  EFI_STRING_ID                   TitleToken[TITLE_TOKEN_COUNT]; // Title string ID
  UINTN                           ItemCount;                     // Selectable item count
  EFI_STRING_ID                   *PtrTokens;                    // All of selectable items string ID
  EFI_STRING_ID                   HelpToken[HELP_TOKEN_COUNT];   // All of help string ID
  UINTN                           SelectItem;                    // Current select  item	
  BOOT_MENU_SCREEN                MenuScreen;                    // Boot menu screen information
  BOOT_MENU_SCROLL_BAR_CONTROL    ScrollBarControl;              // Boot menu scroll bar inoformation
} BOOT_MENU_POPUP_DATA;

#endif 

