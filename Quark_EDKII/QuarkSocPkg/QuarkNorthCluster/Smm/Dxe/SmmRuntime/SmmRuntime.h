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

  SmmRuntime.h

Abstract:

  Implementation specific SMM Runtime stuff

--*/

#ifndef _SMM_RUNTIME_H_
#define _SMM_RUNTIME_H_

#include <PiDxe.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>

#include <Protocol/SmmBase2.h>
/****************************************************
**                                                 **
** EST override - begins here                      **
**                                                 **
****************************************************/
#include <Protocol/SmmAccess2.h>
/*+++++++++++++++++++++++++++++++++++++++++++++++++++
++                                                 ++
++ EST override - ends here                        ++
++                                                 ++
+++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include <Protocol/LoadedImage.h>
#include <Protocol/SmmRtProtocol.h>
#include <Protocol/SmmCommunication.h>

#define MAX_SMM_PROTOCOL    100
#define MAX_CALLBACK        100
#define MAX_HANDLES         100
#define MAX_CONFIG_TABLE    100
#define MAX_SM_RT_CALLBACK  100

typedef struct {
  BOOLEAN             Valid;
  EFI_GUID            ProtocolGuid;
  EFI_INTERFACE_TYPE  InterfaceType;
  VOID                *Protocol;
  EFI_HANDLE          Handle;
} EFI_SMM_PROTO_SERVICES;

typedef struct {
  BOOLEAN           Valid;
  VOID              *Context;
  EFI_GUID          ProtocolGuid;
  EFI_EVENT_NOTIFY  CallbackFunction;
} EFI_SMM_CALLBACK_SERVICES;

typedef struct {
  VOID                      *Context;
  EFI_SMM_RUNTIME_CALLBACK  CallbackFunction;
} EFI_SMM_RT_CALLBACK_SERVICES;

typedef struct {
  UINTN                         Signature;
  UINTN                         CurrentIndex;
  EFI_SMM_PROTO_SERVICES        Services[MAX_SMM_PROTOCOL];
  EFI_SMM_CALLBACK_SERVICES     Callback[MAX_CALLBACK];
  EFI_SMM_RT_CALLBACK_SERVICES  RtCallback[MAX_SM_RT_CALLBACK];
  EFI_HANDLE                    HandleBuffer[MAX_HANDLES];
  EFI_CONFIGURATION_TABLE       ConfigTable[MAX_CONFIG_TABLE];
  EFI_SMM_RUNTIME_PROTOCOL      SmmRtServices;
  EFI_SMM_HANDLER_ENTRY_POINT2  CallbackEntryPoint;
} EFI_SMM_RT_GLOBAL;

//
// BMC Elog Instance signature
//
#define SMM_RT_SIGNATURE                  SIGNATURE_32 ('s', 'm', 'r', 't')

#define INSTANCE_FROM_EFI_SMM_RT_THIS(a)  CR (a, EFI_SMM_RT_GLOBAL, SmmRtServices, SMM_RT_SIGNATURE)

#endif
