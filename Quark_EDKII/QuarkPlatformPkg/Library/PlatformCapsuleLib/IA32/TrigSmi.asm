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
;   TrigSmi.asm
;
; Abstract:
;
;   TrigSmi for capsule update. SMI number is 0x27 and 0x28.
;
; Notes:
;
;------------------------------------------------------------------------------
  .386P
  .MODEL FLAT,C
  .CODE
;------------------------------------------------------------------------------
;  VOID
;  SendCapsuleSmi (
;    UINTN Addr
;    )
;------------------------------------------------------------------------------
SendCapsuleSmi PROC PUBLIC
  push  ecx
  mov   ecx, dword ptr [esp + 8]
  push  ebp
  mov   ebp, esp
  sub   esp, 8
  push  eax
  push  ebx
  mov   ebx, ecx
  mov   eax, 0EF27h
  out   0B2h, AL   
  pop   ebx
  pop   eax
  mov   esp, ebp
  pop   ebp
  pop   ecx
  ret   0
SendCapsuleSmi ENDP

;------------------------------------------------------------------------------
;  VOID
;  GetUpdateStatusSmi (
;    UINTN Addr
;    )
;------------------------------------------------------------------------------
GetUpdateStatusSmi PROC PUBLIC
  push  ecx
  mov   ecx, dword ptr [esp + 8]
  push  ebp
  mov   ebp, esp
  sub   esp, 8
  push  eax
  push  ebx
  mov   ebx, ecx
  mov   eax, 0EF28h
  out   0B2h, AL   
  pop   ebx
  pop   eax
  mov   esp, ebp
  pop   ebp
  pop   ecx
  ret   0
GetUpdateStatusSmi ENDP

END
