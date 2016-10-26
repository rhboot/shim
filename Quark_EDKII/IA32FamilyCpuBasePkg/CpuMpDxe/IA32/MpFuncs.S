#-------------------------------------------------------------------------------
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
#   MPFuncs32.S
#
# Abstract:
#
#   This is the assembly code for MP (Multiple-processor) support
#
#-------------------------------------------------------------------------------


.equ                   VacantFlag,                0x0
.equ                   NotVacantFlag,             0xff
.equ                   StartupApSignal,           0x6E750000
.equ                   MonitorFilterSize,         0x10
.equ                   ApInHltLoop,               1
.equ                   ApInMwaitLoop,             2
.equ                   ApInRunLoop,               3


.equ                   LockLocation,      RendezvousFunnelProcEnd - RendezvousFunnelProcStart
.equ                   StackStart,        RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x04
.equ                   StackSize,         RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x08
.equ                   RendezvousProc,    RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x0C
.equ                   GdtrProfile,       RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x10
.equ                   IdtrProfile,       RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x16
.equ                   BufferStart,       RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x1C
.equ                   Cr3Location,       RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x20
.equ                   InitFlag,          RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x24
.equ                   ApCountLocation,   RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x28
.equ                   ApLoopModeLocation,RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x2C
.equ                   BistBuffer,        RendezvousFunnelProcEnd - RendezvousFunnelProcStart + 0x30

#-------------------------------------------------------------------------------------
.macro  PAUSE32
            .byte 0xF3
            .byte 0x90
.endm

#-------------------------------------------------------------------------------------
#RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
#procedure serializes all the AP processors through an Init sequence. It must be
#noted that APs arrive here very raw...ie: real mode, no stack.
#ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
#IS IN MACHINE CODE.
#-------------------------------------------------------------------------------------
#RendezvousFunnelProc (&WakeUpBuffer,MemAddress);

ASM_GLOBAL ASM_PFX(RendezvousFunnelProc)
ASM_PFX(RendezvousFunnelProc):
RendezvousFunnelProcStart:


# At this point CS = 0x(vv00) and ip= 0x0.

        .byte 0x66,0x8b,0xe8          # mov        ebp, eax

        .byte 0x8c,0xc8               # mov        ax,  cs
        .byte 0x8e,0xd8               # mov        ds,  ax
        .byte 0x8e,0xc0               # mov        es,  ax
        .byte 0x8e,0xd0               # mov        ss,  ax
        .byte 0x33,0xc0               # xor        ax,  ax
        .byte 0x8e,0xe0               # mov        fs,  ax
        .byte 0x8e,0xe8               # mov        gs,  ax

# Get APIC ID

        .byte 0x66,0xB8
        .long 0x00000000              # mov        eax, 0
        .byte 0x0F,0xA2               # cpuid
        .byte 0x66,0x3d
        .long 0x0000000B              # cmp        eax, 0b
        .byte 0x73,0x0e               # jnb        X2Apic

# Processor is not x2APIC capable, so get 8-bit APIC ID
        .byte 0x66,0xB8               # mov        eax, 1
        .long 0x00000001              # cpuid
        .byte 0x0F,0xA2
        .byte 0x66,0xC1,0xEB,0x18     # shr        ebx, 24
        .byte 0xeb,0x0e               # jmp CheckInitFlag

# Processor is x2APIC capable, so get 32-bit x2APIC ID
X2Apic:
        .byte 0x66,0xB8
        .long 0x0000000B              # mov        eax, 0b
        .byte 0x66,0x31,0xc9          # xor        ecx, ecx
        .byte 0x0F,0xA2               # cpuid
        .byte 0x66,0x89,0xd3          # mov        ebx, edx

CheckInitFlag:
# If it is the first time AP wakes up, just record AP's BIST
# Otherwise, switch to flat mode

        .byte 0xBE
        .word InitFlag                # mov        si,  InitFlag
        .byte 0x66,0x83,0x3C,0x0      # cmp        dword ptr [si], 0
        .byte 0x74
        .byte flat32Start - . - 1     # jz         flat32Start

# Increase ApCount as processor number for index of BIST Info array

        .byte 0x66,0xa1                  # mov        eax, [ApCountLocation]
        .word ApCountLocation
IncApCount:
        .byte 0x66,0x67,0x8d,0x48,0x01   # lea        ecx, [eax+1]
        .byte 0xf0,0x66,0x0f,0xb1,0x0e   # lock       cmpxchg [ApCountLocation], ecx
        .word ApCountLocation
        .byte 0x75,0xf2                  # jnz        IncApCount
        .byte 0x66, 0xff, 0xc0           # inc        eax                         ; AP processor number starts from 1
# Record BIST information
#
        .byte 0x66,0x67,0x8d,0x34,0xc5   # lea esi, [BistBuffer + eax*8]
        .long BistBuffer

        .byte 0x66,0x89,0x1c             # mov        dword ptr [si], ebx         ; APIC ID
        .byte 0x66,0x89,0x6C,0x04        # mov        dword ptr [si + 4], ebp     ; Store BIST value

        cli
        hlt
        jmp .-2

