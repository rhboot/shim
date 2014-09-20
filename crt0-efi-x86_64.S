/* crt0-efi-x86_64.S - x86_64 EFI startup code.
 *
 * Copyright 2014 Red Hat, Inc. <pjones@redhat.com>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
	.section	.text.head

	/*
	 * Magic "MZ" signature for PE/COFF
	 */
	.globl	ImageBase
ImageBase:
	.ascii	"MZ"
	.skip	58				// 'MZ' + pad + offset == 64
	.long	pe_header - ImageBase		// Offset to the PE header.
	.long	0x0eba1f0e			/* terrifying code */
	.long	0xcd09b400			/* terrifying code */
	.long	0x4c01b821			/* terrifying code */
	.short	0x21cd				/* terrfiying code */
	.ascii	"The only winning move is not to play.\r\r\n$" /* DOS text */
	.skip	9
pe_header:
	.ascii	"PE"
	.short 	0
coff_header:
	.short	0x8664				// x86_64
	.short	1				// nr_sections
	.long	0 				// TimeDateStamp
	.long	0				// PointerToSymbolTable
	.long	0				// NumberOfSymbols
	.short	section_table - optional_header	// SizeOfOptionalHeader
	.short	0x206				// Characteristics.
						// IMAGE_FILE_DEBUG_STRIPPED |
						// IMAGE_FILE_EXECUTABLE_IMAGE |
						// IMAGE_FILE_LINE_NUMS_STRIPPED
optional_header:
	.short	0x20b				// PE32+ format
	.byte	0x02				// MajorLinkerVersion
	.byte	0x18				// MinorLinkerVersion
	.long	_edata - _start			// SizeOfCode
	.long	0				// SizeOfInitializedData
	.long	0				// SizeOfUninitializedData
	.long	_start - ImageBase		// AddressOfEntryPoint
	.long	_start - ImageBase		// BaseOfCode

extra_header_fields:
	.quad	0				// ImageBase
	.long	0x20				// SectionAlignment
	.long	0x8				// FileAlignment
	.short	0				// MajorOperatingSystemVersion
	.short	0				// MinorOperatingSystemVersion
	.short	0				// MajorImageVersion
	.short	0				// MinorImageVersion
	.short	0				// MajorSubsystemVersion
	.short	0				// MinorSubsystemVersion
	.long	0				// Win32VersionValue

	.long	_edata - ImageBase		// SizeOfImage

	// Everything before the kernel image is considered part of the header
	.long	_start - ImageBase		// SizeOfHeaders
	.long	0				// CheckSum
	.short	EFI_SUBSYSTEM			// Subsystem
	.short	0				// DllCharacteristics
	.quad	0				// SizeOfStackReserve
	.quad	0				// SizeOfStackCommit
	.quad	0				// SizeOfHeapReserve
	.quad	0				// SizeOfHeapCommit
	.long	0				// LoaderFlags
	.long	0x10				// NumberOfRvaAndSizes

	.quad	0				// ExportTable
	.quad	0				// ImportTable
	.quad	0				// ResourceTable
	.quad	0				// ExceptionTable
	.quad	0				// CertificationTable
	.quad	0				// BaseRelocationTable
	.quad	0				// DebugTable
	.quad	0				// ArchTable
	.quad	0				// GlobalPointerTable
	.quad	0				// .tls
	.quad	0				// LoadConfigTable
	.quad	0				// BoundImportsTable
	.quad	0				// ImportAddressTable
	.quad	0				// DelayLoadImportTable
	.quad	0				// ClrRuntimeHeader (.cor)
	.quad	0				// Reserved

	// Section table
section_table:
	.ascii	".text"
	.byte	0
	.byte	0
	.byte	0			// end of 0 padding of section name

	.long	_edata - _start		// VirtualSize
	.long	_start - ImageBase	// VirtualAddress
	.long	_edata - _start		// SizeOfRawData
	.long	_start - ImageBase	// PointerToRawData
	.long	0		// PointerToRelocations (0 for executables)
	.long	0		// PointerToLineNumbers (0 for executables)
	.short	0		// NumberOfRelocations  (0 for executables)
	.short	0		// NumberOfLineNumbers  (0 for executables)
	.long	0x60500020	// Characteristics (section flags)

	/*
	 * The EFI application loader requires a relocation section
	 * because EFI applications must be relocatable.  This is a
	 * dummy section as far as we are concerned.
	 */
	.ascii	".reloc"
	.byte	0
	.byte	0			// end of 0 padding of section name

	.long	0			// VirtualSize
	.long	0			// VirtualAddress
	.long	0			// SizeOfRawData
	.long	0			// PointerToRawData
	.long	0			// PointerToRelocations
	.long	0			// PointerToLineNumbers
	.short	0			// NumberOfRelocations
	.short	0			// NumberOfLineNumbers
	.long	0x42100040		// Characteristics (section flags)

	/* x86-64 needs this padding here; without it, some machines simply
	 * refuse to admit this is an EFI binary.  I'm not really sure why;
	 * reading the spec, it's unclear, but you'd expect it would need to
	 * be aligned to (1 << FileAlignment), which would mean not having
	 * the spacing.
	 */
	.quad	0
_start:
	subq $8, %rsp
	pushq %rcx
	pushq %rdx

0:
	lea ImageBase(%rip), %rdi
	lea _DYNAMIC(%rip), %rsi

	popq %rcx
	popq %rdx
	pushq %rcx
	pushq %rdx
	call _relocate

	popq %rdi
	popq %rsi

	call efi_main
	addq $8, %rsp

.exit:	
  	ret
