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

  SmmRtProtocol.h

Abstract:

  This code abstracts SMM Runtime Protocol  

--*/

#ifndef _SMM_RT_PROTOCOL_H_
#define _SMM_RT_PROTOCOL_H_

#include <Protocol/SmmBase2.h>

#define EFI_SMM_RUNTIME_PROTOCOL_GUID \
  { \
    0xa56897a1, 0xa77f, 0x4600, 0x84, 0xdb, 0x22, 0xb0, 0xa8, 0x1, 0xfa, 0x9a \
  }

typedef enum {
  EfiSmmRtIdlePhase,
  EfiSmmRtArgumentPhase,
  EfiSmmDataFetchPhase,
  EfiSmmCommandExecutePhase,
  EfiSmmDataEmitPhase,
  EfiSmmStatusPhase
} EFI_SMM_RUNTIME_PHASE;

typedef struct {
  UINTN SimpleData[(2048 / 4)];
} SMM_RUNTIME_UINTN_DATA;

typedef struct {
  UINTN SimpleData[2048];
} SMM_RUNTIME_UINT8_DATA;

typedef union {
  SMM_RUNTIME_UINTN_DATA  UintnData;
  SMM_RUNTIME_UINT8_DATA  Uint8Data;
} SMM_RUNTIME_DATA;

typedef struct {
  EFI_SMM_RUNTIME_PHASE SmmRuntimePhase;
  EFI_HANDLE            SmmRuntimeCallHandle;
  BOOLEAN               ArgIsPointer;
  UINT64                Argument;
  SMM_RUNTIME_DATA      Data;
  UINTN                 TotalDataSize;
  UINTN                 CurrentDataSize;
} SMM_RUNTIME_COMMUNICATION_DATA;

typedef struct {
  EFI_GUID                        HeaderGuid;
  UINTN                           MessageLength;
  SMM_RUNTIME_COMMUNICATION_DATA  PrivateData;
} SMM_RUNTIME_COMMUNICATION_STRUCTURE;

typedef
EFI_STATUS
(EFIAPI *EFI_ENABLE_PROTOCOL_NOTIFY_EVENT) (
  IN EFI_EVENT_NOTIFY               CallbackFunction,
  IN VOID                           *Context,
  IN EFI_GUID                       * ProtocolGuid,
  OUT EFI_EVENT                     * Event
  );

typedef
EFI_STATUS
(EFIAPI *EFI_DISABLE_PROTOCOL_NOTIFY_EVENT) (
  IN EFI_EVENT                      Event
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_LOCATE_PROTOCOL_HANDLES) (
  EFI_GUID                          * Protocol,
  EFI_HANDLE                        **Handles,
  UINTN                             *HandlesCount
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SMM_HANDLE_PROTOCOL) (
  IN EFI_HANDLE                     Handle,
  IN EFI_GUID                       * Protocol,
  OUT VOID                          **Interface
  );

typedef
EFI_STATUS
(EFIAPI *EFI_GET_VENDOR_CONFIG_TABLE) (
  IN EFI_GUID                       * Guid,
  OUT VOID                          **Table
  );

typedef
VOID
(EFIAPI *EFI_SMM_RUNTIME_CALLBACK) (
  IN VOID                          *Context,
  IN EFI_SMM_SYSTEM_TABLE2         * Smst,
  IN VOID                          *ChildRuntimeBuffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_REGISTER_SMM_RUNTIME_CHILD) (
  IN EFI_SMM_RUNTIME_CALLBACK             SmmRuntimeCallback,
  IN VOID                                 *Context,
  OUT EFI_HANDLE                          * SmmRuntimeCallHandle
  );

typedef
EFI_STATUS
(EFIAPI *EFI_UNREGISTER_SMM_RUNTIME_CHILD) (
  IN EFI_SMM_RUNTIME_CALLBACK       SmmRuntimeCallback
  );

//
// SMM RUNTIME PROTOCOL
//
typedef struct {
  EFI_LOCATE_PROTOCOL                 LocateProtocol;
  EFI_INSTALL_PROTOCOL_INTERFACE      InstallProtocolInterface;
  EFI_REINSTALL_PROTOCOL_INTERFACE    ReinstallProtocolInterface;
  EFI_UNINSTALL_PROTOCOL_INTERFACE    UninstallProtocolInterface;
  EFI_SIGNAL_EVENT                    SignalProtocol;
  EFI_ENABLE_PROTOCOL_NOTIFY_EVENT    EnableProtocolNotify;
  EFI_DISABLE_PROTOCOL_NOTIFY_EVENT   DisableProtocolNotify;
  EFI_SMM_LOCATE_PROTOCOL_HANDLES     LocateProtocolHandles;
  EFI_SMM_HANDLE_PROTOCOL             HandleProtocol;
  EFI_INSTALL_CONFIGURATION_TABLE     InstallVendorConfigTable;
  EFI_GET_VENDOR_CONFIG_TABLE         GetVendorConfigTable;
  EFI_REGISTER_SMM_RUNTIME_CHILD      RegisterSmmRuntimeChild;
  EFI_UNREGISTER_SMM_RUNTIME_CHILD    UnRegisterSmmRuntimeChild;
  EFI_RUNTIME_SERVICES                SmmRuntime;
  SMM_RUNTIME_COMMUNICATION_STRUCTURE *ChildRuntimeBuffer;
} EFI_SMM_RUNTIME_PROTOCOL;

extern EFI_GUID gEfiSmmRuntimeProtocolGuid;

#endif