# Switch to flat mode.

flat32Start:

        .byte 0xBE
        .word BufferStart             # mov        si, BufferStart
        .byte 0x66,0x8B,0x0C          # mov        ecx,dword ptr [si]          ; ECX is keeping the start address of wakeup buffer

        .byte 0xFA                    # cli

        .byte 0xBE
        .word GdtrProfile             # mov        si, GdtrProfile
        .byte 0x66                    # db         66h
        .byte 0x2E,0xF,0x1,0x14       # lgdt       fword ptr cs:[si]

        .byte 0xBE
        .word IdtrProfile             # mov        si, IdtrProfile
        .byte 0x66                    # db         66h
        .byte 0x2E,0xF,0x1,0x1C       # lidt       fword ptr cs:[si]

        .byte 0x33,0xC0               # xor        ax,  ax
        .byte 0x8E,0xD8               # mov        ds,  ax

        .byte 0xF,0x20,0xC0           # mov        eax, cr0                    ; Get control register 0
        .byte 0x66,0x83,0xC8,0x1      # or         eax, 000000001h             ; Set PE bit (bit #0)
        .byte 0xF,0x22,0xC0           # mov        cr0, eax


#step-4:

FLAT32_JUMP:

        .byte 0x66,0x67,0xEA          # far jump
        .long 0x0
        .word 0x10

PMODE_ENTRY:                          # protected mode entry point

        movw        $0x8,%ax
        .byte       0x66
        movw        %ax,%ds
        .byte       0x66
        movw        %ax,%es
        .byte       0x66
        movw        %ax,%fs
        .byte       0x66
        movw        %ax,%gs
        .byte       0x66
        movw        %ax,%ss           # Flat mode setup.

        movl        %ecx,%esi

        movl        %esi,%edi
        addl        $InitFlag, %edi
        cmpl        $2, (%edi)
        jz          ProgramDynamicStack

ProgramStaticStack:

        #
        # Get processor number for this AP
        # Note that BSP may become an AP due to SwitchBsp()
        #
        xorl        %ecx, %ecx
        leal        BistBuffer(%esi), %edi

GetProcNumber:
        cmpl        %ebx, (%edi)                     # APIC ID match?
        jz          Found
        addl        $8, %edi
        incl        %ecx
        cmpl        ApCountLocation(%esi), %ecx
        jbe         GetProcNumber

Found:
        #
        # ProgramStack
        #

        movl        %esi, %edi
        addl        $StackSize, %edi
        movl        (%edi), %eax
        incl        %ecx
        mull        %ecx                              # EAX = StackSize * (CpuNumber + 1)
        decl        %ecx

        movl        %esi, %edi
        addl        $StackStart, %edi
        movl        (%edi), %ebx
        addl        %ebx, %eax                        # EAX = StackStart + StackSize * (CpuNumber + 1)

        movl        %eax, %esp
        subl        $MonitorFilterSize, %esp          # Reserved Monitor data space
        movl        %ecx, %ebp                        # Save processor number in ebp
        jmp         ProgramStackDone

ProgramDynamicStack:

        movl        %esi,%edi
        addl        $LockLocation, %edi
        movb        $NotVacantFlag, %al
TestLock:
        xchgb       (%edi), %al
        cmpb        $NotVacantFlag, %al
        jz          TestLock

        movl        %esi,%edi
        addl        $StackSize, %edi
        movl        (%edi), %eax
        movl        %esi,%edi
        addl        $StackStart, %edi
        addl        (%edi), %eax
        movl        %eax,%esp
        movl        %eax, (%edi)

Releaselock:
        movb        $VacantFlag, %al
        movl        %esi,%edi
        addl        $LockLocation, %edi
        xchgb       (%edi), %al

ProgramStackDone:

        #
        # Call assembly function to initialize FPU.
        #
        lea         ASM_PFX(InitializeFloatingPointUnits), %ebx
        call        *%ebx

        #
        # Call C Function
        #
        movl        %esi,%edi
        addl        $RendezvousProc, %edi
        addl        $ApLoopModeLocation, %esi         # esi = ApLoopMode address location

        xorl        %ebx, %ebx                          
        movl        %ebx, 0xC(%esp)                   # Clean ReadyToBoot Flag
        movw        %bp, %bx                          # bx = the lowest 16bit of CpuNumber
        orl         $StartupApSignal, %ebx            # Construct AP run Singal
        
WakeUpThisAp:
        movl        (%edi), %eax
        testl       %eax, %eax
        jz          CheckReadyToBoot

        push        %ebp
        call        *%eax
        addl        $4, %esp

        #
        # Check if BSP was switched
        #
        movl        ASM_PFX(mBspSwitched), %eax
        cmpb        $0, (%eax)
        jz          CheckReadyToBoot

        movb        $0, (%eax)                    # clear BSP switch flag

        movl        ASM_PFX(mNewProcessorNumber), %eax
        movl        (%eax), %ebp                  # rbp = new processor number

        movw        %bp, %bx                      # bx = the lowest 16bit of CpuNumber

        #
        # Assign the dedicated AP stack for the new AP
        #
        movl        ASM_PFX(mMonitorDataAddress), %eax
        movl        (%eax), %esp

