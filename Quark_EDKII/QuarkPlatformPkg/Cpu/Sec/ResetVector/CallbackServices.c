/** @file
  C funtions in SEC

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


#include "CallbackServices.h"

/*! \fn 	BOOLEAN ValidateModuleCallBack(	const QuarkSecurityHeader_t * const Header,
								const RSA2048PublicKey_t * const RsaKey,
								ScratchMemory_t	* const MemoryBuffer
								)
 *  \brief 	Callback function to be used by BIOS/OS for validation of signed modules.
 *  		This function is uncalled by ROM code, though is reference bt flat32.asm
 *  		as a data constant to ensure the linker includes the function (i.e.
 *  		it isn't optimised away.
 *  \param	Header: Pointer to the header of the module to be validated.
 *  \param	RsaKey: Pointer to key to be used for validation.
 *  \param	MemoryBuffer: Pointer to scratchpad memory the validation code can use.
 *  \return	BOOLEAN: TRUE if module validates, FALSE if it fails for any reason.
 */
BOOLEAN ValidateModuleCallBack(	const QuarkSecurityHeader_t * const Header,
								const RSA2048PublicKey_t * const RsaKey,
								ScratchMemory_t	* const MemoryBuffer
								)
{
	return TRUE;
}

/** Validate key module routine.

  Checks the SHA256 digest of the Oem Key in a module header against
  that of the key held in fuse bank and then uses that key to validate
  key module.

  @param[in]       Header         A pointer to a header structure of a key
                                  module in memory to be validated..
  @param[in]       Data           Pointer to the signed data associated with
                                  this module.
  @param[in]       ModuleKey      The key used to sign the data.
  @param[in]       Signature      The signature of the data.
  @param[in]       MemoryBuffer   Pointer to crypto heap management structure.
  @param[in]       KeyBankNumber  Which fuse bank are we pulling the Intel key from.

  @retval  QUARK_ROM_NO_ERROR                            If Key Module Validates 
                                                         Successfully.
  @retval  QUARK_ROM_FATAL_KEY_MODULE_FUSE_COMPARE_FAIL  if the Digest of the Intel Key
                                                         in the Header does not match
                                                         that stored in the fuse.
  @retval  QUARK_ROM_FATAL_KEY_MODULE_VALIDATION_FAIL    if the RSA signature
                                                         is not successfully validated.
  @retval  QUARK_ROM_FATAL_INVALID_KEYBANK_NUM           If key bank number too large.
  @return  Other                                         Unexpected error.

**/
EFI_STATUS
EFIAPI
ValidateKeyModCallBack (
  const QuarkSecurityHeader_t * const Header,
  const UINT8 * const Data,
  const RSA2048PublicKey_t * const ModuleKey,
  const UINT8 * const Signature,
  ScratchMemory_t * const MemoryBuffer,
  const UINT8 KeyBankNumber
  )
{
  return QUARK_ROM_NO_ERROR;
}

