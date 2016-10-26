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

  QncS3Support.h
    
Abstract:

  This file defines the QNC S3 support Protocol.

--*/
#ifndef _QNC_S3_SUPPORT_PROTOCOL_H_
#define _QNC_S3_SUPPORT_PROTOCOL_H_

//
// Extern the GUID for protocol users.
//
extern EFI_GUID                             gEfiQncS3SupportProtocolGuid;

//
// Forward reference for ANSI C compatibility
//
typedef struct _EFI_QNC_S3_SUPPORT_PROTOCOL EFI_QNC_S3_SUPPORT_PROTOCOL;

typedef enum {
  QncS3ItemTypeInitPcieRootPortDownstream,
  QncS3ItemTypeMax
} EFI_QNC_S3_DISPATCH_ITEM_TYPE;

//
// It's better not to use pointer here because the size of pointer in DXE is 8, but it's 4 in PEI
// plug 4 to ParameterSize in PEIM if you really need it
//
typedef struct {
  UINT32                        Reserved;
} EFI_QNC_S3_PARAMETER_INIT_PCIE_ROOT_PORT_DOWNSTREAM;

typedef union {
  EFI_QNC_S3_PARAMETER_INIT_PCIE_ROOT_PORT_DOWNSTREAM   PcieRootPortData;
} EFI_DISPATCH_CONTEXT_UNION;

typedef struct {
  EFI_QNC_S3_DISPATCH_ITEM_TYPE Type;
  VOID                          *Parameter;
} EFI_QNC_S3_DISPATCH_ITEM;

//
// Member functions
//
typedef
EFI_STATUS
(EFIAPI *EFI_QNC_S3_SUPPORT_SET_S3_DISPATCH_ITEM) (
  IN     EFI_QNC_S3_SUPPORT_PROTOCOL   * This,
  IN     EFI_QNC_S3_DISPATCH_ITEM      * DispatchItem,
  OUT    VOID                         **S3DispatchEntryPoint,
  OUT    VOID                         **Context
  );

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

  EFI_STATUS

--*/

//
// Protocol definition
//
struct _EFI_QNC_S3_SUPPORT_PROTOCOL {
  EFI_QNC_S3_SUPPORT_SET_S3_DISPATCH_ITEM SetDispatchItem;
};

#endif
