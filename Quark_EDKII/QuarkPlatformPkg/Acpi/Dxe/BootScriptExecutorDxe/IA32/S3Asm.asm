;; @file
;   This is the assembly code for transferring to control to OS S3 waking vector
;   for IA32 platform
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

    .586P
    .model  flat,C
    .code

;-----------------------------------------
;VOID
;AsmTransferControl (
;  IN   UINT32           S3WakingVector,
;  IN   UINT32           AcpiLowMemoryBase
;  );
;-----------------------------------------
   
AsmTransferControl  PROC
    ; S3WakingVector    :DWORD
    ; AcpiLowMemoryBase :DWORD
    push  ebp
    mov   ebp, esp    
    lea   eax, @F
    push  28h               ; CS
    push  eax
    mov   ecx, [ebp + 8]
    shrd  ebx, ecx, 20
    and   ecx, 0fh          
    mov   bx, cx          
    mov   @jmp_addr, ebx
    retf
@@:
    DB    0b8h, 30h, 0      ; mov ax, 30h as selector
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax
    mov   ss, ax
    mov   eax, cr0          ; Get control register 0  
    DB    66h
    DB    83h, 0e0h, 0feh   ; and    eax, 0fffffffeh  ; Clear PE bit (bit #0)
    DB    0fh, 22h, 0c0h    ; mov    cr0, eax         ; Activate real mode
    DB    0eah              ; jmp far @jmp_addr
@jmp_addr DD  ?

AsmTransferControl  ENDP

    END