CheckReadyToBoot:
        movl        (%esi), %eax                  # eax = ApLoopMode for POST
        cmpb        $1, 0xC(%esp)                 # Check ReadyToBoot flag?
        jnz         CheckWakeUpManner

        movl        $ApInHltLoop, %eax
        cmpl        $1, 0xD(%esp)                 # Check if C-State enable?
        jnz         CheckWakeUpManner

        movl        ApInMwaitLoop, %eax           # eax = ApLoopMode for Read To Boot
         
CheckWakeUpManner:
        cli
        cmpl        $ApInHltLoop, %eax
        jz          HltApLoop

        cmpl        $ApInMwaitLoop, %eax
        jnz         CheckRunSignal

ApMwaitLoop:
        movl        %esp, %eax                    # Set Monitor Address
        xorl        %ecx, %ecx                    # ecx = 0
        xorl        %edx, %edx                    # edx = 0
        .byte       0x0f,0x1,0xc8                 # MONITOR

        movl        4(%esp), %eax                 # Mwait Cx, Target C-State per eax[7:4]
        .byte       0x0f,0x1,0xc9                 # MWAIT

CheckRunSignal:
        cmpl        %ebx, (%esp)                  # Check if run signal correct?
        jz          WakeUpThisAp                  # Jmp to execute AP task
        PAUSE32
        jmp         CheckReadyToBoot              # Unknown break, go checking run manner

HltApLoop:
        cli
        hlt
        jmp         HltApLoop                     # Jump to halt loop

RendezvousFunnelProcEnd:
#-------------------------------------------------------------------------------------
#  AsmGetAddressMap (&AddressMap);
#-------------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(AsmGetAddressMap)
ASM_PFX(AsmGetAddressMap):

        pushal
        movl        %esp,%ebp

        movl        0x24(%ebp), %ebx
        movl        $RendezvousFunnelProcStart, (%ebx)
        movl        $(PMODE_ENTRY - RendezvousFunnelProcStart), 0x4(%ebx)
        movl        $(FLAT32_JUMP - RendezvousFunnelProcStart), 0x8(%ebx)
        movl        $(RendezvousFunnelProcEnd - RendezvousFunnelProcStart), 0x0c(%ebx)

        popal
        ret

#-------------------------------------------------------------------------------------
#AsmExchangeRole procedure follows. This procedure executed by current BSP, that is
#about to become an AP. It switches it'stack with the current AP.
#AsmExchangeRole (IN   CPU_EXCHANGE_INFO    *MyInfo, IN   CPU_EXCHANGE_INFO    *OthersInfo);
#-------------------------------------------------------------------------------------
.equ                           CPU_SWITCH_STATE_IDLE, 0
.equ                           CPU_SWITCH_STATE_STORED, 1
.equ                           CPU_SWITCH_STATE_LOADED, 2

ASM_GLOBAL ASM_PFX(AsmExchangeRole)
ASM_PFX(AsmExchangeRole):
        # DO NOT call other functions in this function, since 2 CPU may use 1 stack
        # at the same time. If 1 CPU try to call a functiosn, stack will be corrupted.
        pushal
        movl        %esp,%ebp

        # esi contains MyInfo pointer
        movl        0x24(%ebp), %esi

        # edi contains OthersInfo pointer
        movl        0x28(%ebp), %edi

        #Store EFLAGS, GDTR and IDTR regiter to stack
        pushfl
        sgdt        8(%esi)
        sidt        14(%esi)

        # Store the its StackPointer
        movl        %esp, 4(%esi)

        # update its switch state to STORED
        movb        $CPU_SWITCH_STATE_STORED, (%esi)

WaitForOtherStored:
        # wait until the other CPU finish storing its state
        cmpb        $CPU_SWITCH_STATE_STORED, (%edi)
        jz          OtherStored
        pause
        jmp         WaitForOtherStored

OtherStored:
        # Since another CPU already stored its state, load them
        # load GDTR value
        lgdt        8(%edi)

        # load IDTR value
        lidt        14(%edi)

        # load its future StackPointer
        movl        4(%edi), %esp

        # update the other CPU's switch state to LOADED
        movb        $CPU_SWITCH_STATE_LOADED, (%edi)

WaitForOtherLoaded:
        # wait until the other CPU finish loading new state,
        # otherwise the data in stack may corrupt
        cmpb        $CPU_SWITCH_STATE_LOADED, (%esi)
        jz          OtherLoaded
        pause
        jmp         WaitForOtherLoaded

OtherLoaded:
        # since the other CPU already get the data it want, leave this procedure
        popfl

        popal
        ret

