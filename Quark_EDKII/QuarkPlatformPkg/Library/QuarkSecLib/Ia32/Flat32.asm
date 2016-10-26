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
;  Flat32.asm
;
; Abstract:
;
;  This is the code that goes from real-mode to protected mode.
;  It consumes the reset vector, configures the stack, and enters PEI.
;
;
;------------------------------------------------------------------------------


;
; Define assembler characteristics
;
.686p
.model flat, c

;
; Include processor definitions
;
INCLUDE Ia32.inc
INCLUDE Platform.inc

PlatformIsForceRecoveryConditionsMet PROTO   C

;
; Declare PcdGet32 (PcdFvSecurityHeaderSize) as a double word.
;
EXTERNDEF   C   PcdGet32 (PcdFvSecurityHeaderSize):DWORD

;
; External and public declarations
;  TopOfStack is used by C code
;  SecStartup is the entry point to the C code
; Neither of these names can be modified without
; updating the C code.
;
PUBLIC  TopOfStack
EXTRN   SecStartup: NEAR

;
; Contrary to the name, this file contains 16 bit code as well.
;
_TEXT_REALMODE      SEGMENT PARA PUBLIC USE16 'CODE'
                    ASSUME  CS:_TEXT_REALMODE, DS:_TEXT_REALMODE

