;++
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
;   SwBpeLibIa32.asm
; 
; Abstract:
; 
;   This file provides low level I/O access functions to trigger
;   breakpoint events. 
;
;--
  .686P
  .MODEL FLAT,C
  .CODE

;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------
;  Procedure:   DoSwbpeTypeIoOut
; 
;  Input:       PortAddress   -  Port Address for doing IO Write
;               PortData      -  Data value for doing IO Write
;               PortDataWidth -  Data Width (Byte=1/Word=2/Dword=4) for doing IO Write
;               Mailbox       -  Data structure contains Data Store informations
; 
;  Output:      Debuggers can update structure pointed by EDI via this IO Write Break.
; 
;  Registers:   All are preserved
; 
;  Description: Function to do an IO Write and set EDI EDI pointer to Mailbox structure,
;               so Debuggers can recognize the GUID and update status in the structure.
; 
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------

DoSwbpeTypeIoOut    PROC C  USES ECX EDX EDI PortAddress:DWORD, PortData:DWORD, PortDataWidth:DWORD, Mailbox:DWORD
    ;
    mov     edi, Mailbox
    mov     edx, PortAddress
    mov     eax, PortData
    mov     ecx, PortDataWidth
    ;
    cmp     ecx, 1
    jne     TryIoOutWord
    out     dx, al
    jmp     IoOutReturn
    ;
TryIoOutWord:
    cmp     ecx, 2
    jne     TryIoOutDword
    out     dx, ax
    jmp     IoOutReturn
    ;
TryIoOutDword:
    cmp     ecx, 4
    jne     IoOutReturn
    out     dx, eax
    ;
IoOutReturn:
    ret 
    ;
DoSwbpeTypeIoOut    ENDP
        
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------
;  Procedure:   DoSwbpeTypeIoIn
; 
;  Input:       PortAddress   -  Port Address for doing IO Read
;               PortDataWidth -  Port data width for doing IO Read
;               Mailbox       -  Data structure contains Data Store informations
; 
;  Output:      Debuggers can update structure pointed by EDI via this IO Read Break,
;               EAX contains IO Read byte/word/dword value depends on PortDataWidth.
; 
;  Registers:   All are preserved
; 
;  Description: Function to do an IO Read and set EDI pointer to Mailbox structure,
;               so Debuggers can recognize the GUID and update status in the structure.
; 
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------

DoSwbpeTypeIoIn    PROC C   USES ECX EDX EDI PortAddress:DWORD, PortDataWidth:DWORD, Mailbox:DWORD
    ;
    xor     eax, eax
    mov     edi, Mailbox
    mov     edx, PortAddress
    mov     ecx, PortDataWidth
    ;
    cmp     ecx, 1
    jne     TryIoInWord
    in      al, dx
    jmp     IoInReturn
    ;
TryIoInWord:
    cmp     ecx, 2
    jne     TryIoInDword
    in      ax, dx
    jmp     IoInReturn
    ;
TryIoInDword:
    cmp     ecx, 4
    jne     IoInReturn
    in      eax, dx
    ;
IoInReturn:
    ret 
    ;
DoSwbpeTypeIoIn    ENDP

        
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------
;  Procedure:   DoSwbpeTypeMemWrite
; 
;  Input:       MemAddress   -  Memory Address for doing MEM Write
;               MemData      -  Data for doing Memory Write
;               MemDataWidth -  Write Data Width (Byte=1/Word=2/DWord=4)
;               Mailbox      -  Data structure contains Data Store info
; 
;  Output:      Debuggers can update structure pointed by EDI via this MEM Write Break
; 
;  Registers:   All are preserved
; 
;  Description: Function to do Memory Write and set EDI pointer to Mailbox structure,
;               so Debuggers can recognize the GUID and update status in the structure.
; 
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------

DoSwbpeTypeMemWrite    PROC C  USES ECX EDX EDI MemAddress:DWORD, MemData:DWORD, MemDataWidth:DWORD, Mailbox:DWORD
    ;
    mov     edi, Mailbox
    mov     edx, MemAddress
    mov     eax, MemData
    mov     ecx, MemDataWidth
    ;
    cmp     ecx, 1
    jne     TryMemWriteWord
    mov     BYTE PTR [edx], al
    jmp     MemWriteReturn
    ;
TryMemWriteWord:
    cmp     ecx, 2
    jne     TryMemWriteDword
    mov     WORD PTR [edx], ax
    jmp     MemWriteReturn
    ;
TryMemWriteDword:
    cmp     ecx, 4
    jne     MemWriteReturn
    mov     DWORD PTR [edx], eax
    ;
MemWriteReturn:
    ret 
    ;
DoSwbpeTypeMemWrite    ENDP
        
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------
;  Procedure:   DoSwbpeTypeMemRead
; 
;  Input:       MemAddress   -  Memory Address for doing MEM Write
;               MemDataWidth -  Write Data Width (Byte=1/Word=2/DWord=4)
;               Mailbox      -  Data structure contains Data Store info
; 
;  Output:      Debuggers can update structure pointed by EDI via this MEM Read Break,
;               EAX contains MEM Read byte/word/dword value depends on PortDataWidth.
; 
;  Registers:   All are preserved
; 
;  Description: Function to do Memory Read and set EDI pointer to Mailbox structure,
;               so Debuggers can recognize the GUID and update status in the structure.
; 
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------

DoSwbpeTypeMemRead    PROC C   USES ECX EDX EDI MemAddress:DWORD, MemDataWidth:DWORD, Mailbox:DWORD
    ;
    xor     eax, eax
    mov     edi, Mailbox
    mov     edx, MemAddress
    mov     ecx, MemDataWidth
    ;
    cmp     ecx, 1
    jne     TryMemReadWord
    mov     al, BYTE PTR [edx]
    jmp     MemReadReturn
    ;
TryMemReadWord:
    cmp     ecx, 2
    jne     TryMemReadDword
    mov     ax, WORD PTR [edx]
    jmp     MemReadReturn
    ;
TryMemReadDword:
    cmp     ecx, 4
    jne     MemReadReturn
    mov     eax, DWORD PTR [edx]
    ;
MemReadReturn:
    ret 
    ;
DoSwbpeTypeMemRead    ENDP

        
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------
;  Procedure:   DoSwbpeTypeInt3
; 
;  Input:       Mailbox -  Data structure contains Data Store informations
; 
;  Output:      Debuggers can update structure pointed by EDI via this Interrupt Break
; 
;  Registers:   All are preserved
; 
;  Description: Function to do Interrupt 3 and set EDI pointer to Mailbox structure,
;               so Debuggers can recognize the GUID and update status in the structure.
; 
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------

DoSwbpeTypeInt3    PROC C   USES EDI    Mailbox:DWORD
    ;
    mov     edi, Mailbox
    int     3
    ret 
    ;
DoSwbpeTypeInt3    ENDP

      
END 
