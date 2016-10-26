#------------------------------------------------------------------------------
#
# Copyright (c) 2013 Intel Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Intel Corporation nor the names of its
# contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Module Name:
#
#  Flat32.S
#
# Abstract:
#
#  This is the code that goes from real-mode to protected mode.
#  It consumes the reset vector, configures the stack, and enters PEI.
#
#
#------------------------------------------------------------------------------


#
# CR0 cache control bit definition
#
.equ                    CR0_CACHE_DISABLE, 0x040000000
.equ                    CR0_NO_WRITE,      0x020000000

.macro RET32
    jmp    %esp
.endm

#
# ROM/SPI/MEMORY Definitions
#
.equ  QUARK_DDR3_MEM_BASE_ADDRESS, (0x000000000)		# Memory Base Address = 0
.equ  QUARK_MAX_DDR3_MEM_SIZE_BYTES, (0x80000000)		# DDR3 Memory Size = 2GB
.equ  QUARK_ESRAM_MEM_BASE_ADDRESS, (QUARK_DDR3_MEM_BASE_ADDRESS + QUARK_MAX_DDR3_MEM_SIZE_BYTES)		# eSRAM Memory above DDR3
.equ  QUARK_ESRAM_MEM_SIZE_BYTES, (0x00080000)		# eSRAM Memory Size = 512K
.equ  QUARK_STACK_SIZE_BYTES, (0x008000)			# Quark stack size = 32K
.equ	QUARK_STACK_BASE_ADDRESS, (QUARK_ESRAM_MEM_BASE_ADDRESS + QUARK_ESRAM_MEM_SIZE_BYTES) - QUARK_STACK_SIZE_BYTES		# Top of eSRAM - stack size

#
# RTC/CMOS definitions
#
.equ  RTC_INDEX, (0x70)
.equ    NMI_DISABLE, (0x80)	# Bit7=1 disables NMI
.equ    NMI_ENABLE, (0x00)	# Bit7=0 disables NMI
.equ  RTC_DATA, (0x71)

#
# PCI Configuration definitions
#
.equ  PCI_CFG, (0x80000000) # PCI configuration access mechanism
.equ  PCI_ADDRESS_PORT, (0xCF8)
.equ  PCI_DATA_PORT, (0xCFC)

#
# Quark PCI devices
#
.equ  HOST_BRIDGE_PFA, (0x0000)   # B0:D0:F0 (Host Bridge)
.equ  ILB_PFA, (0x00F8)	    # B0:D31:F0 (Legacy Block)

#
# ILB PCI Config Registers
#
.equ  BDE, (0x0D4)                                # BIOS Decode Enable register
.equ    DECODE_ALL_REGIONS_ENABLE, (0xFF000000)	  # Decode all BIOS decode ranges

#
# Host Bridge PCI Config Registers
#
.equ  MESSAGE_BUS_CONTROL_REG, (0xD0)       # Message Bus Control Register
.equ    SB_OPCODE_FIELD, (0x18)	            # Bit location of Opcode field
.equ      OPCODE_SIDEBAND_REG_READ, (0x10)	# Read opcode
.equ      OPCODE_SIDEBAND_REG_WRITE, (0x11) # Write opcode
.equ    SB_PORT_FIELD, (0x10)               # Bit location of Port ID field
.equ      MEMORY_ARBITER_PORT_ID, (0x00)
.equ      HOST_BRIDGE_PORT_ID, (0x03)
.equ      MEMORY_MANAGER_PORT_ID, (0x05)
.equ    SB_ADDR_FIELD, (0x08)               # Bit location of Register field
.equ    SB_BE_FIELD, (0x04)	                # Bit location of Byte Enables field
.equ      ALL_BYTE_EN, (0x0F)	              # All Byte Enables
.equ  MESSAGE_DATA_REG, (0xD4)              # Message Data Register

#
# Memory Arbiter Config Registers
#
.equ  AEC_CTRL_OFFSET, (0x00)

#
# Host Bridge Config Registers
#
.equ  HMISC2_OFFSET, (0x03) # PCI configuration access mechanism
.equ    OR_PM_FIELD, (0x10)