;----------------------------------------------------------------------------
;
; Procedure:    _ModuleEntryPoint
;
; Input:        None
;
; Output:       None
;
; Destroys:     Assume all registers
;
; Description:
;
;   Transition to non-paged flat-model protected mode from a
;   hard-coded GDT that provides exactly two descriptors.
;   This is a bare bones transition to protected mode only
;   used for a while in PEI and possibly DXE.
;
;   After enabling protected mode, a far jump is executed to
;   transfer to PEI using the newly loaded GDT.
;
; Return:       None
;
;----------------------------------------------------------------------------
align 16
_ModuleEntryPoint      PROC C PUBLIC

  ;
  ; Warm Reset (INIT#) check.
  ;
  mov     si, 0F000h
  mov     ds, si
  mov     si, 0FFF0h
  cmp     BYTE PTR [si], 0EAh   ; Is it warm reset ?
  jne     NotWarmReset          ; JIf not.

  mov     al, 08
  mov     dx, 0cf9h
  out     dx, al
  mov     al, 055h
  out     080h, al;
  jmp $  
NotWarmReset:

  ;
  ; Load the GDT table in GdtDesc
  ;
  mov     esi, OFFSET GdtDesc
  db      66h
  lgdt    fword ptr cs:[si]

  ;
  ; Transition to 16 bit protected mode
  ;
  mov     eax, cr0                   ; Get control register 0
  or      eax, 00000003h             ; Set PE bit (bit #0) & MP bit (bit #1)
  mov     cr0, eax                   ; Activate protected mode

  ;
  ; Now we're in 16 bit protected mode
  ; Set up the selectors for 32 bit protected mode entry
  ; 
  mov     ax, SYS_DATA_SEL
  mov     ds, ax
  mov     es, ax
  mov     fs, ax
  mov     gs, ax
  mov     ss, ax

  ;
  ; Transition to Flat 32 bit protected mode
  ; The jump to a far pointer causes the transition to 32 bit mode
  ;
  mov esi, offset ProtectedModeEntryLinearAddress
  jmp     fword ptr cs:[si]

_ModuleEntryPoint   ENDP

_TEXT_REALMODE      ENDS

.code 
;
; Protected mode portion initializes stack, configures cache, and calls C entry point
;

;----------------------------------------------------------------------------
;
; Procedure:    ProtectedModeEntryPoint
;
; Input:				Executing in 32 Bit Protected (flat) mode
;								cs: 0-4GB
;								ds: 0-4GB
;								es: 0-4GB
;								fs: 0-4GB
;								gs: 0-4GB
;								ss: 0-4GB
;								Maskable interrupts disabled (IF=0)
;               NMI disabled (RTC port)
;               SMI disabled (HMISC2 register)
;
; Output:       This function never returns
;
; Destroys:
;               eax
;               ebx
;               ecx
;	              esi
;               edi
;               ebp
;	              esp
;
; Description:
;								Perform any essential early platform initilaisation
;               Setup a stack
;               Call the main EDKII Sec C code
;
;----------------------------------------------------------------------------
;
; This signature is used by build tools to locate this entry point in the 32-bit SPI code
;
SPI_ENTRY_SIGNATURE	DB 01h DUP ('SPI Entry Point ')
ProtectedModeEntryPoint PROC NEAR C PUBLIC

  JMP32	stackless_EarlyPlatformInit

  ;
  ; Set up stack pointer
  ;
  mov     esp, QUARK_STACK_BASE_ADDRESS
  mov     esi, QUARK_STACK_SIZE_BYTES
  add     esp, esi                         ; ESP = top of stack (stack grows downwards).

  ;
  ; Check if force recovery conditions met.
  ;
  call    PlatformIsForceRecoveryConditionsMet
  test    eax, eax
  jz      NormalBoot

  ;
  ; Copy fixed recovery image to SRAM and call it.
  ; eax has address of routine to copy Fixed Recovery image to SRAM and call it.
  ; copy this routine to QUARK_STACK_BASE_ADDRESS, we have just setup the stack
  ; so it won't reach down to QUARK_STACK_BASE_ADDRESS yet.
  ; Doing this since we don't want the copy routine to copy over itself.
  ;
  mov     ecx, 000000100h                   ; 256 DWORDs = 1K
  mov     edi, QUARK_STACK_BASE_ADDRESS     ; The destination address in QUARK_STACK_BASE_ADDRESS.
CopyRoutineLoop:
  mov     ebx, DWORD PTR ds:[eax]
  mov     DWORD PTR ds:[edi], ebx
  add     eax, 04h
  add     edi, 04h
  loopd   CopyRoutineLoop

  ;
  ; call routine which will copy & call fixed recovery image.
  ;
  mov     eax, QUARK_STACK_BASE_ADDRESS
  call    eax

  ;
  ; Forever loop at end of last routine so should not return here.
  ;
  jmp     $

NormalBoot:

  ;
  ; Store the the BIST value in EBP
  ;
  mov     ebp, 00h				; No processor BIST on Quark
  
  ;
  ; Push processor count to stack first, then BIST status (AP then BSP)
  ;
  mov     eax, 1
  cpuid
  shr     ebx, 16
  and     ebx, 0000000FFh
  cmp     bl, 1
  jae     PushProcessorCount

  ;
  ; Some processors report 0 logical processors.  Effectively 0 = 1.
  ; So we fix up the processor count
  ;
  inc     ebx

PushProcessorCount:  
  push    ebx

  ;
  ; We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  ; for all processor threads
  ;
  xor     ecx, ecx
  mov     cl, bl
PushBist:
  push    ebp
  loop    PushBist

  ;
  ; Pass entry point of the PEI core
  ;
  mov  edi, 0FFFFFFE0h
  push DWORD PTR ds:[edi]
     
  ;
  ; Pass BFV into the PEI Core
  ; Sec/Pei FV Base Address in eSRAM is QUARK_ESRAM_MEM_BASE_ADDRESS + PcdFvSecurityHeaderSize.
  ;
  mov     eax, PcdGet32 (PcdFvSecurityHeaderSize)  ; eax <- Security Header Size.
  add     eax, QUARK_ESRAM_MEM_BASE_ADDRESS      ; eax <- Sec/Pei FV Base Address in eSRAM.

  push    eax

  ;
  ; Pass Temp Ram Base into the PEI Core
  ;
  push    QUARK_STACK_BASE_ADDRESS

  ;
  ; Pass stack size into the PEI Core
  ; It should be QUARK_STACK_SIZE_BYTES
  ;
  push	QUARK_STACK_SIZE_BYTES

  ;
  ; Pass Control into the PEI Core
  ;
  call SecStartup
  
  ;
  ; PEI Core should never return to here, this is just to capture an invalid return.
  ;
  jmp     $

ProtectedModeEntryPoint ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_EarlyPlatformInit
;
; Input:        esp - Return address
;
; Output:       None
;
; Destroys:     Assume all registers
;
; Description:
;				Any early platform initialisation required
;
; Return:
;			None
;
;----------------------------------------------------------------------------
stackless_EarlyPlatformInit  PROC NEAR C PUBLIC

  ;
  ; Save return address
  ;
  mov	ebp, esp

  ;
  ; Ensure cache is disabled.
  ;
  mov eax, cr0
  or  eax, CR0_CACHE_DISABLE + CR0_NO_WRITE
  invd
  mov cr0, eax

  ;
  ; Enable NMI operation
  ; Good convention suggests you should read back RTC data port after
  ; accessing the RTC index port.
  ;
  mov al, NMI_ENABLE
  mov dx, RTC_INDEX
  out dx, al
  mov dx, RTC_DATA
  in  al, dx

  ;
  ; Clear Host Bridge SMI, NMI, INTR fields
  ;
  mov	ecx, (OPCODE_SIDEBAND_REG_READ SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HLEGACY_OFFSET SHL SB_ADDR_FIELD)
  JMP32	stackless_SideBand_Read
  and	eax, NOT(NMI + SMI + INTR)	; Clear NMI, SMI, INTR fields
  mov	ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HLEGACY_OFFSET SHL SB_ADDR_FIELD)
  JMP32	stackless_SideBand_Write


  ;
  ; Set up the HMBOUND register
  ;
  mov	eax, HMBOUND_ADDRESS		; Data (Set HMBOUND location)
  mov	ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HMBOUND_OFFSET SHL SB_ADDR_FIELD)
  JMP32	stackless_SideBand_Write

  ;
  ; Enable PCIEXBAR
  ;
  mov	eax, (EC_BASE + EC_ENABLE)			; Data
  mov	ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (MEMORY_ARBITER_PORT_ID SHL SB_PORT_FIELD) OR (AEC_CTRL_OFFSET SHL SB_ADDR_FIELD)
  JMP32	stackless_SideBand_Write
	
  mov	eax, (EC_BASE + EC_ENABLE)			; Data
  mov	ecx, (OPCODE_SIDEBAND_REG_WRITE SHL SB_OPCODE_FIELD) OR (HOST_BRIDGE_PORT_ID SHL SB_PORT_FIELD) OR (HECREG_OFFSET SHL SB_ADDR_FIELD)
  JMP32	stackless_SideBand_Write

  ;
  ; Restore return address
  ;
  mov	esp, ebp
  RET32
	
