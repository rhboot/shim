/** @file
  Include file for record cache subclass data with Smbios protocol.

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

#ifndef _CACHE_H_
#define _CACHE_H_

#include "MpCommon.h"

//
// Bit field definitions for return registers of CPUID EAX = 4
//
// EAX
#define CPU_CACHE_TYPE_MASK                0x1F
#define CPU_CACHE_LEVEL_MASK               0xE0
#define CPU_CACHE_LEVEL_SHIFT              5
// EBX
#define CPU_CACHE_LINESIZE_MASK            0xFFF
#define CPU_CACHE_PARTITIONS_MASK          0x3FF000
#define CPU_CACHE_PARTITIONS_SHIFT         12
#define CPU_CACHE_WAYS_MASK                0xFFC00000
#define CPU_CACHE_WAYS_SHIFT               22

#define CPU_CACHE_L1        1
#define CPU_CACHE_L2        2
#define CPU_CACHE_L3        3
#define CPU_CACHE_L4        4
#define CPU_CACHE_LMAX      CPU_CACHE_L4

typedef struct {
  UINT8                         CacheLevel;
  UINT8                         CacheDescriptor;
  UINT16                        CacheSizeinKB;
  CACHE_ASSOCIATIVITY_DATA      Associativity;
  CACHE_TYPE_DATA               SystemCacheType;
} CPU_CACHE_CONVERTER;


typedef struct {
  UINT16                        CacheSizeinKB;
  CACHE_ASSOCIATIVITY_DATA      Associativity;
  CACHE_TYPE_DATA               SystemCacheType;
  //
  // Can extend the structure here.
  //
} CPU_CACHE_DATA;

//
// It is defined for SMBIOS_TABLE_TYPE7.CacheConfiguration.
//
typedef struct {
  UINT16    Level           :3;
  UINT16    Socketed        :1;
  UINT16    Reserved2       :1;
  UINT16    Location        :2;
  UINT16    Enable          :1;
  UINT16    OperationalMode :2;
  UINT16    Reserved1       :6;
} CPU_CACHE_CONFIGURATION_DATA;

/**
  Add Cache Information to Type 7 SMBIOS Record.
  
  @param[in]    ProcessorNumber     Processor number of specified processor.
  @param[out]   L1CacheHandle       Pointer to the handle of the L1 Cache SMBIOS record.
  @param[out]   L2CacheHandle       Pointer to the handle of the L2 Cache SMBIOS record.
  @param[out]   L3CacheHandle       Pointer to the handle of the L3 Cache SMBIOS record.

**/
VOID
AddSmbiosCacheTypeTable (
  IN UINTN              ProcessorNumber,
  OUT EFI_SMBIOS_HANDLE *L1CacheHandle,
  OUT EFI_SMBIOS_HANDLE *L2CacheHandle,
  OUT EFI_SMBIOS_HANDLE *L3CacheHandle
  );

#endif

