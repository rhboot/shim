/**@file
  Memory Initialization PPI used in EFI PEI interface
  
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

#ifndef __CLT_MEMORY_INIT_H__
#define __CLT_MEMORY_INIT_H__

#include "mrc.h"

#define PEI_CLT_MEMORY_INIT_PPI_GUID \
  {0x21ff1fee, 0xd33a, 0x4fce, {0xa6, 0x5e, 0x95, 0x5e, 0xa3, 0xc4, 0x1f, 0x40}}  
  


//
// Memory range types
//
typedef enum {
  DualChannelDdrMainMemory,
  DualChannelDdrSmramCacheable,
  DualChannelDdrSmramNonCacheable,
  DualChannelDdrGraphicsMemoryCacheable,
  DualChannelDdrGraphicsMemoryNonCacheable,
  DualChannelDdrReservedMemory,
  DualChannelDdrMaxMemoryRangeType
} PEI_DUAL_CHANNEL_DDR_MEMORY_RANGE_TYPE;

//
// Memory map range information
//
typedef struct {
  EFI_PHYSICAL_ADDRESS                          PhysicalAddress;
  EFI_PHYSICAL_ADDRESS                          CpuAddress;
  EFI_PHYSICAL_ADDRESS                          RangeLength;
  PEI_DUAL_CHANNEL_DDR_MEMORY_RANGE_TYPE        Type;
} PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE;


//                                                        
// PPI Function Declarations                              
//                                                        
typedef                                                   
VOID                                                
(EFIAPI *PEI_CLT_MEMORY_INIT) (
  IN OUT    MRCParams_t     *MRCDATA
  );                                                     
                                                         
typedef struct _PEI_CLT_MEMORY_INIT_PPI {
  PEI_CLT_MEMORY_INIT     MrcStart;
}PEI_CLT_MEMORY_INIT_PPI;

extern EFI_GUID gCltMemoryInitPpiGuid;

#endif
