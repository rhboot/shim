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

  SaveMemoryConfig.c
  
Abstract: 
  This is the driver that locates the MemoryConfigurationData HOB, if it
  exists, and saves the data to nvRAM.  

Revision History

--*/

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include <Guid/MemoryConfigData.h>
#include <Guid/DebugMask.h>

EFI_STATUS
EFIAPI
SaveMemoryConfigEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++
  
  Routine Description:
    This is the standard EFI driver point that detects whether there is a
    MemoryConfigurationData HOB and, if so, saves its data to nvRAM.

  Arguments:
    ImageHandle   - Handle for the image of this driver
    SystemTable   - Pointer to the EFI System Table

  Returns:
    EFI_SUCCESS   - if the data is successfully saved or there was no data
    EFI_NOT_FOUND - if the HOB list could not be located.
    EFI_UNLOAD_IMAGE - It is not success
    
--*/
{
  EFI_STATUS  Status;
  VOID        *HobList;
  EFI_HOB_GUID_TYPE *GuidHob;
  VOID        *HobData;
  VOID        *VariableData;
  UINTN       DataSize;
  UINTN       BufferSize;

  DataSize      = 0;
  VariableData  = NULL;
  GuidHob = NULL;
  HobList = NULL;
  HobData = NULL;
  Status  = EFI_UNSUPPORTED;

  //
  // Get the HOB list.  If it is not present, then ASSERT.
  //
  HobList = GetHobList ();
  ASSERT (HobList != NULL);

  //
  // Search for the Memory Configuration GUID HOB.  If it is not present, then
  // there's nothing we can do. It may not exist on the update path.
  //
  GuidHob = GetNextGuidHob (&gEfiMemoryConfigDataGuid, HobList);
  if (GuidHob != NULL) {
    HobData = GET_GUID_HOB_DATA (GuidHob);
    DataSize = GET_GUID_HOB_DATA_SIZE (GuidHob);
    //
    // Use the HOB to save Memory Configuration Data
    //
    BufferSize = DataSize;
    VariableData = AllocatePool (BufferSize);
    ASSERT (VariableData != NULL); 
    Status = gRT->GetVariable (
                    EFI_MEMORY_CONFIG_DATA_NAME,
                    &gEfiMemoryConfigDataGuid,
                    NULL,
                    &BufferSize,
                    VariableData
                    );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      (gBS->FreePool)(VariableData);
      VariableData = AllocatePool (BufferSize);
      ASSERT (VariableData != NULL);      
      Status = gRT->GetVariable (
                      EFI_MEMORY_CONFIG_DATA_NAME,
                      &gEfiMemoryConfigDataGuid,
                      NULL,
                      &BufferSize,
                      VariableData
                      );
    }

    if ( (EFI_ERROR(Status)) || BufferSize != DataSize || 0 != CompareMem (HobData, VariableData, DataSize)) {
      if (Status != EFI_SUCCESS){
        Status = gRT->SetVariable (
                        EFI_MEMORY_CONFIG_DATA_NAME,
                        &gEfiMemoryConfigDataGuid,
                        (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS),
                        DataSize,
                        HobData
                        );
      }
      ASSERT((Status == EFI_SUCCESS) || (Status == EFI_OUT_OF_RESOURCES));

      DEBUG((EFI_D_INFO, "Restored Size is 0x%x\n", DataSize));
    }
    (gBS->FreePool)(VariableData);
  }
  //
  // This driver does not produce any protocol services, so always unload it.
  //
  return Status;
}
