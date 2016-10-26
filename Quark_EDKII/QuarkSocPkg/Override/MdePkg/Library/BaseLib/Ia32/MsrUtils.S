#------------------------------------------------------------------------------
#
# Copyright (c) 2013 Intel Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Intel Corporation nor the names of its
# contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Module Name:
#
#   ReadMsrUtils.S
#
# Abstract:
#
#   64-bit logical right shift function for IA-32
#
#------------------------------------------------------------------------------

    .686:
    .code:

ASM_GLOBAL ASM_PFX(AsmReadMsrReturnResult)
ASM_GLOBAL ASM_PFX(AsmReadMsrRealMsrAccess)
ASM_GLOBAL ASM_PFX(AsmWriteMsrRealMsrAccess)

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmReadMsrReturnResult (
#   IN      UINT32                    Index,
#   IN      UINT32                    QuarkMsrDataLow,
#   IN      UINT32                    QuarkMsrDataHigh
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadMsrReturnResult):
    movl    8(%esp), %eax               # QuarkMsrDataLow
    movl    4(%esp), %ecx              # Index
    movl    12(%esp), %edx               # QuarkMsrDataHigh
    ret
    
#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmReadMsrRealMsrAccess (
#   IN      UINT32                    Index
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmReadMsrRealMsrAccess):
    movl    4(%esp), %ecx              # Index
    movl    $0xFEE00900, %eax
    movl    $0x00000000, %edx
    cmpl    $0x0000001B, %ecx
    je      L0
    rdmsr
L0:    
    ret
    
#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# AsmWriteMsrRealMsrAccess (
#   IN      UINT32                    Index,
#   IN      UINT32                    QuarkMsrDataLow,
#   IN      UINT32                    QuarkMsrDataHigh
#   );
#------------------------------------------------------------------------------
ASM_PFX(AsmWriteMsrRealMsrAccess):
    movl    8(%esp), %eax               # QuarkMsrDataLow
    movl    4(%esp), %ecx              # Index
    movl    12(%esp), %edx               # QuarkMsrDataHigh
    wrmsr
    ret
        
