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
;   ReadMsrUtils.asm
;
; Abstract:
;
;   Read Msr support functions
;
;------------------------------------------------------------------------------

    .586p
    .model  flat,C
    .code

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMsrReturnResult (
;   IN      UINT32                    Index,
;   IN      UINT32                    QuarkMsrDataLow,
;   IN      UINT32                    QuarkMsrDataHigh
;   );
;------------------------------------------------------------------------------
AsmReadMsrReturnResult   PROC
    mov     eax, [esp + 8]          ; QuarkMsrDataLow
    mov     ecx, [esp + 4]         ; Index
    mov     edx, [esp + 12]          ; QuarkMsrDataHigh
    ret
AsmReadMsrReturnResult   ENDP

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmReadMsrRealMsrAccess (
;   IN      UINT32                    Index
;   );
;------------------------------------------------------------------------------
AsmReadMsrRealMsrAccess   PROC
    mov     ecx, [esp + 4]        ; Index
    mov     eax, 0FEE00900h
    mov     edx, 00000000h
    cmp     ecx, 01Bh             ; EFI_MSR_IA32_APIC_BASE
    je      exitDone
    rdmsr
exitDone:    
    ret
AsmReadMsrRealMsrAccess   ENDP

;------------------------------------------------------------------------------
; UINT64
; EFIAPI
; AsmWriteMsrRealMsrAccess (
;   IN      UINT32                    Index,
;   IN      UINT32                    QuarkMsrDataLow,
;   IN      UINT32                    QuarkMsrDataHigh
;   );
;------------------------------------------------------------------------------
AsmWriteMsrRealMsrAccess   PROC
    mov     eax, [esp + 8]          ; QuarkMsrDataLow
    mov     ecx, [esp + 4]         ; Index
    mov     edx, [esp + 12]          ; QuarkMsrDataHigh
    wrmsr
    ret
AsmWriteMsrRealMsrAccess   ENDP

    END
