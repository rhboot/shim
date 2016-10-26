/** @file
  This file include the file which can help to get the system
  performance, all the function will only include if the performance
  switch is set.

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
--*/

#include "InternalBdsLib.h"

PERF_HEADER               mPerfHeader;
PERF_DATA                 mPerfData;
EFI_PHYSICAL_ADDRESS      mAcpiLowMemoryBase = 0x0FFFFFFFFULL;
UINT32                    mAcpiLowMemoryLength = 0x4000;

/**
  Get the short verion of PDB file name to be
  used in performance data logging.

  @param PdbFileName     The long PDB file name.
  @param GaugeString     The output string to be logged by performance logger.

**/
VOID
GetShortPdbFileName (
  IN  CONST CHAR8  *PdbFileName,
  OUT       CHAR8  *GaugeString
  )
{
  UINTN Index;
  UINTN Index1;
  UINTN StartIndex;
  UINTN EndIndex;

  if (PdbFileName == NULL) {
    AsciiStrCpy (GaugeString, " ");
  } else {
    StartIndex = 0;
    for (EndIndex = 0; PdbFileName[EndIndex] != 0; EndIndex++)
      ;

    for (Index = 0; PdbFileName[Index] != 0; Index++) {
      if (PdbFileName[Index] == '\\') {
        StartIndex = Index + 1;
      }

      if (PdbFileName[Index] == '.') {
        EndIndex = Index;
      }
    }

    Index1 = 0;
    for (Index = StartIndex; Index < EndIndex; Index++) {
      GaugeString[Index1] = PdbFileName[Index];
      Index1++;
      if (Index1 == PERF_TOKEN_LENGTH - 1) {
        break;
      }
    }

    GaugeString[Index1] = 0;
  }

  return ;
}

/**
  Get the name from the Driver handle, which can be a handle with
  EFI_LOADED_IMAGE_PROTOCOL or EFI_DRIVER_BINDING_PROTOCOL installed.
  This name can be used in performance data logging.

  @param Handle          Driver handle.
  @param GaugeString     The output string to be logged by performance logger.

**/
VOID
GetNameFromHandle (
  IN  EFI_HANDLE     Handle,
  OUT CHAR8          *GaugeString
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *Image;
  CHAR8                       *PdbFileName;
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;

  AsciiStrCpy (GaugeString, " ");

  //
  // Get handle name from image protocol
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &Image
                  );

  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    Handle,
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **) &DriverBinding,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return ;
    }
    //
    // Get handle name from image protocol
    //
    Status = gBS->HandleProtocol (
                    DriverBinding->ImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **) &Image
                    );
  }

  PdbFileName = PeCoffLoaderGetPdbPointer (Image->ImageBase);

  if (PdbFileName != NULL) {
    GetShortPdbFileName (PdbFileName, GaugeString);
  }

  return ;
}

/**

  Allocates a block of memory to store performance data.

**/
VOID
AllocateMemoryForPerformanceData (
  VOID
  )
{
  EFI_STATUS    Status;

  if (mAcpiLowMemoryBase == 0x0FFFFFFFF) {
    //
    // Allocate a block of memory that contain performance data to OS
    //
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    EfiReservedMemoryType,
                    EFI_SIZE_TO_PAGES (mAcpiLowMemoryLength),
                    &mAcpiLowMemoryBase
                    );
    if (!EFI_ERROR (Status)) {
      Status = gRT->SetVariable (
                      L"PerfDataMemAddr",
                      &gPerformanceProtocolGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof (EFI_PHYSICAL_ADDRESS),
                      &mAcpiLowMemoryBase
                      );
      ASSERT_EFI_ERROR (Status);
    }
  }
}

/**

  Free the memory to store performance data.

**/
VOID
FreeMemoryForPerformanceData (
  VOID
  )
{
  ASSERT (mAcpiLowMemoryBase != 0x0FFFFFFFF);
  FreePages ((VOID *) (UINTN) mAcpiLowMemoryBase, EFI_SIZE_TO_PAGES (mAcpiLowMemoryLength));
  mAcpiLowMemoryBase = 0x0FFFFFFFF;
}

