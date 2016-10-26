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
;   SmiException.asm
;
; Abstract:
;
;   Exception handlers used in SM mode
;
;------------------------------------------------------------------------------

    .686p
    .model  flat,C

CpuDeadLoop PROTO   C

EXTERNDEF   gSmiMtrrs:QWORD
EXTERNDEF   gcSmiIdtr:FWORD
EXTERNDEF   gcSmiGdtr:FWORD
EXTERNDEF   gcPsd:BYTE
EXTERNDEF   FeaturePcdGet (PcdCpuSmmStackGuard):BYTE
EXTERNDEF   gSavedPageFaultIdtEntry:DWORD
EXTERNDEF   gSavedDebugExceptionIdtEntry:DWORD
EXTERNDEF   FeaturePcdGet (PcdCpuSmmProfileEnable):BYTE


    .data

NullSeg     DQ      0                   ; reserved by architecture
            DQ      0                   ; reserved for future use
CodeSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
DataSeg32   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      93h
            DB      0cfh                ; LimitHigh
            DB      0                   ; BaseHigh
            DQ      0                   ; reserved for future use
CodeSeg16   LABEL   QWORD
            DW      -1
            DW      0
            DB      0
            DB      9bh
            DB      8fh
            DB      0
DataSeg16   LABEL   QWORD
            DW      -1
            DW      0
            DB      0
            DB      93h
            DB      8fh
            DB      0
CodeSeg64   LABEL   QWORD
            DW      -1                  ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      9bh
            DB      0afh                ; LimitHigh
            DB      0                   ; BaseHigh
GDT_SIZE = $ - offset NullSeg

TssSeg      LABEL   QWORD
            DW      TSS_DESC_SIZE       ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      89h
            DB      080h                ; LimitHigh
            DB      0                   ; BaseHigh
ExceptionTssSeg     LABEL   QWORD
            DW      TSS_DESC_SIZE       ; LimitLow
            DW      0                   ; BaseLow
            DB      0                   ; BaseMid
            DB      89h
            DB      080h                ; LimitHigh
            DB      0                   ; BaseHigh

CODE_SEL          = offset CodeSeg32 - offset NullSeg
DATA_SEL          = offset DataSeg32 - offset NullSeg
TSS_SEL           = offset TssSeg - offset NullSeg
EXCEPTION_TSS_SEL = offset ExceptionTssSeg - offset NullSeg

IA32_TSS STRUC
                    DW ?
                    DW ?
  ESP0              DD ?
  SS0               DW ?
                    DW ?
  ESP1              DD ?
  SS1               DW ?
                    DW ?
  ESP2              DD ?
  SS2               DW ?
                    DW ?
  _CR3              DD ?
  EIP               DD ?
  EFLAGS            DD ?
  _EAX              DD ?
  _ECX              DD ?
  _EDX              DD ?
  _EBX              DD ?
  _ESP              DD ?
  _EBP              DD ?
  _ESI              DD ?
  _EDI              DD ?
  _ES               DW ?
                    DW ?
  _CS               DW ?
                    DW ?
  _SS               DW ?
                    DW ?
  _DS               DW ?
                    DW ?
  _FS               DW ?
                    DW ?
  _GS               DW ?
                    DW ?
  LDT               DW ?
                    DW ?
                    DW ?
                    DW ?
IA32_TSS ENDS

; Create 2 TSS segments just after GDT
TssDescriptor LABEL BYTE
            DW      0                   ; PreviousTaskLink
            DW      0                   ; Reserved
            DD      0                   ; ESP0
            DW      0                   ; SS0
            DW      0                   ; Reserved
            DD      0                   ; ESP1
            DW      0                   ; SS1
            DW      0                   ; Reserved
            DD      0                   ; ESP2
            DW      0                   ; SS2
            DW      0                   ; Reserved
            DD      0                   ; CR3
            DD      0                   ; EIP
            DD      0                   ; EFLAGS
            DD      0                   ; EAX
            DD      0                   ; ECX
            DD      0                   ; EDX
            DD      0                   ; EBX
            DD      0                   ; ESP
            DD      0                   ; EBP
            DD      0                   ; ESI
            DD      0                   ; EDI
            DW      0                   ; ES
            DW      0                   ; Reserved
            DW      0                   ; CS
            DW      0                   ; Reserved
            DW      0                   ; SS
            DW      0                   ; Reserved
            DW      0                   ; DS
            DW      0                   ; Reserved
            DW      0                   ; FS
            DW      0                   ; Reserved
            DW      0                   ; GS
            DW      0                   ; Reserved
            DW      0                   ; LDT Selector
            DW      0                   ; Reserved
            DW      0                   ; T
            DW      0                   ; I/O Map Base