.equ  HMBOUND_OFFSET, (0x08)
.equ    HMBOUND_ADDRESS, (QUARK_DDR3_MEM_BASE_ADDRESS + QUARK_MAX_DDR3_MEM_SIZE_BYTES + QUARK_ESRAM_MEM_SIZE_BYTES)
.equ  HECREG_OFFSET, (0x09)
.equ    EC_BASE, (0xE0000000)
.equ    EC_ENABLE, (0x01)
.equ  HLEGACY_OFFSET, (0x0A)
.equ    NMI, (0x00004000)                 # Bit location of Register field
.equ    SMI, (0x00001000)                 # Bit location of Register field
.equ    INTR, (0x00000400)                # Bit location of Register field

#
# Memory Manager Config Registers
#
.equ  ESRAMPGCTRL_BLOCK_OFFSET, (0x82)
.equ  ESRAM_ADDRESS_2G, (0x10000080)

#
# Contrary to the name, this file contains 16 bit code as well.
#
.text
#----------------------------------------------------------------------------
#
# Procedure:    _ModuleEntryPoint
#
# Input:        None
#
# Output:       None
#
# Destroys:     Assume all registers
#
# Description:
#
#   Transition to non-paged flat-model protected mode from a
#   hard-coded GDT that provides exactly two descriptors.
#   This is a bare bones transition to protected mode only
#   used for a while in PEI and possibly DXE.
#
#   After enabling protected mode, a far jump is executed to
#   transfer to PEI using the newly loaded GDT.
#
# Return:       None
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):

  #
  # Warm Reset (INIT#) check.
  #
  .byte   0xbe,0x00,0xf0   #movw    $0xF000, %si
  .byte   0x8e,0xde        #movw    %si, %ds
  .byte   0xbe,0xf0,0xff   #movw    $0xFFF0, %si
  .byte   0x80,0x3c,0xea   #cmpb    $0xEA, (%si)          # Is it warm reset ?
  jne     NotWarmReset     # Jump if not.
  .byte   0xb0,0x08        #movb    $0x08, %al
  .byte   0xba,0xf9,0x0c   #movw    $0xcf9, %dx
  .byte   0xee             #outb    %al, %dx
  .byte   0xb0,0x55        #movb    $0x55, %al
  .byte   0xe6,0x80        #outb    %al, $0x80
  jmp     .
NotWarmReset: 

  #
  # Load the GDT table in GdtDesc
  #
  .byte   0x66,0xbe        #movl    $GdtDesc, %esi
  .long   GdtDesc
  
  .byte   0x66,0x2e,0x0f,0x01,0x14   #lgdt    %cs:(%si)

  #
  # Transition to 16 bit protected mode
  #
  .byte   0x0f,0x20,0xc0       #movl    %cr0, %eax                  # Get control register 0
  .byte   0x66,0x83,0xc8,0x03  #orl     $0x0000003, %eax           # Set PE bit (bit #0) & MP bit (bit #1)
  .byte   0x0f,0x22,0xc0       #movl    %eax, %cr0                  # Activate protected mode

  #
  # Now we're in 16 bit protected mode
  # Set up the selectors for 32 bit protected mode entry
  # 
  .byte   0xb8                 #movw    SYS_DATA_SEL, %ax
  .word   SYS_DATA_SEL
  
  .byte   0x8e,0xd8            #movw    %ax, %ds
  .byte   0x8e,0xc0            #movw    %ax, %es
  .byte   0x8e,0xe0            #movw    %ax, %fs
  .byte   0x8e,0xe8            #movw    %ax, %gs
  .byte   0x8e,0xd0            #movw    %ax, %ss

  #
  # Transition to Flat 32 bit protected mode
  # The jump to a far pointer causes the transition to 32 bit mode
  #
  .byte   0x66,0xbe            #movl   ProtectedModeEntryLinearAddress, %esi
  .long   ProtectedModeEntryLinearAddress 
  .byte   0x66,0x2e,0xff,0x2c  #jmp    %cs:(%esi)

#
# Protected mode portion initializes stack, configures cache, and calls C entry point
#

