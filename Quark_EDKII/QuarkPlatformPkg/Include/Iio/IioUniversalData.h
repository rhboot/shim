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
  IioUniveralData.h
    
Abstract:
  Data format for Universal Data Structure

--*/

#ifndef _IIO_UNIVERSAL_DATA_
#define _IIO_UNIVERSAL_DATA_

#define IIO_UNIVERSAL_DATA_GUID { 0x7FF396A1, 0xEE7D, 0x431E, 0xBA, 0x53, 0x8F, 0xCA, 0x12, 0x7C, 0x44, 0xC0 }
#include "IioRegs.h"
#include "Platform.h"

//--------------------------------------------------------------------------------------//
// Structure definitions for Universal Data Store (UDS)
//--------------------------------------------------------------------------------------//
#define UINT64  unsigned long long

#pragma pack(1)

typedef struct {
    UINT8                   Valid; 
	 UINT8                   SocketID;            // Socket ID of the IIO (0..3)
	 UINT8                   BusBase;
	 UINT8                   BusLimit;
    UINT16                  PciResourceIoBase;
    UINT16                  PciResourceIoLimit;
    UINT32                  IoApicBase;
    UINT32                  IoApicLimit;
    UINT32                  PciResourceMem32Base;
    UINT32                  PciResourceMem32Limit;
    UINT64                  PciResourceMem64Base;
    UINT64                  PciResourceMem64Limit;
    UINT32                  RcbaAddress;
} IIO_RESOURCE_INSTANCE;

typedef struct {
    UINT8                   PfSbspId;  // Socket ID of SBSP
    UINT8                   PfGBusBase;  // Global IIO Bus base
    UINT8                   PfGBusLimit;  // Global IIO Bus limit
    UINT16                  PfGIoBase;  // Global IIO IO base
    UINT16                  PfGIoLimit;  // Global IIO IO limit
    UINT32                  PfGMmiolBase;  // Global Mmiol base
    UINT32                  PfGMmiolLimit;  // Global Mmiol limit
    UINT64                  PfGMmiohBase;  // Global Mmioh Base [43:0]
    UINT64                  PfGMmiohLimit;  // Global Mmioh Limit [43:0]
    UINT32                  MemTsegSize;
    UINT64                  PciExpressBase;
    UINT32                  PciExpressSize;
    UINT8                   Pci64BitResourceAllocation;
    IIO_RESOURCE_INSTANCE   IIO_resource[MAX_NODE];
    UINT8                   numofIIO;
    UINT8                   MaxBusNumber;
} PLATFORM_DATA;

typedef struct {
    PLATFORM_DATA           PlatformData;
} IIO_UDS;
#pragma pack()

#endif