TSS_DESC_SIZE = $ - offset TssDescriptor

ExceptionTssDescriptor LABEL BYTE
            DW      0                   ; PreviousTaskLink
            DW      0                   ; Reserved
            DD      0                   ; ESP0
            DW      0                   ; SS0
            DW      0                   ; Reserved
            DD      0                   ; ESP1
            DW      0                   ; SS1
            DW      0                   ; Reserved
            DD      0                   ; ESP2
            DW      0                   ; SS2
            DW      0                   ; Reserved
            DD      0                   ; CR3
            DD      offset PFHandlerEntry ; EIP
            DD      00000002            ; EFLAGS
            DD      0                   ; EAX
            DD      0                   ; ECX
            DD      0                   ; EDX
            DD      0                   ; EBX
            DD      0                   ; ESP
            DD      0                   ; EBP
            DD      0                   ; ESI
            DD      0                   ; EDI
            DW      DATA_SEL            ; ES
            DW      0                   ; Reserved
            DW      CODE_SEL            ; CS
            DW      0                   ; Reserved
            DW      DATA_SEL            ; SS
            DW      0                   ; Reserved
            DW      DATA_SEL            ; DS
            DW      0                   ; Reserved
            DW      DATA_SEL            ; FS
            DW      0                   ; Reserved
            DW      DATA_SEL            ; GS
            DW      0                   ; Reserved
            DW      0                   ; LDT Selector
            DW      0                   ; Reserved
            DW      0                   ; T
            DW      0                   ; I/O Map Base

gcPsd     LABEL   BYTE
            DB      'PSDSIG  '
            DW      PSD_SIZE
            DW      2
            DW      1 SHL 2
            DW      CODE_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      DATA_SEL
            DW      0
            DQ      0
            DQ      0
            DQ      0
            DQ      offset NullSeg
            DD      GDT_SIZE
            DD      0
            DB      24 dup (0)
            DQ      offset gSmiMtrrs
PSD_SIZE  = $ - offset gcPsd

gcSmiGdtr   LABEL   FWORD
    DW      GDT_SIZE - 1
    DD      offset NullSeg

gcSmiIdtr   LABEL   FWORD
    DW      IDT_SIZE - 1
    DD      offset _SmiIDT

_SmiIDT     LABEL   QWORD
REPEAT      32
    DW      0                           ; Offset 0:15
    DW      CODE_SEL                    ; Segment selector
    DB      0                           ; Unused
    DB      8eh                         ; Interrupt Gate, Present
    DW      0                           ; Offset 16:31
            ENDM
IDT_SIZE = $ - offset _SmiIDT

TaskGateDescriptor LABEL DWORD
    DW      0                           ; Reserved
    DW      EXCEPTION_TSS_SEL           ; TSS Segment selector
    DB      0                           ; Reserved
    DB      85h                         ; Task Gate, present, DPL = 0
    DW      0                           ; Reserved

;
; point to the external interrupt vector table
;
ExternalVectorTablePtr DWORD 0

;
; Saved IDT Entry for Page Fault
;
gSavedPageFaultIdtEntry LABEL   DWORD
    DD      0
    DD      0

;
; Saved IDT Entry for INT 1
;
gSavedDebugExceptionIdtEntry LABEL   DWORD
    DD      0
    DD      0
    

    .code

InitializeSmmExternalVectorTablePtr PROC PUBLIC
    mov     eax, [esp+4]
    mov     ExternalVectorTablePtr, eax
    ret
InitializeSmmExternalVectorTablePtr ENDP

;------------------------------------------------------------------------------
; Exception handlers
;------------------------------------------------------------------------------
_SmiExceptionHandlers   PROC
IHDLRIDX = 0
REPEAT      8                           ; INT0 ~ INT7
    push    eax                         ; dummy error code
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      1                           ; INT8
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
    int     3
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      1                           ; INT9
    push    eax                         ; dummy error code
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      5                           ; INT10 ~ INT14
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
    int     3
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      2                           ; INT15 ~ INT16
    push    eax                         ; dummy error code
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      1                           ; INT17
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
    int     3
IHDLRIDX = IHDLRIDX + 1
            ENDM