#----------------------------------------------------------------------------
#
# Procedure:    ProtectedModeEntryPoint
#
# Input:				Executing in 32 Bit Protected (flat) mode
#								cs: 0-4GB
#								ds: 0-4GB
#								es: 0-4GB
#								fs: 0-4GB
#								gs: 0-4GB
#								ss: 0-4GB
#								Maskable interrupts disabled (IF=0)
#               NMI disabled (RTC port)
#               SMI disabled (HMISC2 register)
#
# Output:       This function never returns
#
# Destroys:
#               eax
#               ebx
#               ecx
#	              esi
#               edi
#               ebp
#	              esp
#
# Description:
#								Perform any essential early platform initilaisation
#               Setup a stack
#               Call the main EDKII Sec C code
#
#----------------------------------------------------------------------------
#
# This signature is used by build tools to locate this entry point in the 32-bit SPI code
#
.byte   0x53,0x50,0x49,0x20,0x45,0x6e,0x74,0x72,0x79,0x20,0x50,0x6f,0x69,0x6e,0x74,0x20     #'SPI Entry Point '
ASM_GLOBAL ASM_PFX(ProtectedModeEntryPoint)
ASM_PFX(ProtectedModeEntryPoint):

	leal	L0, %esp
	jmp	stackless_EarlyPlatformInit
L0:

  #
  # Set up stack pointer
  #
  movl    $QUARK_STACK_BASE_ADDRESS, %esp
  movl    $QUARK_STACK_SIZE_BYTES, %esi
  addl    %esi, %esp                          # ESP = top of stack (stack grows downwards).

  #
  # Check if force recovery conditions met.
  #
  call    ASM_PFX(PlatformIsForceRecoveryConditionsMet)
  testl   %eax, %eax
  jz      NormalBoot

  #
  # Copy fixed recovery image to SRAM and call it.
  # eax has address of routine to copy Fixed Recovery image to SRAM and call it.
  # copy this routine to QUARK_STACK_BASE_ADDRESS, we have just setup the stack
  # so it won't reach down to QUARK_STACK_BASE_ADDRESS yet.
  # Doing this since we don't want the copy routine to copy over itself.
  #
  mov     $(0x00000100), %ecx               # 256 DWORDs = 1K
  movl    $QUARK_STACK_BASE_ADDRESS, %edi   # The destination address in QUARK_STACK_BASE_ADDRESS.
CopyRoutineLoop:
  movl    %ds:(%eax), %ebx
  movl    %ebx, %ds:(%edi)
  addl    $(0x04), %eax
  addl    $(0x04), %edi
  loopl   CopyRoutineLoop

  #
  # call routine which will copy and call fixed recovery image.
  #
  movl    $QUARK_STACK_BASE_ADDRESS, %eax
  call    *%eax

  #
  # Forever loop at end of last routine so should not return here.
  #
  jmp     .

NormalBoot:


  #
  # Store the the BIST value in EBP
  #
  movl    $0, %ebp    # No processor BIST on Quark

  #
  # Push processor count to stack first, then BIST status (AP then BSP)
  #
  movl    $1, %eax
  cpuid
  shrl    $16, %ebx
  andl    $0x000000FF, %ebx
  cmpb    $1, %bl
  jae     PushProcessorCount

  #
  # Some processors report 0 logical processors.  Effectively 0 = 1.
  # So we fix up the processor count
  #
  incl    %ebx

PushProcessorCount:  
  pushl   %ebx

  #
  # We need to implement a long-term solution for BIST capture.  For now, we just copy BSP BIST
  # for all processor threads
  #
  xorl    %ecx, %ecx
  movb    %bl, %cl

PushBist: 
  pushl   %ebp
  loop    PushBist

  #
  # Pass entry point of the PEI core
  #
  movl $0xFFFFFFE0, %edi
  pushl %ds:(%edi)

  #
  # Pass BFV into the PEI Core
  # Sec/Pei FV Base Address in eSRAM is QUARK_ESRAM_MEM_BASE_ADDRESS + PcdFvSecurityHeaderSize.
  #
  movl    _gPcd_FixedAtBuild_PcdFvSecurityHeaderSize, %eax
  addl    $QUARK_ESRAM_MEM_BASE_ADDRESS, %eax
  pushl   %eax

  #
  # Pass Temp Ram Base into the PEI Core
  #
  pushl   $QUARK_STACK_BASE_ADDRESS

  #
  # Pass stack size into the PEI Core
  #
  pushl   $QUARK_STACK_SIZE_BYTES

  #
  # Pass Control into the PEI Core
  #
  call SecStartup

  #
  # PEI Core should never return to here, this is just to capture an invalid return.
  #
  jmp     .

