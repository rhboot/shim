/** @file
Implementation of CPU driver for PEI phase.

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

Module Name: Cache.c

**/

#include "CpuPei.h"

/**
  Reset all the MTRRs to a known state.

  This function provides the PPI for PEI phase to Reset all the MTRRs to a known state(UC)
  @param  PeiServices     General purpose services available to every PEIM.  
  @param  This            Current instance of Pei Cache PPI.                 

  @retval EFI_SUCCESS     All MTRRs have been reset successfully.  
                      
**/
EFI_STATUS
EFIAPI
PeiResetCacheAttributes (
  IN  EFI_PEI_SERVICES     **PeiServices,
  IN  PEI_CACHE_PPI        *This
  );

/**
  program the MTRR according to the given range and cache type.

  This function provides the PPI for PEI phase to set the memory attribute by program
  the MTRR according to the given range and cache type. Actually this function is a 
  wrapper of the MTRR libaray to suit the PEI_CACHE_PPI interface

  @param  PeiServices     General purpose services available to every PEIM.  
  @param  This            Current instance of Pei Cache PPI.                 
  @param  MemoryAddress   Base Address of Memory to program MTRR.            
  @param  MemoryLength    Length of Memory to program MTRR.                  
  @param  MemoryCacheType Cache Type.   
  @retval EFI_SUCCESS             Mtrr are set successfully.        
  @retval EFI_LOAD_ERROR          No empty MTRRs to use.            
  @retval EFI_INVALID_PARAMETER   The input parameter is not valid. 
  @retval others                  An error occurs when setting MTTR.
                                          
**/
EFI_STATUS
EFIAPI
PeiSetCacheAttributes (
  IN  EFI_PEI_SERVICES         **PeiServices,
  IN  PEI_CACHE_PPI            *This,
  IN  EFI_PHYSICAL_ADDRESS     MemoryAddress,
  IN  UINT64                   MemoryLength,
  IN  EFI_MEMORY_CACHE_TYPE    MemoryCacheType
  );

PEI_CACHE_PPI   mCachePpi = {
  PeiSetCacheAttributes,
  PeiResetCacheAttributes
};


/**
  program the MTRR according to the given range and cache type.

  This function provides the PPI for PEI phase to set the memory attribute by program
  the MTRR according to the given range and cache type. Actually this function is a 
  wrapper of the MTRR libaray to suit the PEI_CACHE_PPI interface

  @param  PeiServices     General purpose services available to every PEIM.  
  @param  This            Current instance of Pei Cache PPI.                 
  @param  MemoryAddress   Base Address of Memory to program MTRR.            
  @param  MemoryLength    Length of Memory to program MTRR.                  
  @param  MemoryCacheType Cache Type.   
  @retval EFI_SUCCESS             Mtrr are set successfully.        
  @retval EFI_LOAD_ERROR          No empty MTRRs to use.            
  @retval EFI_INVALID_PARAMETER   The input parameter is not valid. 
  @retval others                  An error occurs when setting MTTR.
                                          
**/
EFI_STATUS
EFIAPI
PeiSetCacheAttributes (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN PEI_CACHE_PPI             *This,
  IN EFI_PHYSICAL_ADDRESS      MemoryAddress,
  IN UINT64                    MemoryLength,
  IN EFI_MEMORY_CACHE_TYPE     MemoryCacheType
  )
{

   RETURN_STATUS            Status;
   //
   // call MTRR libary function
   //
   Status = MtrrSetMemoryAttribute(
              MemoryAddress,
              MemoryLength,
              (MTRR_MEMORY_CACHE_TYPE) MemoryCacheType
            );
   return (EFI_STATUS)Status;
}
/**
  Reset all the MTRRs to a known state.

  This function provides the PPI for PEI phase to Reset all the MTRRs to a known state(UC)
  @param  PeiServices     General purpose services available to every PEIM.  
  @param  This            Current instance of Pei Cache PPI.                 

  @retval EFI_SUCCESS     All MTRRs have been reset successfully.  
                      
**/
EFI_STATUS
EFIAPI
PeiResetCacheAttributes (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN PEI_CACHE_PPI             *This
  )
{

   MTRR_SETTINGS  ZeroMtrr;
   
   //
   // reset all Mtrrs to 0 include fixed MTRR and variable MTRR
   //
   ZeroMem(&ZeroMtrr, sizeof(MTRR_SETTINGS));
   MtrrSetAllMtrrs(&ZeroMtrr);
   
   return EFI_SUCCESS;
}
