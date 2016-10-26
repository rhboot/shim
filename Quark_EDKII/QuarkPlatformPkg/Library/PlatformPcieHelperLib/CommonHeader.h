/**@file
  Common header file shared by all source files in this component.

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

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/QNCAccessLib.h>
#include <Library/IntelQNCLib.h>
#include <QNCAccessLib.h>
#include <IntelQNCRegs.h>
#include <IntelQNCConfig.h>
#include <Pcal9555.h>
#include <Platform.h>
#include <PlatformBoards.h>

#include <Library/PlatformPcieHelperLib.h>

//
// Routines shared between souce modules in this component.
//

VOID
EFIAPI
PlatformPcieErratas (
  VOID
  );

EFI_STATUS
EFIAPI
SocUnitEarlyInitialisation (
  VOID
  );

EFI_STATUS
EFIAPI
SocUnitReleasePcieControllerPreWaitPllLock (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

EFI_STATUS
EFIAPI
SocUnitReleasePcieControllerPostPllLock (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

#endif
