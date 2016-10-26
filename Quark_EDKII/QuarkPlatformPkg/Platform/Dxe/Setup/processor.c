/*++

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

  Processor.c

Abstract:

  None.

Revision History

--*/


#include "CommonHeader.h"

#include "SetupPlatform.h"


#define NUMBER_OF_PACKAGES 1

static EFI_EXP_BASE10_DATA mCoreFrequencyList[] = {
  {   0, 0  }, // 0 Means "Auto", also, the first is the default.
  {  (UINT16) -1, 0  }  // End marker
};

static EFI_EXP_BASE10_DATA mFsbFrequencyList[] = {
  {   0, 0  }, // 0 Means "Auto", also, the first is the default.
  {  (UINT16) -1, 0  }  // End marker
};

static CHAR16 *SocketNames[NUMBER_OF_PACKAGES];
static CHAR16 *AssetTags[NUMBER_OF_PACKAGES];

static CHAR16 EmptyString[] = L" ";
static CHAR16 SocketString[] = L"LGA775";


PLATFORM_CPU_FREQUENCY_LIST  PlatformCpuFrequencyList = {
  mCoreFrequencyList,
  mFsbFrequencyList
};

VOID
ProducePlatformCpuData (
  VOID
  )
{
  UINTN                                      Index;

  for (Index = 0; Index < NUMBER_OF_PACKAGES; Index++) {

    //
    // The String Package of a module is registered together with all IFR packages.
    // So we just arbitrarily pick a package GUID that is always installed to get the string.
    //
    AssetTags[Index] = EmptyString;
    SocketNames[Index] = SocketString;
  }

  PcdSet64 (PcdPlatformCpuSocketNames, (UINT64) (UINTN) SocketNames);
  PcdSet64 (PcdPlatformCpuAssetTags, (UINT64) (UINTN) AssetTags);
  PcdSet64 (PcdPlatformCpuFrequencyLists, (UINT64) (UINTN) &PlatformCpuFrequencyList);
}
