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
;   SmmInit.Asm
;
; Abstract:
;
;   Functions for relocating SMBASE's for all processors
;
; Notes:
;
;------------------------------------------------------------------------------

    .686p
    .xmm
    .model  flat,C

SmmInitHandler  PROTO   C

EXTERNDEF   C   gSmmCr0:DWORD
EXTERNDEF   C   gSmmCr3:DWORD
EXTERNDEF   C   gSmmCr4:DWORD
EXTERNDEF   C   gcSmmInitTemplate:BYTE
EXTERNDEF   C   gcSmmInitSize:WORD
EXTERNDEF   C   gSmmJmpAddr:QWORD
EXTERNDEF   C   mRebasedFlag:PTR BYTE
EXTERNDEF   C   mSmmRelocationOriginalAddress:DWORD
EXTERNDEF   C   gSmmInitStack:DWORD

    .data

NullSeg     DQ      0                   ; reserved by architecture
DataSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      93h
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
CodeSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
GDT_SIZE = $ - offset NullSeg

    .code

GdtDesc     LABEL   FWORD
            DW      GDT_SIZE
            DD      offset NullSeg

SmmStartup  PROC
    DB      66h, 0b8h
gSmmCr3     DD      ?
    mov     cr3, eax
    DB      67h, 66h
    lgdt    fword ptr cs:[ebp + (offset GdtDesc - SmmStartup)]
    DB      66h, 0b8h
gSmmCr4     DD      ?
    mov     cr4, eax
    DB      66h, 0b8h
gSmmCr0     DD      ?
    DB      0bfh, 8, 0                  ; mov di, 8
    mov     cr0, eax
    DB      66h, 0eah                   ; jmp far [ptr48]
gSmmJmpAddr LABEL   QWORD
    DD      @32bit
    DW      10h
@32bit:
    mov     ds, edi
    mov     es, edi
    mov     fs, edi
    mov     gs, edi
    mov     ss, edi
    DB      0bch                        ; mov esp, imm32
gSmmInitStack  DD ?
    call    SmmInitHandler
    rsm
SmmStartup  ENDP

gcSmmInitTemplate   LABEL   BYTE

_SmmInitTemplate    PROC
    DB      66h
    mov     ebp, SmmStartup
    DB      66h, 81h, 0edh, 00h, 00h, 03h, 00  ; sub ebp, 30000h
    jmp     bp                          ; jmp ebp actually
_SmmInitTemplate    ENDP

gcSmmInitSize   DW  $ - gcSmmInitTemplate

SmmRelocationSemaphoreComplete PROC
    push    eax
    mov     eax, mRebasedFlag
    mov     byte ptr [eax], 1
    pop     eax
    jmp     [mSmmRelocationOriginalAddress]
SmmRelocationSemaphoreComplete ENDP

    END
