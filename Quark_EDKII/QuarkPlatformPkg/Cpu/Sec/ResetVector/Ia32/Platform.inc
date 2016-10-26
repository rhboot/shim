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
;------------------------------------------------------------------------------
; 
; Module Name:
;
;   Platform.inc
;
; Abstract:
;
;   Quark A0 Platform Specific Definitions
;
;------------------------------------------------------------------------------

JMP32 MACRO FunctionName
	lea	esp, @F
	jmp	FunctionName
@@:	
ENDM

RET32 MACRO
    jmp    esp
ENDM

;
; ROM/SPI/MEMORY Definitions
;
QUARK_DDR3_MEM_BASE_ADDRESS		EQU 000000000h		; Memory Base Address = 0
QUARK_MAX_DDR3_MEM_SIZE_BYTES		EQU 080000000h		; DDR3 Memory Size = 2GB
QUARK_ESRAM_MEM_BASE_ADDRESS		EQU (QUARK_DDR3_MEM_BASE_ADDRESS + QUARK_MAX_DDR3_MEM_SIZE_BYTES)	; eSRAM Memory above DDR3
QUARK_ESRAM_MEM_SIZE_BYTES		EQU 000080000h		; eSRAM Memory Size = 512K
QUARK_STACK_SIZE_BYTES			EQU 008000h			; Quark stack size = 32K
QUARK_STACK_BASE_ADDRESS			EQU (QUARK_ESRAM_MEM_BASE_ADDRESS+QUARK_ESRAM_MEM_SIZE_BYTES)-QUARK_STACK_SIZE_BYTES	; Top of eSRAM - stack size
QUARK_CMH_SIZE_BYTES			EQU 0400h			; Quark Module Header size = 1024bytes
QUARK_ESRAM_STAGE1_BASE_ADDRESS		EQU (QUARK_ESRAM_MEM_BASE_ADDRESS + QUARK_CMH_SIZE_BYTES)		; Start of Stage1 code in eSRAM
	
;
; RTC/CMOS definitions
;
RTC_INDEX			EQU 070h
	NMI_DISABLE	EQU 080h		; Bit7 disables NMI
RTC_DATA			EQU 071h
	
;
; PCI Configuration definitions
;
PCI_CFG						EQU 1 SHL 01Fh	; PCI configuration access mechanism
PCI_ADDRESS_PORT	EQU 0CF8h
PCI_DATA_PORT			EQU 0CFCh

;
; Quark PCI devices
;
HOST_BRIDGE_PFA				EQU 0000h					; B0:D0:F0 (Host Bridge)
ILB_PFA					EQU 00F8h					; B0:D31:F0 (Legacy Block)

;
; ILB PCI Config Registers
;
BDE															EQU 0D4h				; BIOS Decode Enable register
	DECODE_ALL_REGIONS_ENABLE			EQU 0FF000000h	; Decode all BIOS decode ranges


;
; iLB Reset Register
;
ILB_RESET_REG			EQU 0CF9h
	CF9_WARM_RESET		EQU	02h
	CF9_COLD_RESET		EQU	08h

;
; Host Bridge PCI Config Registers
;
MESSAGE_BUS_CONTROL_REG										EQU 0D0h	; Message Bus Control Register
	SB_OPCODE_FIELD								EQU 018h	; Bit location of Opcode field
		OPCODE_SIDEBAND_REG_READ		EQU 010h	; Read opcode
		OPCODE_SIDEBAND_REG_WRITE		EQU 011h	; Write opcode
		OPCODE_SIDEBAND_ALT_REG_READ	EQU 06h		; Alternate Read opcode
		OPCODE_SIDEBAND_ALT_REG_WRITE	EQU 07h		; Alternate Write opcode
		OPCODE_WARM_RESET_REQUEST		EQU 0F4h	; Reset Warm
		OPCODE_COLD_RESET_REQUEST		EQU 0F5h	; Reset Cold
	SB_PORT_FIELD									EQU 010h	; Bit location of Port ID field
		MEMORY_ARBITER_PORT_ID								EQU 00h
		HOST_BRIDGE_PORT_ID								EQU 03h
		RMU_PORT_ID								EQU 04h
		MEMORY_MANAGER_PORT_ID								EQU 05h	
		SOC_UNIT_PORT_ID							EQU	031h
	SB_ADDR_FIELD									EQU 008h	; Bit location of Register field
	SB_BE_FIELD										EQU	004h	; Bit location of Byte Enables field
		ALL_BYTE_EN									EQU	00Fh	; All Byte Enables
MESSAGE_DATA_REG											EQU 0D4h	; Message Data Register

;
; Memory Arbiter Config Registers
;
AEC_CTRL_OFFSET		EQU 00h

;
; Host Bridge Config Registers
;
HMISC2_OFFSET			EQU 03h
	OR_PM_FIELD			EQU 010h
		SMI_EN				EQU 1 SHL 13h

HMBOUND_OFFSET		EQU 08h
	HMBOUND_ADDRESS	EQU (QUARK_DDR3_MEM_BASE_ADDRESS + QUARK_MAX_DDR3_MEM_SIZE_BYTES + QUARK_ESRAM_MEM_SIZE_BYTES)
	HMBOUND_LOCK	EQU 00000001h
HECREG_OFFSET		EQU 09h
	EC_BASE			EQU 0E0000000h
	EC_ENABLE		EQU 01h


;
; Memory Manager Config Registers
;
ESRAMPGCTRL_BLOCK_OFFSET	EQU 082h
	BLOCK_ENABLE_PG			EQU 010000000h
	ESRAM_ADDRESS_2G		EQU ((01h SHL 1Ch) + (QUARK_ESRAM_MEM_BASE_ADDRESS SHR 18h))
BIMRVCTL_OFFSET				EQU 019h
	ENABLE_IMR_INTERRUPT	EQU 080000000h	
	
;
; SOC UNIT Debug Registers
;
CFGSTICKY_W1_OFFSET					EQU 050h
	FORCE_COLD_RESET			EQU	00000001h
CFGSTICKY_RW_OFFSET    				EQU 051h
	RESET_FOR_ESRAM_LOCK		EQU	00000020h
	RESET_FOR_HMBOUND_LOCK		EQU	00000040h
CFGNONSTICKY_W1_OFFSET				EQU 052h
	FORCE_WARM_RESET			EQU	00000001h


	
	
