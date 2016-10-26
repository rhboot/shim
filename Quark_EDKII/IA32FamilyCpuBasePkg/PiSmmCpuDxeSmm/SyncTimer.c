/** @file

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

#include "PiSmmCpuDxeSmm.h"

UINT64   mTimeoutTicker = 0;
//
//  Number of counts in a roll-over cycle of the performance counter.
//
UINT64   mCycle = 0;
//
// Flag to indicate the performance counter is count-up or count-down.
//
BOOLEAN  mCountDown;

/**
  Initialize Timer for Smm AP Sync.
  
**/
VOID
InitializeSmmTimer (
  VOID
  )
{
  UINT64  TimerFrequency;
  UINT64  Start;
  UINT64  End;

  TimerFrequency = GetPerformanceCounterProperties (&Start, &End);
  mTimeoutTicker = DivU64x32 (
                     MultU64x64(TimerFrequency, PcdGet64 (PcdCpuSmmApSyncTimeout)),
                     1000 * 1000
                     ); 
  if (End < Start) {
    mCountDown = TRUE;
    mCycle = Start - End;
  } else {
    mCountDown = FALSE;
    mCycle = End - Start;
  }
}

/**
  Start Timer for Smm AP Sync.
  
**/
UINT64
EFIAPI
StartSyncTimer (
  VOID
  )
{
  return GetPerformanceCounter ();
}


/**
  Check if the Smm AP Sync timer is timeout.
  
  @param Timer  The start timer from the begin.

**/
BOOLEAN
EFIAPI
IsSyncTimerTimeout (
  IN      UINT64                    Timer
  )
{
  UINT64  CurrentTimer;
  UINT64  Delta;

  CurrentTimer = GetPerformanceCounter ();

  if (mCountDown) {
    //
    // The performance counter counts down.  Check for roll over condition.
    //
    if (CurrentTimer < Timer) {
      Delta = Timer - CurrentTimer;
    } else {
      //
      // Handle one roll-over. 
      //
      Delta = mCycle - (CurrentTimer - Timer);
    }
  } else {
    //
    // The performance counter counts up.  Check for roll over condition.
    //
    if (CurrentTimer > Timer) {
      Delta = CurrentTimer - Timer;
    } else {
      //
      // Handle one roll-over. 
      //
      Delta = mCycle - (Timer - CurrentTimer);
    }
  }
 
  return (BOOLEAN) (Delta >= mTimeoutTicker);
}
