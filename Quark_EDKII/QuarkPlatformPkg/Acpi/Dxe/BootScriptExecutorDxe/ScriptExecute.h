/** @file
  The header file for Boot Script Executer module.
  
  This driver is dispatched by Dxe core and the driver will reload itself to ACPI NVS memory 
  in the entry point. The functionality is to interpret and restore the S3 boot script 
  
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
#ifndef _SCRIPT_EXECUTE_H_
#define _SCRIPT_EXECUTE_H_

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/PeCoffLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/LockBoxLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/QNCAccessLib.h>

#include <Guid/AcpiS3Context.h>
#include <Guid/BootScriptExecutorVariable.h>
#include <Guid/EventGroup.h>
#include <IndustryStandard/Acpi.h>

/**
  a ASM function to transfer control to OS.
  
  @param  S3WakingVector  The S3 waking up vector saved in ACPI Facs table
  @param  AcpiLowMemoryBase a buffer under 1M which could be used during the transfer             
**/
VOID
AsmTransferControl (
  IN   UINT32           S3WakingVector,
  IN   UINT32           AcpiLowMemoryBase
  );

VOID
SetIdtEntry ( 
  IN ACPI_S3_CONTEXT     *AcpiS3Context
  );

/**
  Platform specific mechanism to transfer control to 16bit OS waking vector

  @param[in] AcpiWakingVector    The 16bit OS waking vector
  @param[in] AcpiLowMemoryBase   A buffer under 1M which could be used during the transfer    

**/
VOID
PlatformTransferControl16 (
  IN UINT32       AcpiWakingVector,
  IN UINT32       AcpiLowMemoryBase
  );

#endif //_SCRIPT_EXECUTE_H_
