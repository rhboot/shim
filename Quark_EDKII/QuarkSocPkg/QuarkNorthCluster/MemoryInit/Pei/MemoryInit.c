/**@file
 Framework PEIM to initialize memory on a QuarkNcSocId Memory Controller.

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

//
// Include common header file for this module.
//
#include "MemoryInit.h"

static PEI_CLT_MEMORY_INIT_PPI mPeiCltMemoryInitPpi =
{ MrcStart };

static EFI_PEI_PPI_DESCRIPTOR PpiListPeiCltMemoryInit =
{
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gCltMemoryInitPpiGuid,
    &mPeiCltMemoryInitPpi
};

void Mrc( MRCParams_t *MrcData);

/**

 Do memory initialization for QuarkNcSocId DDR3 SDRAM Controller

 @param  FfsHeader    Not used.
 @param  PeiServices  General purpose services available to every PEIM.

 @return EFI_SUCCESS  Memory initialization completed successfully.
 All other error conditions encountered result in an ASSERT.

 **/
EFI_STATUS
PeimMemoryInit(
    IN EFI_PEI_FILE_HANDLE FileHandle,
    IN CONST EFI_PEI_SERVICES **PeiServices
    )
{
  EFI_STATUS Status;

  Status = (**PeiServices).InstallPpi(PeiServices, &PpiListPeiCltMemoryInit);

  return Status;
}

VOID
EFIAPI
MrcStart(
    IN OUT MRCParams_t *MrcData
    )
{

  Mrc(MrcData);
}