#----------------------------------------------------------------------------
#
# Procedure:    stackless_EarlyPlatformInit
#
# Input:        esp - Return address
#
# Output:       None
#
# Destroys:     Assume all registers
#
# Description:
#				Any early platform initialisation required
#
# Return:
#			None
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(stackless_EarlyPlatformInit)
ASM_PFX(stackless_EarlyPlatformInit):

  #
  # Save return address
  #
  movl  %esp, %ebp
  
  #
  # Ensure cache is disabled.
  #
  movl %cr0, %eax
  orl $(CR0_CACHE_DISABLE + CR0_NO_WRITE), %eax
  invd
  movl    %eax, %cr0

  #
  # Enable NMI operation
  # Good convention suggests you should read back RTC data port after
  # accessing the RTC index port.
  #
  movb $(NMI_ENABLE), %al
  movw $(RTC_INDEX), %dx
  outb %al, %dx
  movw $(RTC_DATA), %dx
  inb  %dx, %al

  #
  # Clear Host Bridge SMI, NMI, INTR fields
  #
  movl	$((OPCODE_SIDEBAND_REG_READ << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HLEGACY_OFFSET << SB_ADDR_FIELD)), %ecx
	leal	L3, %esp
	jmp	stackless_SideBand_Read
L3:
  andl $~(NMI + SMI + INTR), %eax      # Clear NMI, SMI, INTR fields
  movl	$((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HLEGACY_OFFSET << SB_ADDR_FIELD)), %ecx
	leal	L4, %esp
	jmp	stackless_SideBand_Write
L4:
  
  #
  # Set up the HMBOUND register
  #  
  movl $(HMBOUND_ADDRESS), %eax      # Data (Set HMBOUND location)
  movl	$((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HMBOUND_OFFSET << SB_ADDR_FIELD)), %ecx
	leal	L5, %esp
	jmp	stackless_SideBand_Write