REPEAT      14                          ; INT18 ~ INT31
    push    eax                         ; dummy error code
    push    IHDLRIDX
    DB      0e9h                        ; jmp disp32
    DD      _SmiExceptionEntryPoint - ($ + 4)
IHDLRIDX = IHDLRIDX + 1
            ENDM
_SmiExceptionHandlers   ENDP

;------------------------------------------------------------------------------
; _SmiExceptionEntryPoint is the entry point for all exceptions
;
;
; Stack:
; +---------------------+
; +    EFlags           +
; +---------------------+
; +    CS               +
; +---------------------+
; +    EIP              +
; +---------------------+
; +    Error Code       +
; +---------------------+
; +    Vector Number    +
; +---------------------+
; +    EBP              +
; +---------------------+ <-- EBP
;
;
;------------------------------------------------------------------------------
_SmiExceptionEntryPoint PROC


    push    ebp
    mov     ebp, esp


    ;
    ; Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
    ; is 16-byte aligned
    ;
    and     esp, 0fffffff0h
    sub     esp, 12

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    push    eax
    push    ecx
    push    edx
    push    ebx
    lea     ecx, [ebp + 6 * 4]
    push    ecx                          ; ESP
    push    dword ptr [ebp]              ; EBP
    push    esi
    push    edi

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
    mov     eax, ss
    push    eax
    movzx   eax, word ptr [ebp + 4 * 4]
    push    eax
    mov     eax, ds
    push    eax
    mov     eax, es
    push    eax
    mov     eax, fs
    push    eax
    mov     eax, gs
    push    eax

;; UINT32  Eip;
    mov     eax, [ebp + 3 * 4]
    push    eax

;; UINT32  Gdtr[2], Idtr[2];
    sub     esp, 8
    sidt    [esp]
    mov     eax, [esp + 2]
    xchg    eax, [esp]
    and     eax, 0FFFFh
    mov     [esp+4], eax

    sub     esp, 8
    sgdt    [esp]
    mov     eax, [esp + 2]
    xchg    eax, [esp]
    and     eax, 0FFFFh
    mov     [esp+4], eax

;; UINT32  Ldtr, Tr;
    xor     eax, eax
    str     ax
    push    eax
    sldt    ax
    push    eax

;; UINT32  EFlags;
    mov     eax, [ebp + 5 * 4]
    push    eax

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    mov     eax, cr4
    or      eax, 208h
    mov     cr4, eax
    push    eax
    mov     eax, cr3
    push    eax
    mov     eax, cr2
    push    eax
    xor     eax, eax
    push    eax
    mov     eax, cr0
    push    eax

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
    mov     eax, dr7
    push    eax
    mov     eax, dr6
    push    eax
    mov     eax, dr3
    push    eax
    mov     eax, dr2
    push    eax
    mov     eax, dr1
    push    eax
    mov     eax, dr0
    push    eax

;; FX_SAVE_STATE_IA32 FxSaveState;
    sub     esp, 512
    mov     edi, esp
    db      0fh, 0aeh, 07h ;fxsave [edi]

; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

;; UINT32  ExceptionData;
    push    dword ptr [ebp + 2 * 4]

;; call into exception handler
    mov     eax, ExternalVectorTablePtr  ; get the interrupt vectors base
    or      eax, eax                        ; NULL?
    jz      nullExternalExceptionHandler

    mov     ecx, [ebp + 4]
    mov     eax, [eax + ecx * 4]
    or      eax, eax                        ; NULL?
    jz      nullExternalExceptionHandler

;; Prepare parameter and call
    mov     edx, esp
    push    edx
    mov     edx, dword ptr [ebp + 1 * 4]
    push    edx

    ;
    ; Call External Exception Handler
    ;
    call    eax
    add     esp, 8
    jmp     @F

nullExternalExceptionHandler:
; CpuDeadLoop() is the default exception handler since it preserves the processor
; branch log.
  call    CpuDeadLoop

@@:
;; UINT32  ExceptionData;
    add     esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
    mov     esi, esp
    db      0fh, 0aeh, 0eh ; fxrstor [esi]
    add     esp, 512

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;; Skip restoration of DRx registers to support in-circuit emualators
;; or debuggers set breakpoint in interrupt/exception context
    add     esp, 4 * 6

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    pop     eax
    mov     cr0, eax
    add     esp, 4    ; not for Cr1
    pop     eax
    mov     cr2, eax
    pop     eax
    mov     cr3, eax
    pop     eax
    mov     cr4, eax

