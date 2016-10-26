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
#   SmiEntry.S
#
# Abstract:
#
#   Code template of the SMI handler for a particular processor
#
#------------------------------------------------------------------------------


ASM_GLOBAL  ASM_PFX(gcSmiHandlerTemplate)
ASM_GLOBAL  ASM_PFX(gcSmiHandlerSize)
ASM_GLOBAL  ASM_PFX(gSmiCr3)
ASM_GLOBAL  ASM_PFX(gcSmiHandlerOffset)
ASM_GLOBAL  ASM_PFX(gSmiStack)
ASM_GLOBAL  ASM_PFX(gSmbase)
ASM_GLOBAL  ASM_PFX(FeaturePcdGet (PcdCpuSmmDebug))
ASM_GLOBAL  ASM_PFX(FeaturePcdGet (PcdCpuSmmStackGuard))

.equ            DSC_OFFSET, 0xfb00
.equ            DSC_GDTPTR, 0x30
.equ            DSC_GDTSIZ, 0x38
.equ            DSC_CS, 14
.equ            DSC_DS, 16
.equ            DSC_SS, 18
.equ            DSC_OTHERSEG, 20

.equ            TSS_SEGMENT, 0x40

    .data

 ASM_PFX(gcSmiHandlerOffset):  .word      _SmiHandler - _SmiEntryPoint + 0x8000

    .text

ASM_PFX(gcSmiHandlerTemplate):

_SmiEntryPoint:
    .byte 0xbb                          # mov bx, imm16
    .word _GdtDesc - _SmiEntryPoint + 0x8000
    .byte 0x2e,0xa1                     # mov ax, cs:[offset16]
    .word DSC_OFFSET + DSC_GDTSIZ
    decl    %eax
    movl    %eax, %cs:(%edi)             # mov cs:[bx], ax
    .byte 0x66,0x2e,0xa1                # mov eax, cs:[offset16]
    .word   DSC_OFFSET + DSC_GDTPTR
    movw    %ax, %cs:2(%edi)
    movw    %ax, %bp                    # ebp = GDT base
    .byte 0x66
    lgdt    %cs:(%edi)
    .byte 0x66,0xb8                     # mov eax, imm32
ASM_PFX(gSmiCr3):    .space 4
    movl    %eax, %cr3
    .byte 0x66
    movl    $0x020,%eax                 # as cr4.PGE is not set here, refresh cr3
    movl    %eax, %cr4                  # in PreModifyMtrrs() to flush TLB.
    .byte 0x2e,0xa1                     # mov ax, cs:[offset16]
    .word      DSC_OFFSET + DSC_CS
    movl    %eax, %cs:-2(%edi)
    .byte 0x66, 0xbf                    # mov edi, SMBASE
ASM_PFX(gSmbase): .space 4
    .byte 0x67
    lea     ((Start32bit - _SmiEntryPoint) + 0x8000)(%edi), %ax
    movw     %ax, %cs:-6(%edi)
    movl    %cr0, %ebx
    .byte 0x66
    andl    $0x9ffafff3, %ebx
    .byte 0x66
    orl     $0x80000023, %ebx
    movl    %ebx, %cr0
    .byte 0x66,0xea
    .space  4
    .space  2
_GdtDesc:   .space 4
            .space 2
Start32bit: 
    leal    DSC_OFFSET(%edi),%ebx
    movw    DSC_DS(%ebx),%ax
    movl    %eax,%ds
    movw    DSC_OTHERSEG(%ebx),%ax
    movl    %eax,%es
    movl    %eax,%fs
    movl    %eax,%gs
    movw    DSC_SS(%ebx),%ax
    movl    %eax,%ss

    cmpb    $0, ASM_PFX(FeaturePcdGet (PcdCpuSmmStackGuard))
    jz      L5

# Load TSS
    movb    $0x89, (TSS_SEGMENT + 5)(%ebp) # clear busy flag

    movl    $TSS_SEGMENT, %eax
    ltrw    %ax
L5:

#   jmp     _SmiHandler                 # instruction is not needed

_SmiHandler:
    .byte 0xbc                          # mov esp, imm32
ASM_PFX(gSmiStack):   .space  4
    cmpb    $0, ASM_PFX(FeaturePcdGet (PcdCpuSmmDebug))
    jz      L3
    call    L1
L1:
    popl    %ebp
    movl    $0x80000001, %eax
    cpuid
    btl     $29, %edx                   # check cpuid to identify X64 or IA32 
    leal    (0x7fc8 - (L1 - _SmiEntryPoint))(%ebp), %edi
    leal    4(%edi), %esi
    jnc     L2
    addl    $4, %esi
L2:
    movl    (%esi), %ecx
    movl    (%edi), %edx
L7:
    movl    %ecx, %dr6
    movl    %edx, %dr7                  # restore DR6 & DR7 before running C code
L3:

    pushl   (%esp)
    
    movl    $ASM_PFX(SmiRendezvous), %eax
    call    *%eax
    popl    %ecx


    cmpb    $0, ASM_PFX(FeaturePcdGet (PcdCpuSmmDebug))
    jz      L4

    movl    %dr6, %ecx
    movl    %dr7, %edx
    movl    %ecx, (%esi)
    movl    %edx, (%edi)
L4:
    rsm

ASM_PFX(gcSmiHandlerSize):    .word      . - _SmiEntryPoint

