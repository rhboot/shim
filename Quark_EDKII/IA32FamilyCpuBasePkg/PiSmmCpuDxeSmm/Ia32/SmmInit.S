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
#   SmmInit.S
#
# Abstract:
#
#   Functions for relocating SMBASE's for all processors
#
# Notes:
#
#------------------------------------------------------------------------------


ASM_GLOBAL   ASM_PFX(gSmmCr0)
ASM_GLOBAL   ASM_PFX(gSmmCr3)
ASM_GLOBAL   ASM_PFX(gSmmCr4)
ASM_GLOBAL   ASM_PFX(gcSmmInitTemplate)
ASM_GLOBAL   ASM_PFX(gcSmmInitSize)
ASM_GLOBAL   ASM_PFX(gSmmJmpAddr)
ASM_GLOBAL   ASM_PFX(SmmRelocationSemaphoreComplete)
ASM_GLOBAL   ASM_PFX(gSmmInitStack)

    .data

NullSeg:    .quad      0
DataSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x93
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
CodeSeg32:
            .word -1                    # LimitLow
            .word 0                     # BaseLow
            .byte 0                     # BaseMid
            .byte 0x9b
            .byte 0xcf                  # LimitHigh
            .byte 0                     # BaseHigh
.equ  GDT_SIZE, . - NullSeg

    .text

GdtDesc:
            .word      GDT_SIZE
            .long      NullSeg

SmmStartup:
    .byte   0x66,0xb8
ASM_PFX(gSmmCr3):    .space     4
    movl    %eax, %cr3
    .byte   0x67,0x66
    lgdt    %cs:(GdtDesc - SmmStartup)(%ebp)
    .byte   0x66,0xb8
ASM_PFX(gSmmCr4):    .space     4
    movl    %eax, %cr4
    .byte   0x66,0xb8
ASM_PFX(gSmmCr0):    .space     4
    .byte   0xbf,8,0                      # mov di, 8
    movl    %eax, %cr0
    .byte   0x66,0xea                     # jmp far [ptr48]
ASM_PFX(gSmmJmpAddr): .long Start32bit
    .word   0x10
Start32bit:
    movl    %edi,%ds
    movl    %edi,%es
    movl    %edi,%fs
    movl    %edi,%gs
    movl    %edi,%ss
    .byte   0xbc                        # mov esp, imm32
ASM_PFX(gSmmInitStack):  .space  4
    call    ASM_PFX(SmmInitHandler)
    rsm

ASM_PFX(gcSmmInitTemplate):

_SmmInitTemplate:
    .byte 0x66
    movl    $SmmStartup, %ebp
    .byte 0x66, 0x81, 0xed, 0, 0, 3, 0  # sub ebp, 0x30000
    jmp     *%bp                        # jmp ebp actually

ASM_PFX(gcSmmInitSize):   .word  . - ASM_PFX(gcSmmInitTemplate)


ASM_PFX(SmmRelocationSemaphoreComplete):
    pushl   %eax
    movl    ASM_PFX(mRebasedFlag), %eax
    movb    $1, (%eax)
    popl    %eax
    jmp     *ASM_PFX(mSmmRelocationOriginalAddress)