;; UINT32  EFlags;
    pop     dword ptr [ebp + 5 * 4]

;; UINT32  Ldtr, Tr;
;; UINT32  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
    add     esp, 24

;; UINT32  Eip;
    pop     dword ptr [ebp + 3 * 4]

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;
    pop     gs
    pop     fs
    pop     es
    pop     ds
    pop     dword ptr [ebp + 4 * 4]
    pop     ss

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    pop     edi
    pop     esi
    add     esp, 4   ; not for ebp
    add     esp, 4   ; not for esp
    pop     ebx
    pop     edx
    pop     ecx
    pop     eax

    mov     esp, ebp
    pop     ebp

; Set single step DB# if SMM profile is enabled and page fault exception happens
    cmp     FeaturePcdGet (PcdCpuSmmProfileEnable), 0
    jz      @Done
; Check if this is page fault exception
    cmp     dword ptr [esp], 0eh
    jnz     @F
; Enable TF bit after page fault handler runs
    bts     dword ptr [esp + 16], 8  ; EFLAGS
    jmp     @Done
@@:
; Check if this is INT 1 exception
    cmp     dword ptr [esp], 1
    jnz     @Done
; Clear TF bit after INT1 handler runs
    btc     dword ptr [esp + 16], 8  ; EFLAGS
@Done:

    add     esp, 8                      ; skip INT# & ErrCode
Return:
    iretd
;
; Page Fault Exception Handler entry when SMM Stack Guard is enabled
; Executiot starts here after a task switch
;
PFHandlerEntry::
;
; Get this processor's TSS
;
    sub     esp, 8
    sgdt    [esp + 2]
    mov     eax, [esp + 4]              ; GDT base
    add     esp, 8
    mov     ecx, [eax + TSS_SEL + 2]
    shl     ecx, 8
    mov     cl, [eax + TSS_SEL + 7]
    ror     ecx, 8                      ; ecx = TSS base

    mov     ebp, esp

    ;
    ; Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
    ; is 16-byte aligned
    ;
    and     esp, 0fffffff0h
    sub     esp, 12

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    push    (IA32_TSS ptr [ecx])._EAX
    push    (IA32_TSS ptr [ecx])._ECX
    push    (IA32_TSS ptr [ecx])._EDX
    push    (IA32_TSS ptr [ecx])._EBX
    push    (IA32_TSS ptr [ecx])._ESP
    push    (IA32_TSS ptr [ecx])._EBP
    push    (IA32_TSS ptr [ecx])._ESI
    push    (IA32_TSS ptr [ecx])._EDI

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
    movzx   eax, (IA32_TSS ptr [ecx])._SS
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._CS
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._DS
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._ES
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._FS
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx])._GS
    push    eax

;; UINT32  Eip;
    push    (IA32_TSS ptr [ecx]).EIP

;; UINT32  Gdtr[2], Idtr[2];
    sub     esp, 8
    sidt    [esp]
    mov     eax, [esp + 2]
    xchg    eax, [esp]
    and     eax, 0FFFFh
    mov     [esp+4], eax

    sub     esp, 8
    sgdt    [esp]
    mov     eax, [esp + 2]
    xchg    eax, [esp]
    and     eax, 0FFFFh
    mov     [esp+4], eax

;; UINT32  Ldtr, Tr;
    mov     eax, TSS_SEL
    push    eax
    movzx   eax, (IA32_TSS ptr [ecx]).LDT
    push    eax

;; UINT32  EFlags;
    push    (IA32_TSS ptr [ecx]).EFLAGS

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    mov     eax, cr4
    or      eax, 208h
    mov     cr4, eax
    push    eax
    mov     eax, cr3
    push    eax
    mov     eax, cr2
    push    eax
    xor     eax, eax
    push    eax
    mov     eax, cr0
    push    eax

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
    mov     eax, dr7
    push    eax
    mov     eax, dr6
    push    eax
    mov     eax, dr3
    push    eax
    mov     eax, dr2
    push    eax
    mov     eax, dr1
    push    eax
    mov     eax, dr0
    push    eax

;; FX_SAVE_STATE_IA32 FxSaveState;
    sub     esp, 512
    mov     edi, esp
    db      0fh, 0aeh, 07h ;fxsave [edi]

; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
    cld

;; UINT32  ExceptionData;
    push    dword ptr [ebp]

