/** @file
  AsmWriteMsr64 function

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


#include <QuarkNcSocId.h>
#include "QuarkMsr.h"

/**
  Writes a 64-bit value to a Machine Specific Register(MSR), and returns the
  value.

  Writes the 64-bit value specified by Value to the MSR specified by Index. The
  64-bit value written to the MSR is returned. No parameter checking is
  performed on Index or Value, and some of these may cause CPU exceptions. The
  caller must either guarantee that Index and Value are valid, or the caller
  must establish proper exception handlers. This function is only available on
  IA-32 and x64.

  @param  Index The 32-bit MSR index to write.
  @param  Value The 64-bit value to write to the MSR.

  @return Value

**/
UINT64
EFIAPI
AsmWriteMsr64 (
  IN UINT32  Index,
  IN UINT64  Value
  )
{
  UINT8  QuarkMsrIndex = 0;
  UINT32  QuarkMsrDataLow = 0;
  UINT32  QuarkMsrDataHigh = 0;
  UINT64  QuarkMsrReturn = 0;

  QuarkMsrIndex = TranslateMsrIndex (Index);
  if((QuarkMsrIndex >= QUARK_NC_HOST_BRIDGE_IA32_MTRR_CAP) && (QuarkMsrIndex <= QUARK_NC_HOST_BRIDGE_IA32_MTRR_PHYSMASK7)) {
    QuarkMsrDataLow = (Value & 0xFFFFFFFF);
	  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QuarkMsrIndex, QuarkMsrDataLow);
	  if((QuarkMsrIndex >= QUARK_NC_HOST_BRIDGE_MTRR_FIX64K_00000) && (QuarkMsrIndex <= QUARK_NC_HOST_BRIDGE_MTRR_FIX4K_F8000)) {
      QuarkMsrDataHigh = ((Value >> 32) & 0xFFFFFFFF);
      QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QuarkMsrIndex+1, QuarkMsrDataHigh);
	  }
    QuarkMsrReturn = AsmReadMsrReturnResult(Index, QuarkMsrDataLow, QuarkMsrDataHigh);
  }
  else {
    QuarkMsrReturn = AsmWriteMsrRealMsrAccess(Index, QuarkMsrDataLow, QuarkMsrDataHigh);
  }
  return (QuarkMsrReturn);
}

