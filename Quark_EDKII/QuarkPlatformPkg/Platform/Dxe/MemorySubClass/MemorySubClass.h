/**@file
  Header file for MemorySubClass Driver.
  
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

#ifndef _MEMORY_SUB_CLASS_H
#define _MEMORY_SUB_CLASS_H

//
// The package level header files this module uses
//
#include <FrameworkDxe.h>
//
// The protocols, PPI and GUID definitions for this module
//
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/SmbusHc.h>
#include <Guid/DataHubRecords.h>
#include <Guid/MemoryConfigData.h>
#include <Protocol/HiiDatabase.h>
#include <Guid/MdeModuleHii.h>

//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciLib.h>
#include <Library/QNCAccessLib.h>

#include "QNCAccess.h"



//
// This is the generated header file which includes whatever needs to be exported (strings + IFR)
//

#define EFI_MEMORY_SUBCLASS_DRIVER_GUID \
  { 0xef17cee7, 0x267d, 0x4bfd, { 0xa2, 0x57, 0x4a, 0x6a, 0xb3, 0xee, 0x85, 0x91 }}
  
//
// Prototypes
//
EFI_STATUS
MemorySubClassEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );

#endif
