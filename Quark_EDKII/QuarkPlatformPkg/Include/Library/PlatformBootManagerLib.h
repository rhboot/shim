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
--*/


#ifndef __PLATFORM_BOOT_MANAGER_LIB_H_
#define __PLATFORM_BOOT_MANAGER_LIB_H_
#include <Library/UefiBootManagerLib.h>

//
// Bit mask returned from PlatformBootManagerInit
// We can define more value in future per requirement
//
#define PLATFORM_BOOT_MANAGER_ENABLE_HOTKEY      0x00000001
#define PLATFORM_BOOT_MANAGER_ENABLE_TIMEOUT     0x00000002
#define PLATFORM_BOOT_MANAGER_ENABLE_ALL         0xFFFFFFFF

/**
  Do the platform specific action before the console is ready
  Possible things that can be done in PlatformBootManagerInit:
  > Update console variable
  > Register new Driver#### or Boot####
  > Signal ReadyToLock event
  > Authentication action: 1. connect Auth devices; 2. Identify auto logon user.

  @return  Bit mask to indicate the platform boot manager requirement.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  );

/**
  Do the platform specific action after the console is ready
  Possible things that can be done in PlatformBootManagerConsoleReady:
  > Dynamically switch output mode
  > Signal console ready platform customized event
  > Run diagnostics: 1. driver health check; 2. memory test
  > Connect certain devices: 1. connect all; 2. connect nothing; 3. connect platform specified devices
  > Dispatch aditional option roms
   @param   PlatformBdsRequest  Original platform boot manager requirement.
   
   @return  Bit mask to indicate the udpated platform boot manager requirement.
**/
UINT32
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  );

/**
  This function is called each second during the boot manager waits the timeout.

  @param TimeoutRemain  The remaining timeout.
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16          TimeoutRemain
  );

#endif
