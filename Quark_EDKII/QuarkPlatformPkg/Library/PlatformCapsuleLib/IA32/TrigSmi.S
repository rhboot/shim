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
#   TrigSmi.S
#
# Abstract:
#
#   TrigSmi for capsule update. SMI number is 0x27 and 0x28.
#
# Notes:
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
#  VOID
#  SendCapsuleSmi (
#    UINTN Addr
#    )
#------------------------------------------------------------------------------
ASM_GLOBAL  ASM_PFX(SendCapsuleSmi)
ASM_PFX(SendCapsuleSmi):
  pushl %ecx
  movl  8(%esp), %ecx
  pushl %ebp
  movl  %esp, %ebp
  subl  $8, %esp
  pushl %eax
  pushl %ebx
  movl  %ecx, %ebx
  movl  $0xEF27, %eax
  outb  %al, $0xB2
  popl  %ebx
  popl  %eax
  movl  %ebp, %esp
  popl  %ebp
  popl  %ecx
  ret   $0


#------------------------------------------------------------------------------
#  VOID
#  GetUpdateStatusSmi (
#    UINTN Addr
#    )
#------------------------------------------------------------------------------
ASM_GLOBAL  ASM_PFX(GetUpdateStatusSmi)
ASM_PFX(GetUpdateStatusSmi):
  pushl %ecx
  movl  8(%esp), %ecx
  pushl %ebp
  movl  %esp, %ebp
  subl  $8, %esp
  pushl %eax
  pushl %ebx
  movl  %ecx, %ebx
  movl  $0xEF28, %eax
  outb  %al, $0xB2
  popl  %ebx
  popl  %eax
  movl  %ebp, %esp
  popl  %ebp
  popl  %ecx
  ret   $0
