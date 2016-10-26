/** @file
  This file includes a memory call back function notified when MRC is done,
  following action is performed in this file,
    1. ICH initialization after MRC.
    2. SIO initialization.
    3. Install ResetSystem and FinvFv PPI.
    4. Set MTRR for PEI
    5. Create FV HOB and Flash HOB

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


#include "CommonHeader.h"

#include "PlatformEarlyInit.h"
#include <Cpu/CpuRegs.h>

extern EFI_PEI_PPI_DESCRIPTOR mPpiStall[];

STATIC EFI_PEI_RESET_PPI mResetPpi = { ResetSystem };

STATIC EFI_PEI_PPI_DESCRIPTOR mPpiList[1] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiResetPpiGuid,
    &mResetPpi
  }
};

//
// RTC:28208 - System hang/crash when entering probe mode(ITP) when relocating SMBASE
//             Workaround to make default SMRAM UnCachable
//
MTRR_FIXED_SETTINGS mFixedMtrrTable = {
    0x0606060600060606,
    0x0606060606060606,
    0,
    0x0000000000000000,
    0x0000000000000000,
    0x0000000000000000,
    0x0000000000000000,
    0x0000000000000000,
    0x0000000000000000,
    0x0000000000000000,
    0x0000000000000000
  };

/**
  This function reset the entire platform, including all processor and devices, and
  reboots the system.

  @param  PeiServices General purpose services available to every PEIM.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
ResetSystem (
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  ResetCold();
  return EFI_SUCCESS;
}

/**
  This function provides a blocking stall for reset at least the given number of microseconds
  stipulated in the final argument.

  @param  PeiServices General purpose services available to every PEIM.

  @param  this Pointer to the local data for the interface.

  @param  Microseconds number of microseconds for which to stall.

  @retval EFI_SUCCESS the function provided at least the required stall.
**/
EFI_STATUS
EFIAPI
Stall (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN CONST EFI_PEI_STALL_PPI  *This,
  IN UINTN                    Microseconds
  )
{
  MicroSecondDelay (Microseconds);
  return EFI_SUCCESS;
}