/**

  Writes performance data of booting into the allocated memory.
  OS can process these records.

  @param  Event                 The triggered event.
  @param  Context               Context for this event.

**/
VOID
EFIAPI
WriteBootToOsPerformanceData (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                Status;
  UINT32                    LimitCount;
  EFI_HANDLE                *Handles;
  UINTN                     NoHandles;
  CHAR8                     GaugeString[PERF_TOKEN_LENGTH];
  UINT8                     *Ptr;
  UINT32                    Index;
  UINT64                    Ticker;
  UINT64                    Freq;
  UINT32                    Duration;
  UINTN                     LogEntryKey;
  CONST VOID                *Handle;
  CONST CHAR8               *Token;
  CONST CHAR8               *Module;
  UINT64                    StartTicker;
  UINT64                    EndTicker;
  UINT64                    StartValue;
  UINT64                    EndValue;
  BOOLEAN                   CountUp;
  UINTN                     EntryIndex;
  UINTN                     NumPerfEntries;
  //
  // List of flags indicating PerfEntry contains DXE handle
  //
  BOOLEAN                   *PerfEntriesAsDxeHandle;

  //
  // Record the performance data for End of BDS
  //
  PERF_END(NULL, "BDS", NULL, 0);

  if (mAcpiLowMemoryBase == 0x0FFFFFFFF) {
    return;
  }

  //
  // Retrieve time stamp count as early as possible
  //
  Ticker  = GetPerformanceCounter ();

  Freq    = GetPerformanceCounterProperties (&StartValue, &EndValue);
  
  Freq    = DivU64x32 (Freq, 1000);

  mPerfHeader.CpuFreq = Freq;

  //
  // Record BDS raw performance data
  //
  if (EndValue >= StartValue) {
    mPerfHeader.BDSRaw = Ticker - StartValue;
    CountUp            = TRUE;
  } else {
    mPerfHeader.BDSRaw = StartValue - Ticker;
    CountUp            = FALSE;
  }

  //
  // Put Detailed performance data into memory
  //
  Handles = NULL;
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &NoHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  Ptr        = (UINT8 *) ((UINT32) mAcpiLowMemoryBase + sizeof (PERF_HEADER));
  LimitCount = (mAcpiLowMemoryLength - sizeof (PERF_HEADER)) / sizeof (PERF_DATA);

  NumPerfEntries = 0;
  LogEntryKey    = 0;
  while ((LogEntryKey = GetPerformanceMeasurement (
                          LogEntryKey,
                          &Handle,
                          &Token,
                          &Module,
                          &StartTicker,
                          &EndTicker)) != 0) {
    NumPerfEntries++;
  }
  PerfEntriesAsDxeHandle = AllocateZeroPool (NumPerfEntries * sizeof (BOOLEAN));
  ASSERT (PerfEntriesAsDxeHandle != NULL);
  
  //
  // Get DXE drivers performance
  //
  for (Index = 0; Index < NoHandles; Index++) {
    Ticker = 0;
    LogEntryKey = 0;
    EntryIndex  = 0;
    while ((LogEntryKey = GetPerformanceMeasurement (
                            LogEntryKey,
                            &Handle,
                            &Token,
                            &Module,
                            &StartTicker,
                            &EndTicker)) != 0) {
      if (Handle == Handles[Index] && !PerfEntriesAsDxeHandle[EntryIndex]) {
        PerfEntriesAsDxeHandle[EntryIndex] = TRUE;
      }
      EntryIndex++;
      if ((Handle == Handles[Index]) && (EndTicker != 0)) {
        if (StartTicker == 1) {
          StartTicker = StartValue;
        }
        if (EndTicker == 1) {
          EndTicker = StartValue;
        }
        Ticker += CountUp ? (EndTicker - StartTicker) : (StartTicker - EndTicker);
      }
    }

    Duration = (UINT32) DivU64x32 (Ticker, (UINT32) Freq);

    if (Duration > 0) {

      GetNameFromHandle (Handles[Index], GaugeString);

      AsciiStrCpy (mPerfData.Token, GaugeString);
      mPerfData.Duration = Duration;

      CopyMem (Ptr, &mPerfData, sizeof (PERF_DATA));
      Ptr += sizeof (PERF_DATA);

      mPerfHeader.Count++;
      if (mPerfHeader.Count == LimitCount) {
        goto Done;
      }
    }
  }

  //
  // Get inserted performance data
  //
  LogEntryKey = 0;
  EntryIndex  = 0;
  while ((LogEntryKey = GetPerformanceMeasurement (
                          LogEntryKey,
                          &Handle,
                          &Token,
                          &Module,
                          &StartTicker,
                          &EndTicker)) != 0) {
    if (!PerfEntriesAsDxeHandle[EntryIndex] && EndTicker != 0) {

      ZeroMem (&mPerfData, sizeof (PERF_DATA));

      AsciiStrnCpy (mPerfData.Token, Token, PERF_TOKEN_LENGTH);
      if (StartTicker == 1) {
        StartTicker = StartValue;
      }
      if (EndTicker == 1) {
        EndTicker = StartValue;
      }
      Ticker = CountUp ? (EndTicker - StartTicker) : (StartTicker - EndTicker);

      mPerfData.Duration = (UINT32) DivU64x32 (Ticker, (UINT32) Freq);

      CopyMem (Ptr, &mPerfData, sizeof (PERF_DATA));
      Ptr += sizeof (PERF_DATA);

      mPerfHeader.Count++;
      if (mPerfHeader.Count == LimitCount) {
        goto Done;
      }
    }
    EntryIndex++;
  }

Done:

  FreePool (Handles);
  FreePool (PerfEntriesAsDxeHandle);

  mPerfHeader.Signiture = PERFORMANCE_SIGNATURE;

  //
  // Put performance data to Reserved memory
  //
  CopyMem (
    (UINTN *) (UINTN) mAcpiLowMemoryBase,
    &mPerfHeader,
    sizeof (PERF_HEADER)
    );

  return ;
}