stackless_EarlyPlatformInit ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_SideBand_Read
;
; Input:        esp - return address
;								ecx[15:8] - Register offset
;								ecx[23:16] - Port ID
;								ecx[31:24] - Opcode
;
; Output:       eax - Data read
;
; Destroys:
;								eax
;								ebx
;								cl
;								esi
;
; Description:
;				Perform requested sideband read
;
;----------------------------------------------------------------------------
stackless_SideBand_Read  PROC NEAR C PUBLIC

	mov	esi, esp				; Save the return address
	
	;
	; Load the SideBand Packet Register to generate the transaction
	;	
	mov	ebx, PCI_CFG OR (HOST_BRIDGE_PFA SHL 8) OR MESSAGE_BUS_CONTROL_REG	; PCI Configuration address
	mov	cl, (ALL_BYTE_EN SHL SB_BE_FIELD)		; Set all Byte Enable bits
	xchg	eax, ecx
	JMP32	stackless_PCIConfig_Write
	xchg	eax, ecx
	
	;
	; Read the SideBand Data Register
	;	
	mov	ebx, PCI_CFG OR (HOST_BRIDGE_PFA SHL 8) OR MESSAGE_DATA_REG		; PCI Configuration address
	JMP32	stackless_PCIConfig_Read
	
	mov	esp, esi				; Restore the return address
	RET32

