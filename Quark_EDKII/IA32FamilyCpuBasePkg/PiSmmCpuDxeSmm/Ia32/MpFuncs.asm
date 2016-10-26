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
;   MpFuncs.asm
; 
; Abstract:
; 
;   This is the assembly code for Multi-processor S3 support
;
;-------------------------------------------------------------------------------

.686p
.model  flat,C       
.code        

EXTERN  InitializeFloatingPointUnits:PROC

VacantFlag             Equ   00h
NotVacantFlag          Equ   0ffh

LockLocation        equ        RendezvousFunnelProcEnd - RendezvousFunnelProcStart
StackStart          equ        LockLocation + 4h
StackSize           equ        LockLocation + 8h
RendezvousProc      equ        LockLocation + 0Ch
GdtrProfile         equ        LockLocation + 10h
IdtrProfile         equ        LockLocation + 16h
BufferStart         equ        LockLocation + 1Ch

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

        db 8ch,  0c8h                 ; mov        ax,  cs
        db 8eh,  0d8h                 ; mov        ds,  ax
        db 8eh,  0c0h                 ; mov        es,  ax
        db 8eh,  0d0h                 ; mov        ss,  ax 
        db 33h,  0c0h                 ; xor        ax,  ax
        db 8eh,  0e0h                 ; mov        fs,  ax
        db 8eh,  0e8h                 ; mov        gs,  ax

flat32Start::

        db 0BEh
        dw BufferStart                ; mov        si, BufferStart
        db 66h,  8Bh, 14h             ; mov        edx,dword ptr [si]          ; EDX is keeping the start address of wakeup buffer

        db 0BEh
        dw GdtrProfile                ; mov        si, GdtrProfile
        db 66h                        ; db         66h
        db 2Eh,  0Fh, 01h, 14h        ; lgdt       fword ptr cs:[si]

        db 0BEh
        dw IdtrProfile                ; mov        si, IdtrProfile
        db 66h                        ; db         66h
        db 2Eh,  0Fh, 01h, 1Ch        ; lidt       fword ptr cs:[si]
        
        db 33h,  0C0h                 ; xor        ax,  ax
        db 8Eh,  0D8h                 ; mov        ds,  ax
        
        db 0Fh,  20h, 0C0h            ; mov        eax, cr0                    ; Get control register 0
        db 66h,  83h, 0C8h, 01h       ; or         eax, 000000001h             ; Set PE bit (bit #0)
        db 0Fh,  22h, 0C0h            ; mov        cr0, eax

FLAT32_JUMP::

        db 66h,  67h, 0EAh            ; far jump
        dd 0h                         ; 32-bit offset
        dw 20h                        ; 16-bit selector

PMODE_ENTRY::                         ; protected mode entry point

        mov         ax,  8h
        mov         ds,  ax
        mov         es,  ax
        mov         fs,  ax
        mov         gs,  ax
        mov         ss,  ax           ; Flat mode setup.

        mov         esi, edx

        mov         edi, esi
        add         edi, LockLocation
        mov         al,  NotVacantFlag
TestLock::
        xchg        byte ptr [edi], al
        cmp         al, NotVacantFlag
        jz          TestLock

ProgramStack::

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
        mov         eax, dword ptr [edi]

        test        eax, eax
        jz          GoToSleep
        call        eax                           ; Call C function
        
GoToSleep::
        cli
        hlt
        jmp         $-2
        
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

END
