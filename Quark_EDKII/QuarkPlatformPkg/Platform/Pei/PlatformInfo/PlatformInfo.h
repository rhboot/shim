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
  PlatformInfo.h

Abstract:
  Platform Info Driver.

--*/

#ifndef _PLATFORM_INFO_H_
#define _PLATFORM_INFO_H_

#include <PiPei.h>
#include <Ppi/CpuIo.h>
#include <Ppi/PciCfg.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/PlatformHelperLib.h>
#include <Library/PlatformDataLib.h>
#include <Library/MfhLib.h>
#include <Guid/PlatformInfo.h>
#include <Guid/PlatformDataFileNameGuids.h>
#include <IndustryStandard/Pci22.h>
#include <CpuRegs.h>
#include <IntelQNCRegs.h>
#include <Platform.h>
#include <PlatformData.h>

#define EFI_PLATFORMINFO_DRIVER_PRIVATE_SIGNATURE SIGNATURE_32 ('P', 'I', 'N', 'F')

#endif
