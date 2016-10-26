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

  Facs.h

Abstract:

  This file describes the contents of the ACPI Firmware ACPI Control Structure (FACS)
  .  Some additional ACPI values are defined in Acpi10.h, Acpi20.h, and Acpi30.h
  All changes to the FACS contents should be done in this file.

--*/

#ifndef _FACS_H_
#define _FACS_H_

//
// Statements that include other files
//
#include <IndustryStandard/Acpi.h>

//
// FACS Definitions
//
#define EFI_ACPI_FIRMWARE_WAKING_VECTOR 0x00000000
#define EFI_ACPI_GLOBAL_LOCK 0x00000000

#define EFI_ACPI_FIRMWARE_CONTROL_STRUCTURE_FLAGS 0x00000000
#define EFI_ACPI_X_FIRMWARE_WAKING_VECTOR 0x0000000000000000

#endif
