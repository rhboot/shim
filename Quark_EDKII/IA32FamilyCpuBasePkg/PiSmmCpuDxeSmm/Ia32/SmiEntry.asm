;------------------------------------------------------------------------------
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
; Module Name:
;
;   SmiEntry.asm
;
; Abstract:
;
;   Code template of the SMI handler for a particular processor
;
;------------------------------------------------------------------------------

    .686p
    .model  flat,C
    .xmm

DSC_OFFSET    EQU     0fb00h
DSC_GDTPTR    EQU     30h
DSC_GDTSIZ    EQU     38h
DSC_CS        EQU     14
DSC_DS        EQU     16
DSC_SS        EQU     18
DSC_OTHERSEG  EQU     20

TSS_SEGMENT   EQU     40h

SmiRendezvous   PROTO   C

EXTERNDEF   gcSmiHandlerTemplate:BYTE
EXTERNDEF   gcSmiHandlerSize:WORD
EXTERNDEF   gSmiCr3:DWORD
EXTERNDEF   gcSmiHandlerOffset:WORD
EXTERNDEF   gSmiStack:DWORD
EXTERNDEF   gSmbase:DWORD
EXTERNDEF   FeaturePcdGet (PcdCpuSmmDebug):BYTE
EXTERNDEF   FeaturePcdGet (PcdCpuSmmStackGuard):BYTE

    .const

gcSmiHandlerOffset  DW      _SmiHandler - _SmiEntryPoint + 8000h

    .code

gcSmiHandlerTemplate    LABEL   BYTE

_SmiEntryPoint  PROC
    DB      0bbh                        ; mov bx, imm16
    DW      offset _GdtDesc - _SmiEntryPoint + 8000h
    DB      2eh, 0a1h                   ; mov ax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTSIZ
    dec     eax
    mov     cs:[edi], eax               ; mov cs:[bx], ax
    DB      66h, 2eh, 0a1h              ; mov eax, cs:[offset16]
    DW      DSC_OFFSET + DSC_GDTPTR
    mov     cs:[edi + 2], ax            ; mov cs:[bx + 2], eax
    mov     bp, ax                      ; ebp = GDT base
    DB      66h
    lgdt    fword ptr cs:[edi]          ; lgdt fword ptr cs:[bx]
    DB      66h, 0b8h                   ; mov eax, imm32
gSmiCr3     DD      ?
    mov     cr3, eax
    DB      66h
    mov     eax, 020h                   ; as cr4.PGE is not set here, refresh cr3
    mov     cr4, eax                    ; in PreModifyMtrrs() to flush TLB.
    DB      2eh, 0a1h                   ; mov ax, cs:[offset16]
    DW      DSC_OFFSET + DSC_CS
    mov     cs:[edi - 2], eax           ; mov cs:[bx - 2], ax
    DB      66h, 0bfh                   ; mov edi, SMBASE
gSmbase    DD    ?
    DB      67h
    lea     ax, [edi + (@32bit - _SmiEntryPoint) + 8000h]
    mov     cs:[edi - 6], ax            ; mov cs:[bx - 6], eax
    mov     ebx, cr0
    DB      66h
    and     ebx, 9ffafff3h
    DB      66h
    or      ebx, 80000023h
    mov     cr0, ebx
    DB      66h, 0eah
    DD      ?
    DW      ?
_GdtDesc    FWORD   ?
@32bit:
    lea     ebx, [edi + DSC_OFFSET]
    mov     ax, [ebx + DSC_DS]
    mov     ds, eax
    mov     ax, [ebx + DSC_OTHERSEG]
    mov     es, eax
    mov     fs, eax
    mov     gs, eax
    mov     ax, [ebx + DSC_SS]
    mov     ss, eax

    cmp     FeaturePcdGet (PcdCpuSmmStackGuard), 0
    jz      @F

; Load TSS
    mov     byte ptr [ebp + TSS_SEGMENT + 5], 89h ; clear busy flag

    mov     eax, TSS_SEGMENT
    ltr     ax
@@:
;   jmp     _SmiHandler                 ; instruction is not needed
_SmiEntryPoint  ENDP

_SmiHandler PROC
    DB      0bch                        ; mov esp, imm32
gSmiStack   DD      ?
    cmp     FeaturePcdGet (PcdCpuSmmDebug), 0
    jz      @3
    call    @1
@1:
    pop     ebp
    mov     eax, 80000001h
    cpuid
    bt      edx, 29                     ; check cpuid to identify X64 or IA32 
    lea     edi, [ebp - (@1 - _SmiEntryPoint) + 7fc8h]
    lea     esi, [edi + 4]
    jnc     @2
    add     esi, 4
@2:
    mov     ecx, [esi]
    mov     edx, [edi]
@5:
    mov     dr6, ecx
    mov     dr7, edx                    ; restore DR6 & DR7 before running C code
@3:
    mov     ecx, [esp]                  ; CPU Index
    
    push    ecx
    mov     eax, SmiRendezvous
    call    eax
    pop     ecx

    cmp     FeaturePcdGet (PcdCpuSmmDebug), 0
    jz	    @4

    mov     ecx, dr6
    mov     edx, dr7
    mov     [esi], ecx
    mov     [edi], edx
@4:
    rsm
_SmiHandler ENDP

gcSmiHandlerSize    DW      $ - _SmiEntryPoint

    END
