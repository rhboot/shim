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
;   SecCpu.asm
;
; Abstract:
;
;   Sec CPU functions
;
;------------------------------------------------------------------------------

    .686p
    .xmm
    .model  flat,c

INCLUDE     Ia32.inc

;
; Declare PcdGet32 (PcdCpuLocalApicBaseAddress) as a double word
;
EXTERNDEF   C   PcdGet32 (PcdCpuLocalApicBaseAddress):DWORD

    .code

;
; Local APIC timer is enabled in this function.
;
SecCpuInitTimer PROC
    mov     eax, PcdGet32 (PcdCpuLocalApicBaseAddress)  ; eax <- local APIC base
    mov     dword ptr [eax + 3e0h], 0ah ; divided by 128 (divide config register)
    bts     dword ptr [eax + 320h], 17  ; set timer mode to "periodic"
    mov     dword ptr [eax + 380h], -1  ; start timer by writing to init count
    ret
SecCpuInitTimer ENDP

    END
