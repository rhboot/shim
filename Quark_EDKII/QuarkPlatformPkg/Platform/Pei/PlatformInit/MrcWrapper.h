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

    MrcWrapper.h

Abstract:

    Framework PEIM to initialize memory on an DDR2 SDRAM Memory Controller.

--*/

#ifndef _MRC_WRAPPER_H
#define _MRC_WRAPPER_H

#include <Ppi/CltMemoryInit.h>
#include "PlatformEarlyInit.h"

//
// Define the default memory areas required
//
#define EDKII_RESERVED_SIZE_PAGES         0x10
#define ACPI_RECLAIM_SIZE_PAGES           0x40
#define ACPI_NVS_SIZE_PAGES               0xD00
#define RUNTIME_SERVICES_CODE_SIZE_PAGES  0x60
#define RUNTIME_SERVICES_DATA_SIZE_PAGES  0x60
#define EDKII_DXE_MEM_SIZE_PAGES          0x20

#define AP_STARTUP_VECTOR                 0x00097000

//
// Maximum number of "Socket Sets", where a "Socket Set is a set of matching
// DIMM's from the various channels
//
#define MAX_SOCKET_SETS      2

//
// Maximum number of memory ranges supported by the memory controller
//
#define MAX_RANGES (MAX_ROWS + 5)

//
// Min. of 48MB PEI phase
//
#define  PEI_MIN_MEMORY_SIZE               (6 * 0x800000)
#define  PEI_RECOVERY_MIN_MEMORY_SIZE      (6 * 0x800000)

#define PEI_MEMORY_RANGE_OPTION_ROM UINT32
#define PEI_MR_OPTION_ROM_NONE      0x00000000

//
// SMRAM Memory Range
//
#define PEI_MEMORY_RANGE_SMRAM      UINT32
#define PEI_MR_SMRAM_ALL            0xFFFFFFFF
#define PEI_MR_SMRAM_NONE           0x00000000
#define PEI_MR_SMRAM_CACHEABLE_MASK 0x80000000
#define PEI_MR_SMRAM_SEGTYPE_MASK   0x00FF0000
#define PEI_MR_SMRAM_ABSEG_MASK     0x00010000
#define PEI_MR_SMRAM_HSEG_MASK      0x00020000
#define PEI_MR_SMRAM_TSEG_MASK      0x00040000
//
// SMRAM Size is a multiple of 128KB.
//
#define PEI_MR_SMRAM_SIZE_MASK          0x0000FFFF

//
// Pci Memory Hole
//
#define PEI_MEMORY_RANGE_PCI_MEMORY       UINT32

typedef enum {
  Ignore,
  Quick,
  Sparse,
  Extensive
} PEI_MEMORY_TEST_OP;

//
// MRC Params Variable structure.
//

typedef struct {
  MrcTimings_t timings;              // Actual MRC config values saved in variable store.
  UINT8        VariableStorePad[8];  // Allow for data stored in variable is required to be multiple of 8bytes.
} PLATFORM_VARIABLE_MEMORY_CONFIG_DATA;

//
// Function prototypes.
//

EFI_STATUS
InstallEfiMemory (
  IN      EFI_PEI_SERVICES                           **PeiServices,
  IN      EFI_PEI_READ_ONLY_VARIABLE2_PPI            *VariableServices,
  IN      EFI_BOOT_MODE                              BootMode,
  IN      UINT32                                     TotalMemorySize
  );

EFI_STATUS
InstallS3Memory (
  IN      EFI_PEI_SERVICES                      **PeiServices,
  IN      EFI_PEI_READ_ONLY_VARIABLE2_PPI       *VariableServices,
  IN      UINT32                                TotalMemorySize
  );

EFI_STATUS
MemoryInit (
  IN EFI_PEI_SERVICES                       **PeiServices
  );

  
EFI_STATUS
LoadConfig (
  IN      EFI_PEI_SERVICES                        **PeiServices,
  IN      EFI_PEI_READ_ONLY_VARIABLE2_PPI         *VariableServices,
  IN OUT  MRCParams_t                             *MrcData
  );

EFI_STATUS
SaveConfig (
  IN      MRCParams_t                      *MrcData
  );

VOID
RetriveRequiredMemorySize (
  IN      EFI_PEI_SERVICES                  **PeiServices,
  OUT     UINTN                             *Size
  );

EFI_STATUS
GetMemoryMap (
  IN     EFI_PEI_SERVICES                                    **PeiServices,
  IN     UINT32                                              TotalMemorySize,
  IN OUT PEI_DUAL_CHANNEL_DDR_MEMORY_MAP_RANGE               *MemoryMap,
  IN OUT UINT8                                               *NumRanges
  );  

EFI_STATUS
ChooseRanges (
  IN OUT   PEI_MEMORY_RANGE_OPTION_ROM      *OptionRomMask,
  IN OUT   PEI_MEMORY_RANGE_SMRAM           *SmramMask,
  IN OUT   PEI_MEMORY_RANGE_PCI_MEMORY      *PciMemoryMask
  );
  
EFI_STATUS
GetPlatformMemorySize (
  IN      EFI_PEI_SERVICES                       **PeiServices,
  IN      EFI_BOOT_MODE                          BootMode,
  IN OUT  UINT64                                 *MemorySize
  );

EFI_STATUS
BaseMemoryTest (
  IN  EFI_PEI_SERVICES                   **PeiServices,
  IN  EFI_PHYSICAL_ADDRESS               BeginAddress,
  IN  UINT64                             MemoryLength,
  IN  PEI_MEMORY_TEST_OP                 Operation,
  OUT EFI_PHYSICAL_ADDRESS               *ErrorAddress
  );

EFI_STATUS
SetPlatformImrPolicy (
  IN      EFI_PHYSICAL_ADDRESS    PeiMemoryBaseAddress,
  IN      UINT64                  PeiMemoryLength,
  IN      UINTN                   RequiredMemSize
  );

VOID
EFIAPI
InfoPostInstallMemory (
  OUT     UINT32                  *RmuBaseAddressPtr OPTIONAL,
  OUT     EFI_SMRAM_DESCRIPTOR    **SmramDescriptorPtr OPTIONAL,
  OUT     UINTN                   *NumSmramRegionsPtr OPTIONAL
  );

#endif
