  page    ,132
  title   CPU ARCHITECTURAL DXE PROTOCOL ASSEMBLY HOOKS
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
;   CpuAsm.asm
; 
; Abstract:
; 
;   Assembly code that supports IA32 CPU architectural protocol
;
;------------------------------------------------------------------------------

.686p
.model  flat        

.stack
.code
.MMX
.XMM

ApMachineCheckHandler    PROC C    PUBLIC
  ;
  ; Clear MCIP flag of IA32_MCG_STATUS register
  ;
  mov     ecx, 17ah
  rdmsr
  btr     eax, 2
  wrmsr

  iretd
  
ApMachineCheckHandler ENDP

EXTERNDEF C ApMachineCheckHandlerEnd:BYTE

ApMachineCheckHandlerEnd LABEL BYTE

  end