/**
  This function will be called when MRC is done.

  @param  PeiServices General purpose services available to every PEIM.

  @param  NotifyDescriptor Information about the notify event..

  @param  Ppi The notify context.

  @retval EFI_SUCCESS If the function completed successfully.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                            Status;
  EFI_BOOT_MODE                         BootMode;
  UINT64                                MemoryLength;
  UINT64                                MemoryLengthUc;
  UINT64                                MaxMemoryLength;
  UINT64                                MemOverflow;
  EFI_SMRAM_DESCRIPTOR                  *SmramDescriptor;
  UINTN                                 NumSmramRegions;
  UINT64                                EnlargedMemoryLength;
  UINT32                                RmuMainBaseAddress;
  UINTN                                 Index;
  MTRR_SETTINGS                         MtrrSetting;
  UINT32                                RegData32;
  EFI_PHYSICAL_ADDRESS                  NewBuffer;
  UINT8                                 CpuAddressWidth;
  EFI_CPUID_REGISTER                    FeatureInfo;
  
  DEBUG ((EFI_D_INFO, "Platform PEIM Memory Callback\n"));

  NumSmramRegions = 0;
  SmramDescriptor = NULL;
  RmuMainBaseAddress = 0;

  PERF_START (NULL, "SetCache", NULL, 0);

  InfoPostInstallMemory (&RmuMainBaseAddress, &SmramDescriptor, &NumSmramRegions);
  ASSERT (SmramDescriptor != NULL);
  ASSERT (RmuMainBaseAddress != 0);

  MemoryLength = ((UINT64) RmuMainBaseAddress) + 0x10000;
  EnlargedMemoryLength = MemoryLength;

  if (NumSmramRegions > 0) {
	  //
	  // Find the TSEG
	  //
	  for (Index = 0; Index < NumSmramRegions; Index ++) {
	    if ((SmramDescriptor[Index].PhysicalStart) == EnlargedMemoryLength) {
	      if (SmramDescriptor[Index].RegionState & EFI_CACHEABLE) {
	        //
	        // Enlarge memory length to include TSEG size
	        //
	        EnlargedMemoryLength += (SmramDescriptor[Index].PhysicalSize);
	      }
	    }
	  }
	}

  //
  // Check if a UC region is present
  //
  MaxMemoryLength = EnlargedMemoryLength;
  // Round up to nearest 256MB
  MemOverflow = (MemoryLength & 0x0fffffff);
  if (MemOverflow != 0) {
    MaxMemoryLength = MemoryLength + (0x10000000 - MemOverflow);
  }


  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  ZeroMem (&MtrrSetting, sizeof(MTRR_SETTINGS));
  (**PeiServices).CopyMem ((VOID *)&MtrrSetting.Fixed,(VOID *)&mFixedMtrrTable, sizeof(MTRR_FIXED_SETTINGS) );
  //
  // Cache the flash area to improve the boot performance in PEI phase
  //
  MtrrSetting.Variables.Mtrr[0].Base = (QUARK_BOOTROM_BASE_ADDRESS | CacheWriteBack);  
  MtrrSetting.Variables.Mtrr[0].Mask = (((~(QUARK_BOOTROM_SIZE_BYTES - 1)) & MTRR_LIB_CACHE_VALID_ADDRESS) | MTRR_LIB_CACHE_MTRR_ENABLED);
  
  MtrrSetting.Variables.Mtrr[1].Base = CacheWriteBack;  
  MtrrSetting.Variables.Mtrr[1].Mask = ((~(MaxMemoryLength - 1)) & MTRR_LIB_CACHE_VALID_ADDRESS) | MTRR_LIB_CACHE_MTRR_ENABLED;

  Index = 2;
  while (MaxMemoryLength != MemoryLength) {
    MemoryLengthUc = GetPowerOfTwo64 (MaxMemoryLength - MemoryLength);

    //Status = MtrrSetMemoryAttribute (MaxMemoryLength - MemoryLengthUc, MemoryLengthUc, CacheUncacheable);
    //ASSERT_EFI_ERROR (Status);
    
    MtrrSetting.Variables.Mtrr[Index].Base = ((MaxMemoryLength - MemoryLengthUc) & MTRR_LIB_CACHE_VALID_ADDRESS) | CacheUncacheable;
    MtrrSetting.Variables.Mtrr[Index].Mask= ((~(MemoryLengthUc   - 1)) & MTRR_LIB_CACHE_VALID_ADDRESS) | MTRR_LIB_CACHE_MTRR_ENABLED;      
    MaxMemoryLength -= MemoryLengthUc;
    Index++;
  }

  AsmInvd ();
 
  MtrrSetting.MtrrDefType = MTRR_LIB_CACHE_MTRR_ENABLED | MTRR_LIB_CACHE_FIXED_MTRR_ENABLED;
  MtrrSetAllMtrrs(&MtrrSetting);

  PERF_END (NULL, "SetCache", NULL, 0);

  //
  // Install PeiReset for PeiResetSystem service
  //
  Status = PeiServicesInstallPpi (&mPpiList[0]);
  ASSERT_EFI_ERROR (Status);

  //
  // Do QNC initialization after MRC
  //
  PeiQNCPostMemInit ();

  Status = PeiServicesInstallPpi (&mPpiStall[0]);
  ASSERT_EFI_ERROR (Status);

  //
  // Set E000/F000 Routing
  //
  RegData32 = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  RegData32 |= (BIT2|BIT1);
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, RegData32);

  if (BootMode == BOOT_IN_RECOVERY_MODE) {
  PeiServicesInstallFvInfoPpi (
    NULL,
    (VOID *) (UINTN) PcdGet32 (PcdFlashFvRecovery2Base),
    PcdGet32 (PcdFlashFvRecovery2Size),
    NULL,
    NULL
    );
    Status = PeimInitializeRecovery (PeiServices);
    ASSERT_EFI_ERROR (Status);

  } else if (BootMode == BOOT_ON_S3_RESUME) {
    return EFI_SUCCESS;

  } else {
    //
    // Allocate the memory so that it gets preserved into DXE
    //
    Status = PeiServicesAllocatePages (
              EfiBootServicesData,
              EFI_SIZE_TO_PAGES (PcdGet32 (PcdFvSecurityHeaderSize) + PcdGet32 (PcdFlashFvMainSize)),
              &NewBuffer
              );

    //
    // Copy the compressed main Firmware Volume to memory for faster processing later
    //
    CopyMem ((VOID *) (UINTN) NewBuffer, (VOID *) (UINTN) (PcdGet32 (PcdFlashFvMainBase) - PcdGet32 (PcdFvSecurityHeaderSize)), (PcdGet32 (PcdFvSecurityHeaderSize) +PcdGet32 (PcdFlashFvMainSize)));

    PeiServicesInstallFvInfoPpi (
      NULL,
      (VOID *) (UINTN) (NewBuffer + PcdGet32 (PcdFvSecurityHeaderSize)),
      PcdGet32 (PcdFlashFvMainSize),
      NULL,
      NULL
      );
  }

  //
  // Build flash HOB, it's going to be used by GCD and E820 building
  // Map full SPI flash decode range (regardless of smaller SPI flash parts installed)
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_FIRMWARE_DEVICE,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT    |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    (SIZE_4GB - SIZE_8MB),
    SIZE_8MB
    );

  //
  // Create a CPU hand-off information
  //
  CpuAddressWidth = 32;
  AsmCpuid (EFI_CPUID_EXTENDED_FUNCTION, &FeatureInfo.RegEax, NULL, NULL, NULL);
  if (FeatureInfo.RegEax >= EFI_CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (EFI_CPUID_VIR_PHY_ADDRESS_SIZE, &FeatureInfo.RegEax, NULL, NULL, NULL);
    CpuAddressWidth = (UINT8) (FeatureInfo.RegEax & 0xFF);
  }
  DEBUG ((EFI_D_INFO, "CpuAddressWidth: %d\n", CpuAddressWidth));

  BuildCpuHob (CpuAddressWidth, 16);

  ASSERT_EFI_ERROR (Status);

  return Status;
}
