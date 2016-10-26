;-------------------------------------------------------------------------------
;
; Copyright (c) 2013 Intel Corporation.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
;
; * Redistributions of source code must retain the above copyright
; notice, this list of conditions and the following disclaimer.
; * Redistributions in binary form must reproduce the above copyright
; notice, this list of conditions and the following disclaimer in
; the documentation and/or other materials provided with the
; distribution.
; * Neither the name of Intel Corporation nor the names of its
; contributors may be used to endorse or promote products derived
; from this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
; LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
; A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
; OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
; LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;
;
; Module Name:
;
;   MPFuncs32.asm
;
; Abstract:
;
;   This is the assembly code for MP (Multiple-processor) support
;
;-------------------------------------------------------------------------------

.686p
.model  flat
.code

include AsmInclude.inc
InitializeFloatingPointUnits PROTO C

EXTERNDEF   C   mBspSwitched:BYTE
EXTERNDEF   C   mNewProcessorNumber:DWORD
EXTERNDEF   C   mMonitorDataAddress:DWORD

;-------------------------------------------------------------------------------------
FJMP32  MACRO   Selector, Offset
            DB      066h
            DB      067h
            DB      0EAh            ; far jump
            DD      Offset          ; 32-bit offset
            DW      Selector        ; 16-bit selector
            ENDM
FCALL32 MACRO   Selector, Offset
            DB      09Ah
            DD      Offset          ; 32-bit offset
            DW      Selector        ; 16-bit selector
            ENDM
;-------------------------------------------------------------------------------------
;RendezvousFunnelProc  procedure follows. All APs execute their procedure. This
;procedure serializes all the AP processors through an Init sequence. It must be
;noted that APs arrive here very raw...ie: real mode, no stack.
;ALSO THIS PROCEDURE IS EXECUTED BY APs ONLY ON 16 BIT MODE. HENCE THIS PROC
;IS IN MACHINE CODE.
;-------------------------------------------------------------------------------------
;RendezvousFunnelProc (&WakeUpBuffer,MemAddress);

RendezvousFunnelProc   PROC  near C  PUBLIC
RendezvousFunnelProcStart::

; At this point CS = 0x(vv00) and ip= 0x0.

        db 66h,  08bh, 0e8h           ; mov        ebp, eax

        db 8ch,  0c8h                 ; mov        ax,  cs
        db 8eh,  0d8h                 ; mov        ds,  ax
        db 8eh,  0c0h                 ; mov        es,  ax
        db 8eh,  0d0h                 ; mov        ss,  ax
        db 33h,  0c0h                 ; xor        ax,  ax
        db 8eh,  0e0h                 ; mov        fs,  ax
        db 8eh,  0e8h                 ; mov        gs,  ax

; Get APIC ID

        db 66h,  0B8h
        dd 00000000h                  ; mov        eax, 0
        db 0Fh,  0A2h                 ; cpuid
        db 66h, 3dh
        dd 0000000Bh                  ; cmp        eax, 0b
        db 73h
        db X2Apic - $ - 1             ; jnb        X2Apic

; Processor is not x2APIC capable, so get 8-bit APIC ID
        db 66h,  0B8h
        dd 00000001h                  ; mov        eax, 1
        db 0Fh,  0A2h                 ; cpuid
        db 66h,  0C1h, 0EBh, 18h      ; shr        ebx, 24
        db 0ebh
        db CheckInitFlag - $ - 1      ; jmp CheckInitFlag

; Processor is x2APIC capable, so get 32-bit x2APIC ID
X2Apic::
        db 66h,  0B8h
        dd 0000000Bh                  ; mov        eax, 0b
        db 66h, 31h, 0c9h             ; xor        ecx, ecx
        db 0Fh,  0A2h                 ; cpuid
        db 66h, 89h, 0d3h             ; mov        ebx, edx

CheckInitFlag::
; EBX is keeping APIC ID
; If it is the first time AP wakes up, just record AP's BIST
; Otherwise, switch to flat mode

        db 0BEh
        dw InitFlag                   ; mov        si,  InitFlag
        db 66h,  83h, 3Ch, 00h        ; cmp        dword ptr [si], 0
        db 74h
        db flat32Start - $ - 1        ; jz         flat32Start

; Increase ApCount as processor number for index of BIST Info array

        db 66h, 0a1h                  ; mov        eax, [ApCountLocation]
        dw ApCountLocation
IncApCount::
        db 66h, 67h, 8dh, 48h, 01     ; lea        ecx, [eax+1]
        db 0f0h, 66h, 0fh, 0b1h, 0eh  ; lock       cmpxchg [ApCountLocation], ecx
        dw ApCountLocation
        db 75h
        db IncApCount - $ - 1         ; jnz        IncApCount

        db 66h, 0ffh, 0c0h            ; inc        eax                         ; AP processor number starts from 1

