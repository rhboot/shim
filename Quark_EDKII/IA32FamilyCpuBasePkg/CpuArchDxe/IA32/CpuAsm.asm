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

.data
ExternalVectorTablePtr DD ?             ; Table of call backs
CommonInterruptEntry   DD CommonEntry   ; Address of CommonEntry
Idtr                   DW ?             ; FWORD for IDT register
Idtr1                  DD ?             ; MUST BE IMMEDIATELY AFTER Idtr

EXTRN _mErrorCodeFlag:DWORD             ; Error code flags for exceptions

.stack
.code
.MMX
.XMM

UINT8    TYPEDEF    BYTE
UINT16   TYPEDEF    WORD
UINT32   TYPEDEF    DWORD
UINT64   TYPEDEF    QWORD
UINTN    TYPEDEF    UINT32

;---------------------------------------;
; _InitializeIdt                ;
;----------------------------------------------------------------------------;
; 
; Protocol prototype
;   InitializeIdt (
;   IN EFI_CPU_INTERRUPT_HANDLER TableStart,
;   IN UINTN                     *IdtTablePtr,
;   IN UINT16                    IdtLimit
;   )
;           
; Routine Description:
; 
;  Creates an IDT table starting at IdtTablPtr. It has IdtLimit/8 entries.
;  Table is initialized to intxx where xx is from 00 to number of entries or
;  100h, whichever is smaller. After table has been initialized the LIDT
;  instruction is invoked.
; 
;  TableStart is the pointer to the callback table and is not used by 
;  InitializedIdt but by commonEntry. CommonEntry handles all interrupts,
;  does the context save and calls the callback entry, if non-NULL.
;  It is the responsibility of the callback routine to do hardware EOIs.
; 
; Arguments:
; 
;   TableStart          - Pointer to interrupt callback table
;
;   IdtTablePtr         - Pointer to IDT table
;
;   IdtLimit            - IDT Table limit = number of interrupt entries * 8
; 
; Returns: 
; 
;   Nothing
;
; 
; Input:  [ebp][0]  = Original ebp
;         [ebp][4]  = Return address
;         [ebp][8]  = TableStart
;         [ebp][0c] = *IdtTablePtr
;         [ebp][10] = IdtLimit
;          
; Output: Nothing
;           
; Destroys: Nothing
;-----------------------------------------------------------------------------;

_InitializeIdt  proc near public
  push    ebp               ; C prolog
  mov     ebp, esp
  push    edi

  mov     eax, [ebp+8]                  ; Get ExternalVectorTable Address
  mov     ExternalVectorTablePtr, eax

  mov     ax, [ebp+10h]                 ; Get IDT Table limit
  dec     ax
  mov     Idtr, ax

  mov     eax, [ebp+0ch]                ; Get Start of IDT
  mov     Idtr1, eax
  
  mov     edi, OFFSET Idtr              ; Load IDT register
  lidt    FWORD PTR es:[edi]

  pop     edi
  pop     ebp
  ret
_InitializeIdt  endp

;----------------------------------------------------------------------------;
; 
; Protocol prototype
;   None
;           
; Routine Description:
; 
;  These routines handle the individual interrupts. These routines always
;  gain control on any interrupt or exception. They save EAX and place
;  the interrupt number in EAX. CommonEntry is then jumped to. 
;  instruction is invoked.
; 
;  CommonEntry handles all interrupts,does the context save and calls the 
;  callback entry, if non-NULL. It is the responsibility of the callback 
;  routine to do hardware EOIs. Callbacks are entered into the table
;  located at TableStart. Entries are modified by the InstallInterruptHandler
;  and UninstallInterruptHandler protocols.
; 
; Arguments to CommonEntry:
; 
;   EAX                 - Interrupt or exception number
;
;   TableStart          - Pointer to interrupt callback table
; 
; Returns: 
; 
;   Nothing
;
; 
; Output: Nothing
;           
; Destroys: Nothing
;-----------------------------------------------------------------------------;

