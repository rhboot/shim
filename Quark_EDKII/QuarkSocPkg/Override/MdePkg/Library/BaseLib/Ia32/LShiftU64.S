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
#   LShiftU64.S
#
# Abstract:
#
#   64-bit left shift function for IA-32
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(InternalMathLShiftU64)

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InternalMathLShiftU64 (
#   IN      UINT64                    Operand,
#   IN      UINTN                     Count
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalMathLShiftU64):
    movb    12(%esp), %cl
    xorl    %eax, %eax
    movl    4(%esp), %edx
    testb   $32, %cl                    # Count >= 32?
    jnz     L0
    movl    %edx, %eax
    movl    8(%esp), %edx
L0:        
    shld    %cl, %eax, %edx
    shl     %cl, %eax
    ret
