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
#include <Guid/QuarkCapsuleGuid.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/SerialPortLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CapsuleLib.h>
#include <Library/PlatformDataLib.h>
#include <Library/IntelQNCLib.h>
#include <Platform.h>
#include <PlatformBoards.h>
#include <Pcal9555.h>
#include "FlashLayout.h"
#include <CommonIncludes.h>
#include <QNCAccess.h>
#include <QNCAccessLib.h>
#include <IohAccess.h>

#include <Library/PlatformHelperLib.h>

//
// Routines shared between souce modules in this component.
//

EFI_STATUS
WriteFirstFreeSpiProtect (
  IN CONST UINT32                         PchRootComplexBar,
  IN CONST UINT32                         DirectValue,
  IN CONST UINT32                         BaseAddress,
  IN CONST UINT32                         Length,
  OUT UINT32                              *OffsetPtr
  );

VOID
Pcal9555SetPortRegBit (
  IN CONST UINT32                         Pcal9555SlaveAddr,
  IN CONST UINT32                         GpioNum,
  IN CONST UINT8                          RegBase,
  IN CONST BOOLEAN                        LogicOne
  );

#endif
