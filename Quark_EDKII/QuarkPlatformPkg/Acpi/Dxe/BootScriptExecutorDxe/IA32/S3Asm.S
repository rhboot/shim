## @file
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

#-----------------------------------------
#VOID
#AsmTransferControl (
#  IN   UINT32           S3WakingVector,
#  IN   UINT32           AcpiLowMemoryBase
#  );
#-----------------------------------------

ASM_GLOBAL ASM_PFX(AsmTransferControl)
ASM_PFX(AsmTransferControl):
    # S3WakingVector    :DWORD
    # AcpiLowMemoryBase :DWORD
    pushl %ebp
    movl  %esp,%ebp
    leal  LABLE, %eax
    pushl $0x28             # CS
    pushl %eax
    movl  8(%ebp),%ecx
    shrdl $20,%ecx,%ebx
    andl  $0xf,%ecx
    movw  %cx,%bx
    movl  %ebx, jmp_addr
    lret
LABLE: 
    .byte 0xb8,0x30,0       # mov ax, 30h as selector
    movw  %ax,%ds
    movw  %ax,%es
    movw  %ax,%fs
    movw  %ax,%gs
    movw  %ax,%ss
    movl  %cr0, %eax        # Get control register 0  
    .byte 0x66
    .byte 0x83,0xe0,0xfe    # and    eax, 0fffffffeh  ; Clear PE bit (bit #0)
    .byte 0xf,0x22,0xc0     # mov    cr0, eax         ; Activate real mode
    .byte 0xea              # jmp far @jmp_addr
jmp_addr: 
    .long    0

