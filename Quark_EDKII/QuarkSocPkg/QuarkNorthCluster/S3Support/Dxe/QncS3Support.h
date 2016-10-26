/**@file
  Header file for QNC S3 Support driver

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

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

#ifndef _QNC_S3_SUPPORT_H_
#define _QNC_S3_SUPPORT_H_

//
// External include files do NOT need to be explicitly specified in real EDKII
// environment
//
//
// Driver Consumed Protocol Prototypes
//
#include <Protocol/FirmwareVolume2.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/BaseMemoryLib.h> 
#include <Library/PeCoffLib.h>
#include <Library/LockBoxLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
//
// Driver Produced Protocol Prototypes
//
#include <Protocol/LoadedImage.h>
#include <Protocol/QncS3Support.h>
#include <Protocol/ExitPmAuth.h>

#include <Library/CacheMaintenanceLib.h>
#include <Library/IntelQNCLib.h>
//
// Define the header of the context region.
//
typedef struct {
  UINT32                      MaxContexts;
  UINT32                      StorePosition;
  EFI_DISPATCH_CONTEXT_UNION  Contexts[1];
} QNC_S3_PARAMETER_HEADER;
//
// Function prototypes
//
EFI_STATUS
EFIAPI
QncS3SetDispatchItem (
  IN     EFI_QNC_S3_SUPPORT_PROTOCOL   *This,
  IN     EFI_QNC_S3_DISPATCH_ITEM      *DispatchItem,
  OUT    VOID                          **S3DispatchEntryPoint,
  OUT    VOID                          **Context
  )
/*++

Routine Description:

  Set an item to be dispatched at S3 resume time. At the same time, the entry point
  of the QNC S3 support image is returned to be used in subsequent boot script save
  call

Arguments:

  This                    - Pointer to the protocol instance.
  DispatchItem            - The item to be dispatched.
  S3DispatchEntryPoint    - The entry point of the QNC S3 support image.

Returns:

  EFI_STATUS              - Successfully completed.
  EFI_OUT_OF_RESOURCES    - Out of resources.

--*/
;

EFI_STATUS
LoadQncS3Image (
  IN  EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Load the QNC S3 Image into Efi Reserved Memory below 4G.

Arguments:

  ImageEntryPoint     the ImageEntryPoint after success loading

Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
QncS3InitPcieRootPortDownstream (
  IN EFI_HANDLE ImageHandle,
  IN VOID       *Context
  );

EFI_STATUS
EFIAPI
QncS3BootEvent (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  );


#endif
