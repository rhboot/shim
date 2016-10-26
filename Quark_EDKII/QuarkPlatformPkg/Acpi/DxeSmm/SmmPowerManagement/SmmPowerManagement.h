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

  SmmPowerManagement.h

Abstract:

  Header file for  QNC Smm Power Management driver

Revision History

++*/

#ifndef _SMM_POWER_MANAGEMENT_H_
#define _SMM_POWER_MANAGEMENT_H_

#include <PiSmm.h>
#include <AcpiCpuData.h>
#include <QuarkPlatformDxe.h>
#include <IntelQNCDxe.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/SmmCpu.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/GlobalNvsArea.h>
#include <Protocol/AcpiSupport.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/MpService.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/SocketLga775Lib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CpuConfigLib.h>

#include <IndustryStandard/Acpi.h>


#include "Ppm.h"

//
// Module global variable
//
extern EFI_SMM_CPU_PROTOCOL                    *mSmmCpu;
extern EFI_GLOBAL_NVS_AREA                     *mGlobalNvsAreaPtr;
extern EFI_ACPI_SUPPORT_PROTOCOL               *mAcpiSupport;
extern EFI_MP_SERVICES_PROTOCOL                *mMpService; 

//
// Function prototypes
//

#endif