;; call into exception handler
    mov     eax, ExternalVectorTablePtr     ; get the interrupt vectors base
    or      eax, eax                        ; NULL?
    jz      nullExternalExceptionHandler

    mov     ebx, ecx
    mov     ecx, 14
    mov     eax, [eax + ecx * 4]
    or      eax, eax                        ; NULL?
    jz      nullExternalExceptionHandler

;; Prepare parameter and call
    mov     edx, esp
    push    edx
    mov     edx, 14
    push    edx

    ;
    ; Call External Exception Handler
    ;
    call    eax
    add     esp, 8

    mov     ecx, ebx
;; UINT32  ExceptionData;
    add     esp, 4

;; FX_SAVE_STATE_IA32 FxSaveState;
    mov     esi, esp
    db      0fh, 0aeh, 0eh ; fxrstor [esi]
    add     esp, 512

;; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
;; Skip restoration of DRx registers to support in-circuit emualators
;; or debuggers set breakpoint in interrupt/exception context
    add     esp, 4 * 6

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
    pop     eax
    mov     cr0, eax
    add     esp, 4    ; not for Cr1
    pop     eax
    mov     cr2, eax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._CR3, eax
    pop     eax
    mov     cr4, eax

;; UINT32  EFlags;
    pop     (IA32_TSS ptr [ecx]).EFLAGS

;; UINT32  Ldtr, Tr;
;; UINT32  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
    add     esp, 24

;; UINT32  Eip;
    pop     (IA32_TSS ptr [ecx]).EIP

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;
    pop     eax
    mov     (IA32_TSS ptr [ecx])._GS, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._FS, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._ES, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._DS, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._CS, ax
    pop     eax
    mov     (IA32_TSS ptr [ecx])._SS, ax

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    pop     (IA32_TSS ptr [ecx])._EDI
    pop     (IA32_TSS ptr [ecx])._ESI
    add     esp, 4   ; not for ebp
    add     esp, 4   ; not for esp
    pop     (IA32_TSS ptr [ecx])._EBX
    pop     (IA32_TSS ptr [ecx])._EDX
    pop     (IA32_TSS ptr [ecx])._ECX
    pop     (IA32_TSS ptr [ecx])._EAX

    mov     esp, ebp

; Set single step DB# if SMM profile is enabled and page fault exception happens
    cmp     FeaturePcdGet (PcdCpuSmmProfileEnable), 0
    jz      @Done2
; Enable TF bit after page fault handler runs
    bts     (IA32_TSS ptr [ecx]).EFLAGS, 8  ; EFLAGS

@Done2:

    add     esp, 4                      ; skip ErrCode

    jmp     Return
_SmiExceptionEntryPoint ENDP

InitializeIDT   PROC    USES    ebx
    lea     edx, _SmiIDT - 8
;    push    IDT_SIZE / 8
    DB      68h                         ; push /id
    DD      IDT_SIZE / 8
    lea     ebx, _SmiExceptionHandlers - 8
    pop     ecx
@@:
    lea     eax, [ebx + ecx*8]
    mov     [edx + ecx*8], ax
    shr     eax, 16
    mov     [edx + ecx*8 + 6], ax
    loop    @B

    cmp     FeaturePcdGet (PcdCpuSmmStackGuard), 0
    jz      @F

;
; If SMM Stack Guard feature is enabled, the Page Fault Exception entry in IDT
; is a Task Gate Descriptor so that when a Page Fault Exception occurrs,
; the processors can use a known good stack in case stack is ran out.
;
    lea     ebx, _SmiIDT + 14 * 8
    lea     edx, TaskGateDescriptor
    mov     eax, [edx]
    mov     [ebx], eax
    mov     eax, [edx + 4]
    mov     [ebx + 4], eax

@@:
;
; Save Page Fault IDT entry in gPageFaultIdtEntry
;
    lea     ebx, _SmiIDT + 14 * 8
    lea     edx, gSavedPageFaultIdtEntry
    mov     eax, [ebx]
    mov     [edx], eax
    mov     eax, [ebx + 4]
    mov     [edx + 4], eax

    cmp     FeaturePcdGet (PcdCpuSmmProfileEnable), 0
    jz      @F

;
; Save INT 1 IDT entry in gSavedDebugExceptionIdtEntry
;
    lea     ebx, _SmiIDT + 1 * 8
    lea     edx, gSavedDebugExceptionIdtEntry
    mov     eax, [ebx]
    mov     [edx], eax
    mov     eax, [ebx + 4]
    mov     [edx + 4], eax

@@:
    ret
InitializeIDT   ENDP

    END
