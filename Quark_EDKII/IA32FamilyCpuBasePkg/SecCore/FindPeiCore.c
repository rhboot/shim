/** @file
  Locate the entry point for the PEI Core

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

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#include "SecMain.h"

/**
  Find core image base.
  
  @param   BootFirmwareVolumePtr    Point to the boot firmware volume.
  @param   SecCoreImageBase         The base address of the SEC core image.
  @param   PeiCoreImageBase         The base address of the PEI core image.

**/
EFI_STATUS
EFIAPI
FindImageBase (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *BootFirmwareVolumePtr,
  OUT EFI_PHYSICAL_ADDRESS             *SecCoreImageBase,
  OUT EFI_PHYSICAL_ADDRESS             *PeiCoreImageBase
  )
{
  EFI_PHYSICAL_ADDRESS        CurrentAddress;
  EFI_PHYSICAL_ADDRESS        EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER         *File;
  UINT32                      Size;
  EFI_PHYSICAL_ADDRESS        EndOfFile;
  EFI_COMMON_SECTION_HEADER   *Section;
  EFI_PHYSICAL_ADDRESS        EndOfSection;

  *SecCoreImageBase = 0;
  *PeiCoreImageBase = 0;

  CurrentAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) BootFirmwareVolumePtr;
  EndOfFirmwareVolume = CurrentAddress + BootFirmwareVolumePtr->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + BootFirmwareVolumePtr->HeaderLength; ; ) {

    CurrentAddress = (EndOfFile + 7) & 0xfffffffffffffff8ULL;
    if (CurrentAddress > EndOfFirmwareVolume) {
      return EFI_NOT_FOUND;
    }

    File = (EFI_FFS_FILE_HEADER*)(UINTN) CurrentAddress;
    if (IS_FFS_FILE2 (File)) {
      Size = FFS_FILE2_SIZE (File);
      if (Size <= 0x00FFFFFF) {
        return EFI_NOT_FOUND;
      }
    } else {
      Size = FFS_FILE_SIZE (File);
      if (Size < sizeof (EFI_FFS_FILE_HEADER)) {
        return EFI_NOT_FOUND;
      }
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return EFI_NOT_FOUND;
    }

    //
    // Look for SEC Core / PEI Core files
    //
    if (File->Type != EFI_FV_FILETYPE_SECURITY_CORE &&
        File->Type != EFI_FV_FILETYPE_PEI_CORE) {
      continue;
    }

    //
    // Loop through the FFS file sections within the FFS file
    //
    if (IS_FFS_FILE2 (File)) {
      EndOfSection = (EFI_PHYSICAL_ADDRESS) (UINTN) ((UINT8 *) File + sizeof (EFI_FFS_FILE_HEADER2));
    } else {
      EndOfSection = (EFI_PHYSICAL_ADDRESS) (UINTN) ((UINT8 *) File + sizeof (EFI_FFS_FILE_HEADER));
    }
    for (;;) {
      CurrentAddress = (EndOfSection + 3) & 0xfffffffffffffffcULL;
      Section = (EFI_COMMON_SECTION_HEADER*)(UINTN) CurrentAddress;

      if (IS_SECTION2 (Section)) {
        Size = SECTION2_SIZE (Section);
        if (Size <= 0x00FFFFFF) {
          return EFI_NOT_FOUND;
        }
      } else {
        Size = SECTION_SIZE (Section);
        if (Size < sizeof (EFI_COMMON_SECTION_HEADER)) {
          return EFI_NOT_FOUND;
        }
      }

      EndOfSection = CurrentAddress + Size;
      if (EndOfSection > EndOfFile) {
        return EFI_NOT_FOUND;
      }

      //
      // Look for executable sections
      //
      if (Section->Type == EFI_SECTION_PE32 || Section->Type == EFI_SECTION_TE) {
        if (File->Type == EFI_FV_FILETYPE_SECURITY_CORE) {
          if (IS_SECTION2 (Section)) {
            *SecCoreImageBase = (PHYSICAL_ADDRESS) (UINTN) ((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER2));
          } else {
            *SecCoreImageBase = (PHYSICAL_ADDRESS) (UINTN) ((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER));
          }
        } else {
          if (IS_SECTION2 (Section)) {
            *PeiCoreImageBase = (PHYSICAL_ADDRESS) (UINTN) ((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER2));
          } else {
            *PeiCoreImageBase = (PHYSICAL_ADDRESS) (UINTN) ((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER));
          }
        }
        break;
      }
    }

    //
    // Both SEC Core and PEI Core images found
    //
    if (*SecCoreImageBase != 0 && *PeiCoreImageBase != 0) {
      return EFI_SUCCESS;
    }
  }
}

/**
  Find and return Pei Core entry point.

  It also find SEC and PEI Core file debug inforamtion. It will report them if
  remote debug is enabled.
  
  @param   BootFirmwareVolumePtr    Point to the boot firmware volume.
  @param   PeiCoreEntryPoint        The entry point of the PEI core.

**/
VOID
EFIAPI
FindAndReportEntryPoints (
  IN  EFI_FIRMWARE_VOLUME_HEADER       *BootFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT         *PeiCoreEntryPoint
  )
{
  EFI_STATUS                       Status;
  EFI_PHYSICAL_ADDRESS             SecCoreImageBase;
  EFI_PHYSICAL_ADDRESS             PeiCoreImageBase;
  PE_COFF_LOADER_IMAGE_CONTEXT     ImageContext;

  //
  // Find SEC Core and PEI Core image base
  //
  Status = FindImageBase (BootFirmwareVolumePtr, &SecCoreImageBase, &PeiCoreImageBase);
  ASSERT_EFI_ERROR (Status);

  ZeroMem ((VOID *) &ImageContext, sizeof (PE_COFF_LOADER_IMAGE_CONTEXT));
  //
  // Report SEC Core debug information when remote debug is enabled
  //
  ImageContext.ImageAddress = SecCoreImageBase;
  ImageContext.PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageContext.ImageAddress);
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Report PEI Core debug information when remote debug is enabled
  //
  ImageContext.ImageAddress = PeiCoreImageBase;
  ImageContext.PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageContext.ImageAddress);
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Find PEI Core entry point
  //
  Status = PeCoffLoaderGetEntryPoint ((VOID *) (UINTN) PeiCoreImageBase, (VOID**) PeiCoreEntryPoint);
  if (EFI_ERROR (Status)) {
    *PeiCoreEntryPoint = 0;
  }

  return;
}

