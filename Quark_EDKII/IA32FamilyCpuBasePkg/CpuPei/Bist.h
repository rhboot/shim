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

Module Name: Bist.h

**/

#ifndef _CPU_BIST_H_
#define _CPU_BIST_H_

/**

  Implementation of the PlatformInformation service
  
  Implementation of the PlatformInformation service in EFI_SEC_PLATFORM_INFORMATION_PPI.
  This function will parse SecPlatform Information from BIST Hob.

  @param  PeiServices                Pointer to the PEI Services Table.
  @param  StructureSize              Pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord  Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.
  
  @retval EFI_SUCCESS                The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL       The buffer was too small.
  
**/
EFI_STATUS
EFIAPI
SecPlatformInformation (
  IN CONST EFI_PEI_SERVICES                   **PeiServices,
  IN OUT UINT64                               *StructureSize,
     OUT EFI_SEC_PLATFORM_INFORMATION_RECORD *PlatformInformationRecord
  );

/**
  A callback function to build CPU(only BSP) BIST. 

  This function is a callback function to build bsp's BIST by calling SecPlatformInformationPpi

  @param  PeiServices      Pointer to PEI Services Table      
  @param  NotifyDescriptor Address if the notification descriptor data structure 
  @param  Ppi              Address of the PPI that was installed     
  @retval EFI_SUCCESS      Retrieve of the BIST data successfully 
  @retval EFI_SUCCESS      No sec platform information ppi export   
  @retval EFI_SUCCESS      The boot mode is S3 path   
**/
EFI_STATUS
EFIAPI
BuildBistHob (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

#endif