TemplateStart:
   push eax
   
   ;mov  eax, 0nnh (nn stands for vector number, which will be fixed at runtime 
   DB   0b8h
VectorNumber:   
   DD   00h

   jmp  dword ptr [CommonInterruptEntry];
TemplateEnd:

CommonEntry:

;---------------------------------------;
; _CommonEntry                  ;
;----------------------------------------------------------------------------;
; The follow algorithm is used for the common interrupt routine.
; Entry from each interrupt with a push eax and eax=interrupt number

;
; +---------------------+
; +    EFlags           +
; +---------------------+
; +    CS               +
; +---------------------+
; +    EIP              +
; +---------------------+
; +    Error Code       +
; +---------------------+
; + EAX / Vector Number +
; +---------------------+
; +    EBP              +
; +---------------------+ <-- EBP
;

  cli
  ;
  ; All interrupt handlers are invoked through interrupt gates, so
  ; IF flag automatically cleared at the entry point
  ;
  cmp     eax, 32         ; Intel reserved vector for exceptions?
  jae     NoErrorCode
  bt      cs:_mErrorCodeFlag, eax
  jc      @F

NoErrorCode:
  ;
  ; Push a dummy error code on the stack
  ; to maintain coherent stack map
  ;
  push    [esp]
  mov     dword ptr [esp + 4], 0
@@:       
  push    ebp
  mov     ebp, esp

  ;
  ; Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
  ; is 16-byte aligned
  ;
  and     esp, 0fffffff0h
  sub     esp, 12

;; UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
  push    dword ptr [ebp + 4]          ; EAX
  push    ecx
  push    edx
  push    ebx
  lea     ecx, [ebp + 24]
  push    ecx                          ; ESP
  push    dword ptr [ebp]              ; EBP
  push    esi
  push    edi

  mov     [ebp + 4], eax               ; save vector number

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
  mov  eax, ss
  push eax
  movzx eax, word ptr [ebp + 16]
  push eax
  mov  eax, ds
  push eax
  mov  eax, es
  push eax
  mov  eax, fs
  push eax
  mov  eax, gs
  push eax

;; UINT32  Eip;
  push    dword ptr [ebp + 12]

;; UINT32  Gdtr[2], Idtr[2];
  sub  esp, 8
  sidt fword ptr [esp]
  sub  esp, 8
  sgdt fword ptr [esp]

;; UINT32  Ldtr, Tr;
  xor  eax, eax
  str  ax
  push eax
  sldt ax
  push eax

;; UINT32  EFlags;
  push    dword ptr [ebp + 20]

;; UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
  mov  eax, cr4
  push eax
  mov  eax, cr3
  push eax
  mov  eax, cr2
  push eax
  xor  eax, eax
  push eax
  mov  eax, cr0
  push eax

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

;; UEFI calling convention for IA32 requires that Direction flag in EFLAGs is clear
  cld

;; UINT32  ExceptionData;
  push    dword ptr [ebp + 8]

;; call into exception handler
  mov     ebx, [ebp + 4]
  mov     eax, ExternalVectorTablePtr
  mov     eax, [eax + ebx * 4]
  or      eax, eax                ; NULL?
  je  nonNullValue;

;; Prepare parameter and call
  mov     edx, esp
  push    edx
  push    ebx
  call    eax
  add     esp, 8

nonNullValue:
  cli
;; UINT32  ExceptionData;
  add esp, 4

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
  pop     dword ptr [ebp + 20]

;; UINT32  Ldtr, Tr;
;; UINT32  Gdtr[2], Idtr[2];
;; Best not let anyone mess with these particular registers...
  add     esp, 24

;; UINT32  Eip;
  pop     dword ptr [ebp + 12]

;; UINT32  Gs, Fs, Es, Ds, Cs, Ss;
;; NOTE - modified segment registers could hang the debugger...  We
;;        could attempt to insulate ourselves against this possibility,
;;        but that poses risks as well.
;;
  pop     gs
  pop     fs
  pop     es
  pop     ds
  pop     dword ptr [ebp + 16]
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
  add     esp, 8
  iretd


;---------------------------------------;
; _GetTemplateAddressMap                  ;
;----------------------------------------------------------------------------;
; 
; Protocol prototype
;   GetTemplateAddressMap (
;     INTERRUPT_HANDLER_TEMPLATE_MAP *AddressMap
;   );
;           
; Routine Description:
; 
;  Return address map of interrupt handler template so that C code can generate
;  interrupt handlers, and dynamically do address fix.
; 
; Arguments:
; 
; 
; Returns: 
; 
;   Nothing
;
; 
; Input:  [ebp][0]  = Original ebp
;         [ebp][4]  = Return address
;          
; Output: Nothing
;           
; Destroys: Nothing
;-----------------------------------------------------------------------------;
_GetTemplateAddressMap  proc near public
  push    ebp                 ; C prolog
  mov     ebp, esp
  pushad

  mov ebx, dword ptr [ebp+08h]
  mov dword ptr [ebx],    TemplateStart
  mov dword ptr [ebx+4h], TemplateEnd - TemplateStart
  
  ; if code in Template is updated, the value fills into the 3rd parameter 
  ; also needs update
  mov dword ptr [ebx+8h], VectorNumber - TemplateStart

  popad
  pop     ebp
  ret
_GetTemplateAddressMap  endp



;---------------------------------------;
; _InitializeSelectors                  ;
;----------------------------------------------------------------------------;
; 
; Protocol prototype
;   InitializeSelectors (
;   )
;           
; Routine Description:
; 
;  Creates an new GDT in RAM.  The problem is that our former selectors
;  were ROM based and the EFI OS Loader does not manipulate the machine state 
;  to change them (as it would for a 16-bit PC/AT startup code that had to
;  go from Real Mode to flat mode).
; 
; Arguments:
; 
; 
; Returns: 
; 
;   Nothing
;
; 
; Input:  [ebp][0]  = Original ebp
;         [ebp][4]  = Return address
;          
; Output: Nothing
;           
; Destroys: Nothing
;-----------------------------------------------------------------------------;

CODE_SELECTOR EQU 10h
DATA_SELECTOR EQU 18h

_InitializeGdt  proc near public
  push    ebp                 ; C prolog
  mov     ebp, esp
  pushad
  mov     edi, OFFSET Gdtr    ; Load GDT register

  mov     ax,cs               ; Get the selector data from our code image          
  mov     es,ax
  lgdt    FWORD PTR es:[edi]  ; and update the GDTR

  db      067h
  db      0eah              ; Far Jump Offset:Selector to reload CS
  dd      OFFSET SelectorRld;   Offset is ensuing instruction boundary
  dw      CODE_SELECTOR     ;   Selector is our code selector, 10h
SelectorRld::
  mov     ax, DATA_SELECTOR ; Update the Base for the new selectors, too
  mov     ds, ax
  mov     es, ax
  mov     fs, ax
  mov     gs, ax
  mov     ss, ax  

  popad
  pop     ebp
  ret
_InitializeGdt  endp
  
;-----------------------------------------------------------------------------;
; data
;-----------------------------------------------------------------------------;

;.data
        align 16

gdtr    dw GDT_END - GDT_BASE - 1   ; GDT limit
        dd OFFSET GDT_BASE          ; (GDT base gets set above)

;-----------------------------------------------------------------------------;
;   global descriptor table (GDT)
;-----------------------------------------------------------------------------;

        align 16

public GDT_BASE
GDT_BASE:
; null descriptor
NULL_SEL            equ $-GDT_BASE    ; Selector [0]
        dw 0            ; limit 15:0
        dw 0            ; base 15:0
        db 0            ; base 23:16
        db 0            ; type
        db 0            ; limit 19:16, flags
        db 0            ; base 31:24

; linear data segment descriptor
LINEAR_SEL      equ $-GDT_BASE        ; Selector [0x8]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; linear code segment descriptor
LINEAR_CODE_SEL equ $-GDT_BASE        ; Selector [0x10]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; system data segment descriptor
SYS_DATA_SEL    equ $-GDT_BASE        ; Selector [0x18]
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 092h         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; system code segment descriptor
SYS_CODE_SEL    equ $-GDT_BASE
        dw 0FFFFh       ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 09Ah         ; present, ring 0, data, expand-up, writable
        db 0CFh                 ; page-granular, 32-bit
        db 0

; spare segment descriptor
SPARE3_SEL  equ $-GDT_BASE
        dw 0            ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 0            ; present, ring 0, data, expand-up, writable
        db 0            ; page-granular, 32-bit
        db 0

; spare segment descriptor
SPARE4_SEL  equ $-GDT_BASE
        dw 0            ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 0            ; present, ring 0, data, expand-up, writable
        db 0            ; page-granular, 32-bit
        db 0

; spare segment descriptor
SPARE5_SEL  equ $-GDT_BASE
        dw 0            ; limit 0xFFFFF
        dw 0            ; base 0
        db 0
        db 0            ; present, ring 0, data, expand-up, writable
        db 0            ; page-granular, 32-bit
        db 0

GDT_END:


  end

