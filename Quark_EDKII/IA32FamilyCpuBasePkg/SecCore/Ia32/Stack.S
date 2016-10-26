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
# Abstract:
#
#   Switch the stack from temporary memory to permenent memory.
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# VOID
# EFIAPI
# SecSwitchStack (
#   UINT32   TemporaryMemoryBase,
#   UINT32   PermenentMemoryBase
#   )#
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX (SecSwitchStack)
ASM_PFX(SecSwitchStack):
    #
    # Save standard registers so they can be used to change stack
    #
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx

    #
    # !!CAUTION!! this function address's is pushed into stack after
    # migration of whole temporary memory, so need save it to permenent
    # memory at first!
    #
    movl  20(%esp), %ebx         # Save the first parameter
    movl  24(%esp), %ecx         # Save the second parameter

    #
    # Save this function's return address into permenent memory at first.
    # Then, Fixup the esp point to permenent memory
    #
    movl  %esp, %eax
    subl  %ebx, %eax
    addl  %ecx, %eax
    movl  0(%esp), %edx          # copy pushed register's value to permenent memory
    movl  %edx, 0(%eax)
    movl  4(%esp), %edx
    movl  %edx, 4(%eax)
    movl  8(%esp), %edx
    movl  %edx, 8(%eax)
    movl  12(%esp), %edx
    movl  %edx, 12(%eax)
    movl  16(%esp), %edx        # Update this function's return address into permenent memory
    movl  %edx, 16(%eax)
    movl  %eax, %esp            # From now, esp is pointed to permenent memory

    #
    # Fixup the ebp point to permenent memory
    #
    movl  %ebp, %eax
    subl  %ebx, %eax
    addl  %ecx, %eax
    movl  %eax, %ebp            # From now, ebp is pointed to permenent memory

    popl  %edx
    popl  %ecx
    popl  %ebx
    popl  %eax
    ret