L5:  

  #
  # Enable PCIEXBAR
  #  
  movl $(EC_BASE + EC_ENABLE), %eax      # Data
  movl	$((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (MEMORY_ARBITER_PORT_ID << SB_PORT_FIELD) | (AEC_CTRL_OFFSET << SB_ADDR_FIELD)), %ecx
	leal	L6, %esp
	jmp	stackless_SideBand_Write
L6: 

  movl $(EC_BASE + EC_ENABLE), %eax      # Data
  movl	$((OPCODE_SIDEBAND_REG_WRITE << SB_OPCODE_FIELD) | (HOST_BRIDGE_PORT_ID << SB_PORT_FIELD) | (HECREG_OFFSET << SB_ADDR_FIELD)), %ecx
	leal	L7, %esp
	jmp	stackless_SideBand_Write
L7: 

  #
  # Restore return address
  #
  movl	%ebp, %esp
  RET32  
    
#----------------------------------------------------------------------------
#
# Procedure:    stackless_SideBand_Read
#
# Input:        esp - return address
#								ecx[15:8] - Register offset
#								ecx[23:16] - Port ID
#								ecx[31:24] - Opcode
#
# Output:       eax - Data read
#
# Destroys:
#								eax
#								ebx
#								cl
#								esi
#
# Description:
#				Perform requested sideband read
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(stackless_SideBand_Read)
ASM_PFX(stackless_SideBand_Read):

  movl	%esp, %esi      # Save the return address
  
	#
	# Load the SideBand Packet Register to generate the transaction
	#
  movl	$((PCI_CFG) | (HOST_BRIDGE_PFA << 8) | (MESSAGE_BUS_CONTROL_REG)), %ebx   # PCI Configuration address
	movb  $(ALL_BYTE_EN << SB_BE_FIELD), %cl      # Set all Byte Enable bits
  xchgl	%ecx, %eax
	leal	L8, %esp
	jmp	stackless_PCIConfig_Write
L8:
  xchgl	%ecx, %eax

	#
	# Read the SideBand Data Register
	#	
  movl	$((PCI_CFG) | (HOST_BRIDGE_PFA << 8) | (MESSAGE_DATA_REG)), %ebx   # PCI Configuration address
	leal	L9, %esp
	jmp	stackless_PCIConfig_Read
L9:
	
  movl	%esi, %esp      # Restore the return address
  RET32
  

#----------------------------------------------------------------------------
#
# Procedure:    stackless_SideBand_Write
#
# Input:        esp - return address
#								eax - Data
#								ecx[15:8] - Register offset
#								ecx[23:16] - Port ID
#								ecx[31:24] - Opcode
#
# Output:       None
#
# Destroys:
#								ebx
#								cl
#								esi
#
# Description:
#				Perform requested sideband write
#
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(stackless_SideBand_Write)
ASM_PFX(stackless_SideBand_Write):

  movl	%esp, %esi      # Save the return address

	#
	# Load the SideBand Data Register with the data
	#
  movl	$((PCI_CFG) | (HOST_BRIDGE_PFA << 8) | (MESSAGE_DATA_REG)), %ebx   # PCI Configuration address
	leal	L10, %esp
	jmp	stackless_PCIConfig_Write
L10:
	
	#
	# Load the SideBand Packet Register to generate the transaction
	#	
  movl	$((PCI_CFG) | (HOST_BRIDGE_PFA << 8) | (MESSAGE_BUS_CONTROL_REG)), %ebx   # PCI Configuration address
	movb  $(ALL_BYTE_EN << SB_BE_FIELD), %cl      # Set all Byte Enable bits
  xchgl	%ecx, %eax
	leal	L11, %esp
	jmp	stackless_PCIConfig_Write
L11:
  xchgl	%ecx, %eax
  	
  movl	%esi, %esp      # Restore the return address
  RET32
  
  
#----------------------------------------------------------------------------
#
# Procedure:    stackless_PCIConfig_Write
#
# Input:        esp - return address
#								eax - Data to write
#								ebx - PCI Config Address
#
# Output:       None
#
# Destroys:
#								dx
#
# Description:
#				Perform a DWORD PCI Configuration write
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(stackless_PCIConfig_Write)
ASM_PFX(stackless_PCIConfig_Write):
  
	#
	# Write the PCI Config Address to the address port
	#
	xchgl	%ebx, %eax
	movw	$(PCI_ADDRESS_PORT), %dx
	outl	%eax, %dx
	xchgl	%ebx, %eax
	  
	#
	# Write the PCI DWORD Data to the data port
	#
	movw	$(PCI_DATA_PORT), %dx
	outl	%eax, %dx
		  
  RET32
		
		
#----------------------------------------------------------------------------
#
# Procedure:    stackless_PCIConfig_Read
#
# Input:        esp - return address
#								ebx - PCI Config Address
#
# Output:       eax - Data read
#
# Destroys:
#								eax
#								dx
#
# Description:
#				Perform a DWORD PCI Configuration read
#
#----------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(stackless_PCIConfig_Read)
ASM_PFX(stackless_PCIConfig_Read):
		  
	#
	# Write the PCI Config Address to the address port
	#
	xchgl	%ebx, %eax
	movw	$(PCI_ADDRESS_PORT), %dx
	outl	%eax, %dx
	xchgl	%ebx, %eax
			  
	#
	# Read the PCI DWORD Data from the data port
	#
	movw	$(PCI_DATA_PORT), %dx
	inl	%dx, %eax
	
	RET32
		
				  
#
# ROM-based Global-Descriptor Table for the Tiano PEI Phase
#
.align 16
#
# GDT[0]: 000h: Null entry, never used.
#
.equ   NULL_SEL, . - GDT_BASE         # Selector [0]
GDT_BASE: 
BootGdtTable:   
        .long   0
        .long   0
#
# Linear code segment descriptor
#
.equ     LINEAR_CODE_SEL, . - GDT_BASE      # Selector [08h]
        .word   0xFFFF                      # limit 0FFFFh
        .word   0                           # base 0
        .byte   0
        .byte   0x9B                        # present, ring 0, data, expand-up, not-writable
        .byte   0xCF                        # page-granular, 32-bit
        .byte   0
#
# System data segment descriptor
#
.equ    SYS_DATA_SEL, . - GDT_BASE         # Selector [018h]
        .word   0xFFFF                      # limit 0FFFFh
        .word   0                           # base 0
        .byte   0
        .byte   0x93                        # present, ring 0, data, expand-up, not-writable
        .byte   0xCF                        # page-granular, 32-bit
        .byte   0

.equ            GDT_SIZE, . - BootGdtTable  # Size, in bytes

#
# GDT Descriptor
#
GdtDesc:                                     # GDT descriptor
       .word    GDT_SIZE - 1               
       .long    BootGdtTable        

ProtectedModeEntryLinearAddress:
ProtectedModeEntryLinearOffset:
       .long    ProtectedModeEntryPoint
       .word    LINEAR_CODE_SEL

#
# Used by C code
#
ASM_GLOBAL ASM_PFX(TopOfStack)
ASM_PFX(TopOfStack):
       .long        QUARK_STACK_BASE_ADDRESS + QUARK_STACK_SIZE_BYTES
