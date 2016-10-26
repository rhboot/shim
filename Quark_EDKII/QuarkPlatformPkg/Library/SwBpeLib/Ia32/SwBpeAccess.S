#++
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
# 
# Module Name:
#
#   SwbpeLibAccess.S
# 
# Abstract:
# 
#   This file provides low level I/O access functions to trigger
#   breakpoint events.
#
#--
  
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#  Procedure:   DoSwbpeTypeIoOut
# 
#  Input:       PortAddress   -  Port Address for doing IO Write
#               PortData      -  Data value for doing IO Write
#               PortDataWidth -  Data Width (Byte=1/Word=2/Dword=4) for doing IO Write
#               Mailbox       -  Data structure contains Data Store informations
# 
#  Output:      Debuggers can update structure pointed by %edi via this IO Write Break.
# 
#  Registers:   All are preserved
# 
#  Description: Function to do an IO Write and set %edi %edi pointer to Mailbox structure,
#               so Debuggers can recognize the GUID and update status in the structure.
# 
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
ASM_GLOBAL  ASM_PFX(DoSwbpeTypeIoOut)
ASM_PFX(DoSwbpeTypeIoOut):        
    pushl   %edi
    movl    8(%esp), %edx   # PortAddress
    movl    12(%esp), %eax  # PortData
    movl    16(%esp), %ecx  # PortDataWidth
    movl    20(%esp), %edi  # Mailbox
    cmpl    $1, %ecx
    jne     TryIoOutWord
    outb    %al, %dx
    jmp     IoOutReturn
TryIoOutWord:
    cmpl    $2, %ecx
    jne     TryIoOutDword
    outw    %ax, %dx
    jmp     IoOutReturn
TryIoOutDword:
    cmpl    $4, %ecx
    jne     IoOutReturn
    outl    %eax, %dx
IoOutReturn:
    popl    %edi
    ret 
        
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#  Procedure:   DoSwbpeTypeIoIn
# 
#  Input:       PortAddress   -  Port Address for doing IO Read
#               PortDataWidth -  Port data width for doing IO Read
#               Mailbox       -  Data structure contains Data Store informations
# 
#  Output:      Debuggers can update structure pointed by %edi via this IO Read Break,
#               %eax contains IO Read byte/word/dword value depends on PortDataWidth.
# 
#  Registers:   All are preserved
# 
#  Description: Function to do an IO Read and set %edi pointer to Mailbox structure,
#               so Debuggers can recognize the GUID and update status in the structure.
# 
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
ASM_GLOBAL  ASM_PFX(DoSwbpeTypeIoIn)
ASM_PFX(DoSwbpeTypeIoIn):    
    pushl   %edi
    xor     %eax, %eax
    movl    8(%esp), %edx  # PortAddress
    movl    12(%esp), %ecx # PortDataWidth
    movl    16(%esp), %edi # Mailbox
    cmp     $1, %ecx
    jne     TryIoInWord
    inb     %dx, %al
    jmp     IoInReturn
TryIoInWord:
    cmpl    $2,  %ecx
    jne     TryIoInDword
    inw     %dx, %ax
    jmp     IoInReturn
TryIoInDword:
    cmpl    $4, %ecx
    jne     IoInReturn
    inl     %dx, %eax
IoInReturn:
    popl    %edi
    ret 

        
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#  Procedure:   DoSwbpeTypeMemWrite
# 
#  Input:       MemAddress   -  Memory Address for doing MEM Write
#               MemData      -  Data for doing Memory Write
#               MemDataWidth -  Write Data Width (Byte=1/Word=2/DWord=4)
#               Mailbox      -  Data structure contains Data Store info
# 
#  Output:      Debuggers can update structure pointed by %edi via this MEM Write Break
# 
#  Registers:   All are preserved
# 
#  Description: Function to do Memory Write and set %edi pointer to Mailbox structure,
#               so Debuggers can recognize the GUID and update status in the structure.
# 
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
ASM_GLOBAL  ASM_PFX(DoSwbpeTypeMemWrite)
ASM_PFX(DoSwbpeTypeMemWrite):    
    pushl    %edi    
    movl     8(%esp), %edx  # MemAddress
    movl     12(%esp),%eax  # MemData
    movl     16(%esp),%ecx  # MemDataWidth
    movl     20(%esp),%edi  # Mailbox   
    cmpl     $1, %ecx
    jne      TryMemWriteWord
    movb     %al, (%edx)
    jmp      MemWriteReturn
TryMemWriteWord:
    cmpl     $2, %ecx
    jne      TryMemWriteDword
    movw     %ax, (%edx)
    jmp      MemWriteReturn    
TryMemWriteDword:
    cmpl     $4, %ecx
    jne      MemWriteReturn
    movl     %eax, (%edx)
MemWriteReturn:
    popl     %edi
    ret     
        
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#  Procedure:   DoSwbpeTypeMemRead
# 
#  Input:       MemAddress   -  Memory Address for doing MEM Write
#               MemDataWidth -  Write Data Width (Byte=1/Word=2/DWord=4)
#               Mailbox      -  Data structure contains Data Store info
# 
#  Output:      Debuggers can update structure pointed by %edi via this MEM Read Break,
#               %eax contains MEM Read byte/word/dword value depends on PortDataWidth.
# 
#  Registers:   All are preserved
# 
#  Description: Function to do Memory Read and set %edi pointer to Mailbox structure,
#               so Debuggers can recognize the GUID and update status in the structure.
# 
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
ASM_GLOBAL  ASM_PFX(DoSwbpeTypeMemRead)
ASM_PFX(DoSwbpeTypeMemRead):
    pushl    %edi
    xorl     %eax, %eax    
    movl     8(%esp),  %edx   # MemAddress
    movl     12(%esp), %ecx   # MemDataWidth    
    movl     16(%esp), %edi   # Mailbox
    cmpl     $1, %ecx
    jne      TryMemReadWord
    movb     (%edx), %al
    jmp      MemReadReturn    
TryMemReadWord:
    cmpl     $2, %ecx
    jne      TryMemReadDword
    movw     (%edx), %ax
    jmp      MemReadReturn    
TryMemReadDword:
    cmpl     $4, %ecx
    jne      MemReadReturn
    movl     (%edx), %eax    
MemReadReturn:
    popl     %edi
    ret 

        
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#  Procedure:   DoSwbpeTypeInt3
# 
#  Input:       Mailbox -  Data structure contains Data Store informations
# 
#  Output:      Debuggers can update structure pointed by %edi via this Interrupt Break
# 
#  Registers:   All are preserved
# 
#  Description: Function to do Interrupt 3 and set %edi pointer to Mailbox structure,
#               so Debuggers can recognize the GUID and update status in the structure.
# 
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
ASM_GLOBAL  ASM_PFX(DoSwbpeTypeInt3)
ASM_PFX(DoSwbpeTypeInt3):
    pushl    %edi
    movl     8(%esp), %edi
    int      $3
    popl     %edi
    ret       
