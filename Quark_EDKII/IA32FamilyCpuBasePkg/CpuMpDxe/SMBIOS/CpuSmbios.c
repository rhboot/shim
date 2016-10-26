/** @file
  Code to log processor and cache subclass data to smbios record.

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

#include "Processor.h"
#include "Cache.h"

EFI_SMBIOS_PROTOCOL     *mSmbios;
EFI_HII_HANDLE          mStringHandle;
UINT32                  mPopulatedSocketCount;

/**
  Add SMBIOS Processor Type and Cache Type tables for the CPU.
**/
VOID
AddCpuSmbiosTables (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_SMBIOS_HANDLE     L1CacheHandle;
  EFI_SMBIOS_HANDLE     L2CacheHandle;
  EFI_SMBIOS_HANDLE     L3CacheHandle;
  UINT32                PreviousPackageNumber;
  UINT32                PackageNumber;
  UINTN                 ProcessorIndex;
  UINTN                 *SocketProcessorNumberTable;
  UINT32                SocketIndex;

  L1CacheHandle = 0xFFFF;
  L2CacheHandle = 0xFFFF;
  L3CacheHandle = 0xFFFF;

  //
  // Initialize the mSmbios to contain the SMBIOS protocol,
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **) &mSmbios);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize strings to HII database
  //
  mStringHandle = HiiAddPackages (
                    &gEfiCallerIdGuid,
                    NULL,
                    CpuMpDxeStrings,
                    NULL
                    );
  ASSERT (mStringHandle != NULL);

  SocketProcessorNumberTable = AllocateZeroPool (mCpuConfigConextBuffer.NumberOfProcessors * sizeof (UINTN));
  ASSERT (SocketProcessorNumberTable != NULL);

  //
  // Detect populated sockets (comparing the processors' PackangeNumber) and record their ProcessorNumber.
  // For example:
  //   ProcessorNumber: 0 1 2 3 (PackageNumber 0) 4 5 6 7 (PackageNumber 1)
  //   And then, populated socket count will be 2 and record ProcessorNumber 0 for Socket 0, ProcessorNumber 4 for Socket 1
  //

  //
  // System has 1 populated socket at least, initialize mPopulatedSocketCount to 1 and record ProcessorNumber 0 for it.
  //
  mPopulatedSocketCount = 1;
  SocketProcessorNumberTable[0] = 0;
  GetProcessorLocation (0, &PreviousPackageNumber, NULL, NULL);

  //
  // Scan and compare the processors' PackageNumber to find the populated sockets.
  //
  for (ProcessorIndex = 1; ProcessorIndex < mCpuConfigConextBuffer.NumberOfProcessors; ProcessorIndex++) {
    GetProcessorLocation (ProcessorIndex, &PackageNumber, NULL, NULL);
    if (PackageNumber != PreviousPackageNumber) {
      //
      // Found a new populated socket.
      //
      PreviousPackageNumber = PackageNumber;
      mPopulatedSocketCount++;
      SocketProcessorNumberTable[mPopulatedSocketCount - 1] = ProcessorIndex;
    }
  }

  //
  // Add SMBIOS tables for populated sockets.
  //
  for (SocketIndex = 0; SocketIndex < mPopulatedSocketCount; SocketIndex++) {
    AddSmbiosCacheTypeTable (SocketProcessorNumberTable[SocketIndex], &L1CacheHandle, &L2CacheHandle, &L3CacheHandle);
    AddSmbiosProcessorTypeTable (SocketProcessorNumberTable[SocketIndex], L1CacheHandle, L2CacheHandle, L3CacheHandle);
  }
  FreePool (SocketProcessorNumberTable);

  //
  // Register notification functions for Smbios Processor Type.
  //
  SmbiosProcessorTypeTableCallback ();
}

