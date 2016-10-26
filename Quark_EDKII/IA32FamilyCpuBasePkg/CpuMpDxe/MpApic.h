/** @file

  Include file for APIC feature.

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

  Module Name:  MpApic.h

**/

#ifndef _CPU_MP_APIC_H_
#define _CPU_MP_APIC_H_

#include "Cpu.h"

/**
  Sort the APIC ID of all processors.

  This function sorts the APIC ID of all processors so that processor number is assigned in the
  ascending order of APIC ID which eases MP debugging. SMBIOS logic also depends on this assumption.

**/
VOID
SortApicId (
  VOID
  );

/**
  Check if there is legacy APIC ID conflict among all processors.

  @retval EFI_SUCCESS      No coflict or conflict was resoved by force x2APIC mode
  @retval EFI_UNSUPPORTED  There is legacy APIC ID conflict and can't be rsolved in xAPIC mode
**/
EFI_STATUS
CheckApicId (
  VOID
  );

/**
  Assign a package scope BSP responsible for package scope programming.

  This functions assigns the processor with the least APIC ID within a processor package as
  the package BSP.

**/
VOID
AssignPackageBsp (
  VOID
  );

#endif
