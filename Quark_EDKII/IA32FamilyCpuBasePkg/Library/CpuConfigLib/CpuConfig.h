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

Module Name:

  CpuConfig.h

Abstract:

  Header for the status code data hub logging component

**/

#ifndef _CPU_CONFIG_H_
#define _CPU_CONFIG_H_

#include <PiDxe.h>

#include <Protocol/MpService.h>

#include <Library/CpuConfigLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SocketLga775Lib.h>

/**
  Worker function to create a new feature entry.
  
  This is a worker function to create a new feature entry. The new entry is added to
  the feature list by other functions in the library.

  @param  FeatureID   The ID of the feature.
  @param  Attribute   Feature-specific data.
  
  @return  The address of the created entry.

**/
CPU_FEATURE_ENTRY *
CreateFeatureEntry (
  IN  CPU_FEATURE_ID      FeatureID,
  IN  VOID                *Attribute
  );

/**
  Worker function to search for a feature entry in processor feature list.
  
  This is a worker function to search for a feature entry in processor feature list.

  @param  ProcessorNumber Handle number of specified logical processor
  @param  FeatureIndex    The index of the new node in feature list.
  
  @return Pointer to the feature node. If not found, NULL is returned.

**/
CPU_FEATURE_ENTRY *
SearchFeatureEntry (
  IN  UINTN               ProcessorNumber,
  IN  UINTN               FeatureIndex
  );

#endif