stackless_SideBand_Read ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_SideBand_Write
;
; Input:        esp - return address
;								eax - Data
;								ecx[15:8] - Register offset
;								ecx[23:16] - Port ID
;								ecx[31:24] - Opcode
;
; Output:       None
;
; Destroys:
;								ebx
;								cl
;								esi
;
; Description:
;				Perform requested sideband write
;
;
;----------------------------------------------------------------------------
stackless_SideBand_Write  PROC NEAR C PUBLIC

	mov	esi, esp				; Save the return address

	;
	; Load the SideBand Data Register with the data
	;
	mov	ebx, PCI_CFG OR (HOST_BRIDGE_PFA SHL 8) OR MESSAGE_DATA_REG	; PCI Configuration address
	JMP32	stackless_PCIConfig_Write
	
	;
	; Load the SideBand Packet Register to generate the transaction
	;	
	mov	ebx, PCI_CFG OR (HOST_BRIDGE_PFA SHL 8) OR MESSAGE_BUS_CONTROL_REG	; PCI Configuration address
	mov	cl, (ALL_BYTE_EN SHL SB_BE_FIELD)		; Set all Byte Enable bits
	xchg	eax, ecx
	JMP32	stackless_PCIConfig_Write
	xchg	eax, ecx
		
	mov	esp, esi				; Restore the return address
	RET32

stackless_SideBand_Write ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_PCIConfig_Write
;
; Input:        esp - return address
;								eax - Data to write
;								ebx - PCI Config Address
;
; Output:       None
;
; Destroys:
;								dx
;
; Description:
;				Perform a DWORD PCI Configuration write
;
;----------------------------------------------------------------------------
stackless_PCIConfig_Write  PROC NEAR C PUBLIC

	;
	; Write the PCI Config Address to the address port
	;
	xchg	eax, ebx
	mov	dx, PCI_ADDRESS_PORT
	out	dx, eax
	xchg	eax, ebx

	;
	; Write the PCI DWORD Data to the data port
	;
	mov	dx, PCI_DATA_PORT
	out	dx, eax
	
	RET32

stackless_PCIConfig_Write ENDP

;----------------------------------------------------------------------------
;
; Procedure:    stackless_PCIConfig_Read
;
; Input:        esp - return address
;								ebx - PCI Config Address
;
; Output:       eax - Data read
;
; Destroys:
;								eax
;								dx
;
; Description:
;				Perform a DWORD PCI Configuration read
;
;----------------------------------------------------------------------------
stackless_PCIConfig_Read  PROC NEAR C PUBLIC

	;
	; Write the PCI Config Address to the address port
	;
	xchg	eax, ebx
	mov	dx, PCI_ADDRESS_PORT
	out	dx, eax
	xchg	eax, ebx

	;
	; Read the PCI DWORD Data from the data port
	;
	mov	dx, PCI_DATA_PORT
	in	eax, dx
	
	RET32

stackless_PCIConfig_Read ENDP

;
; ROM-based Global-Descriptor Table for the Tiano PEI Phase
;
align 16
PUBLIC  BootGdtTable

;
; GDT[0]: 0x00: Null entry, never used.
;
NULL_SEL        equ     $ - GDT_BASE        ; Selector [0]
GDT_BASE:
BootGdtTable    DD      0
                DD      0
;
; Linear code segment descriptor
;
LINEAR_CODE_SEL equ     $ - GDT_BASE        ; Selector [0x8]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      09Bh                        ; present, ring 0, data, expand-up, not-writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0
;
; System data segment descriptor
;
SYS_DATA_SEL    equ     $ - GDT_BASE        ; Selector [0x10]
        DW      0FFFFh                      ; limit 0xFFFF
        DW      0                           ; base 0
        DB      0
        DB      093h                        ; present, ring 0, data, expand-up, not-writable
        DB      0CFh                        ; page-granular, 32-bit
        DB      0

GDT_SIZE        EQU     $ - BootGDTtable    ; Size, in bytes

;
; GDT Descriptor
;
GdtDesc:                                    ; GDT descriptor
        DW      GDT_SIZE - 1                ; GDT limit
        DD      OFFSET BootGdtTable         ; GDT base address

ProtectedModeEntryLinearAddress   LABEL   FWORD
ProtectedModeEntryLinearOffset    LABEL   DWORD
  DD      OFFSET ProtectedModeEntryPoint  ; Offset of our 32 bit code
  DW      LINEAR_CODE_SEL
  
;
; Used by C code
;
TopOfStack DD QUARK_STACK_BASE_ADDRESS + QUARK_STACK_SIZE_BYTES


END