; Record BIST information
;
        db 66h, 67h, 8dh, 34h, 0c5h   ; lea esi, [BistBuffer + eax*8]
        dd BistBuffer

        db 66h, 89h, 1ch              ; mov        dword ptr [si], ebx         ; APIC ID
        db 66h,  89h,  6Ch,  04h      ; mov        dword ptr [si + 4], ebp     ; Store BIST value

        cli
        hlt
        jmp $-2

; Switch to flat mode.

flat32Start::

        db 0BEh
        dw BufferStart                ; mov        si, BufferStart
        db 66h,  8Bh, 0Ch             ; mov        ecx,dword ptr [si]          ; ECX is keeping the start address of wakeup buffer

        db 0FAh                       ; cli
        db 0BEh
        dw GdtrProfile                ; mov        si, GdtrProfile
        db 66h                        ; db         66h
        db 2Eh,0Fh, 01h, 14h          ; lgdt       fword ptr cs:[si]

        db 0BEh
        dw IdtrProfile                ; mov        si, IdtrProfile
        db 66h                        ; db         66h
        db 2Eh,0Fh, 01h, 1Ch          ; lidt       fword ptr cs:[si]

        db 33h, 0C0h                  ; xor        ax,  ax
        db 8Eh, 0D8h                  ; mov        ds,  ax
        db 0Fh, 20h, 0C0h             ; mov        eax, cr0                    ; Get control register 0
        db 66h, 83h, 0C8h, 01h        ; or         eax, 000000001h             ; Set PE bit (bit #0)
        db 0Fh, 22h, 0C0h             ; mov        cr0, eax


;step-4:

FLAT32_JUMP::
        FJMP32  010h,0h               ; Far jmp using code segment descriptor

PMODE_ENTRY::                         ; protected mode entry point

        mov         ax,  8h
        mov         ds,  ax
        mov         es,  ax
        mov         fs,  ax
        mov         gs,  ax
        mov         ss,  ax           ; Flat mode setup.

        mov         esi, ecx

        mov         edi, esi
        add         edi, InitFlag
        cmp         dword ptr [edi], 2                ; Check whether in S3 boot path
        jz          ProgramDynamicStack

ProgramStaticStack::

        ;
        ; Get processor number for this AP
        ; Note that BSP may become an AP due to SwitchBsp()
        ;

        xor         ecx, ecx
        lea         edi, [esi + BistBuffer]

GetProcNumber::
        cmp         [edi], ebx                       ; APIC ID match?
        jz          Found
        add         edi, 8
        inc         ecx
        cmp         ecx, dword ptr [esi + ApCountLocation]
        jbe         GetProcNumber

Found::
        ;
        ; ProgramStack
        ;

        mov         edi, esi
        add         edi, StackSize
        mov         eax, dword ptr [edi]
        inc         ecx
        mul         ecx                               ; EAX = StackSize * (CpuNumber + 1)
        dec         ecx

        mov         edi, esi
        add         edi, StackStart
        mov         ebx, dword ptr [edi]
        add         eax, ebx                          ; EAX = StackStart + StackSize * (CpuNumber + 1)

        mov         esp, eax
        sub         esp, MonitorFilterSize            ; Reserved Monitor data space
        mov         ebp, ecx                          ; Save processor number in ebp
        jmp         ProgramStackDone

ProgramDynamicStack::

        mov         edi, esi
        add         edi, LockLocation
        mov         al,  NotVacantFlag
TestLock::
        xchg        byte ptr [edi], al
        cmp         al,  NotVacantFlag
        jz          TestLock

        mov         edi, esi
        add         edi, StackSize
        mov         eax, dword ptr [edi]
        mov         edi, esi
        add         edi, StackStart
        add         eax, dword ptr [edi]
        mov         esp, eax
        mov         dword ptr [edi], eax

Releaselock::
        mov         al,  VacantFlag
        mov         edi, esi
        add         edi, LockLocation
        xchg        byte ptr [edi], al

ProgramStackDone::

        ;
        ; Call assembly function to initialize FPU.
        ;
        mov         ebx, InitializeFloatingPointUnits
        call        ebx

        ;
        ; Call C Function
        ;
        mov         edi, esi
        add         edi, RendezvousProc
        add         esi, ApLoopModeLocation              ; esi = ApLoopMode address location
                                                         
        xor         ebx, ebx                          
        mov         dword ptr [esp + 0Ch], ebx           ; Clean ReadyToBoot Flag
        mov         bx, bp                               ; bx = the lowest 16bit of CpuNumber
        or          ebx, StartupApSignal                 ; Construct AP run Singal

WakeUpThisAp::
        mov         eax, dword ptr [edi]
        test        eax, eax
        jz          CheckWakeUpManner

        push        ebp                                  ; Push processor number
        call        eax                                  ; Call C function
        add         esp, 4

        ;
        ; Check if BSP was switched
        ;
        mov         eax, offset mBspSwitched
        cmp         byte ptr [eax], 0
        jz          CheckReadyToBoot

        mov         byte ptr [eax], 0                 ;clear BSP switch flag

        mov         eax, offset mNewProcessorNumber
        mov         ebp, [eax]                        ; rbp = new processor number

        mov         bx, bp                            ; bx = the lowest 16bit of CpuNumber

        ;
        ; Assign the dedicated AP stack for the new AP
        ;
        mov         eax, offset mMonitorDataAddress
        mov         esp, [eax]

CheckReadyToBoot::
        mov         eax, dword ptr [esi]                 ; eax = ApLoopMode for POST
        cmp         byte ptr [esp + 0Ch], 1                ; Check ReadyToBoot flag?
        jnz         CheckWakeUpManner

        mov         eax, ApInHltLoop
        cmp         byte ptr [esp + 0Dh], 1              ; Check if C-State enable?
        jnz         CheckWakeUpManner

        mov         eax, ApInMwaitLoop                   ; eax = ApLoopMode for Read To Boot
         
CheckWakeUpManner::
        cli
        cmp         eax, ApInHltLoop
        jz          HltApLoop

        cmp         eax, ApInMwaitLoop
        jnz         CheckRunSignal

ApMwaitLoop::
        mov         eax, esp                      ; Set Monitor Address
        xor         ecx, ecx                      ; ecx = 0
        xor         edx, edx                      ; edx = 0
        DB          0fh, 1, 0c8h                  ; MONITOR

        mov         eax, dword ptr [esp + 4]      ; Mwait Cx, Target C-State per eax[7:4]
        DB          0fh, 1, 0c9h                  ; MWAIT

CheckRunSignal::
        cmp         dword ptr [esp], ebx          ; Check if run signal correct?
        jz          WakeUpThisAp                  ; Jmp to execute AP task
        PAUSE32
        jmp         CheckReadyToBoot              ; Unknown break, go checking run manner

HltApLoop::
        cli
        hlt
        jmp         HltApLoop                     ; Jump to halt loop

RendezvousFunnelProc   ENDP
RendezvousFunnelProcEnd::
;-------------------------------------------------------------------------------------
;  AsmGetAddressMap (&AddressMap);
;-------------------------------------------------------------------------------------
AsmGetAddressMap   PROC  near C  PUBLIC

        pushad
        mov         ebp,esp

        mov         ebx, dword ptr [ebp+24h]
        mov         dword ptr [ebx], RendezvousFunnelProcStart
        mov         dword ptr [ebx+4h], PMODE_ENTRY - RendezvousFunnelProcStart
        mov         dword ptr [ebx+8h], FLAT32_JUMP - RendezvousFunnelProcStart
        mov         dword ptr [ebx+0ch], RendezvousFunnelProcEnd - RendezvousFunnelProcStart

        popad
        ret
AsmGetAddressMap   ENDP

;-------------------------------------------------------------------------------------
;AsmExchangeRole procedure follows. This procedure executed by current BSP, that is
;about to become an AP. It switches it'stack with the current AP.
;AsmExchangeRole (IN   CPU_EXCHANGE_INFO    *MyInfo, IN   CPU_EXCHANGE_INFO    *OthersInfo);
;-------------------------------------------------------------------------------------
CPU_SWITCH_STATE_IDLE          equ        0
CPU_SWITCH_STATE_STORED        equ        1
CPU_SWITCH_STATE_LOADED        equ        2

AsmExchangeRole   PROC  near C  PUBLIC
        ; DO NOT call other functions in this function, since 2 CPU may use 1 stack
        ; at the same time. If 1 CPU try to call a functiosn, stack will be corrupted.
        pushad
        mov         ebp,esp

        ; esi contains MyInfo pointer
        mov         esi, dword ptr [ebp+24h]

        ; edi contains OthersInfo pointer
        mov         edi, dword ptr [ebp+28h]

        ;Store EFLAGS, GDTR and IDTR regiter to stack
        pushfd
        sgdt        fword ptr [esi+8]
        sidt        fword ptr [esi+14]

        ; Store the its StackPointer
        mov         dword ptr [esi+4],esp

        ; update its switch state to STORED
        mov         byte ptr [esi], CPU_SWITCH_STATE_STORED

WaitForOtherStored::
        ; wait until the other CPU finish storing its state
        cmp         byte ptr [edi], CPU_SWITCH_STATE_STORED
        jz          OtherStored
        pause
        jmp         WaitForOtherStored

OtherStored::
        ; Since another CPU already stored its state, load them
        ; load GDTR value
        lgdt        fword ptr [edi+8]

        ; load IDTR value
        lidt        fword ptr [edi+14]

        ; load its future StackPointer
        mov         esp, dword ptr [edi+4]

        ; update the other CPU's switch state to LOADED
        mov         byte ptr [edi], CPU_SWITCH_STATE_LOADED

WaitForOtherLoaded::
        ; wait until the other CPU finish loading new state,
        ; otherwise the data in stack may corrupt
        cmp         byte ptr [esi], CPU_SWITCH_STATE_LOADED
        jz          OtherLoaded
        pause
        jmp         WaitForOtherLoaded

OtherLoaded::
        ; since the other CPU already get the data it want, leave this procedure
        popfd

        popad
        ret
AsmExchangeRole   ENDP
END
