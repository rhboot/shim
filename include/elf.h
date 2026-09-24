// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * elf.h - definitions of elf structures we need
 * Copyright Peter Jones <pjones@redhat.com>
 */

#pragma once

#include <stdint.h>

typedef uint16_t elf32_half;
typedef int16_t elf64_half;

typedef uint32_t elf32_word;
typedef int32_t elf32_sword;
typedef uint32_t elf64_word;
typedef int32_t elf64_sword;

typedef uint64_t elf32_xword;
typedef int64_t elf32_sxword;
typedef uint64_t elf64_xword;
typedef int64_t elf64_sxword;

typedef uint32_t elf32_addr;
typedef uint64_t elf64_addr;

/*
 * Legal values for d_tag (dynamic entry type).
 */
#define DT_NULL			0	/* Marks end of dynamic section */
#define DT_NEEDED		1	/* Name of needed library */
#define DT_PLTRELSZ		2	/* Size in bytes of PLT relocs */
#define DT_PLTGOT		3	/* Processor defined value */
#define DT_HASH			4	/* Address of symbol hash table */
#define DT_STRTAB		5	/* Address of string table */
#define DT_SYMTAB		6	/* Address of symbol table */
#define DT_RELA			7	/* Address of Rela relocs */
#define DT_RELASZ		8	/* Total size of Rela relocs */
#define DT_RELAENT		9	/* Size of one Rela reloc */
#define DT_STRSZ		10	/* Size of string table */
#define DT_SYMENT		11	/* Size of one symbol table entry */
#define DT_INIT			12	/* Address of init function */
#define DT_FINI			13	/* Address of termination function */
#define DT_SONAME		14	/* Name of shared object */
#define DT_RPATH		15	/* Library search path (deprecated) */
#define DT_SYMBOLIC		16	/* Start symbol search here */
#define DT_REL			17	/* Address of Rel relocs */
#define DT_RELSZ		18	/* Total size of Rel relocs */
#define DT_RELENT		19	/* Size of one Rel reloc */
#define DT_PLTREL		20	/* Type of reloc in PLT */
#define DT_DEBUG		21	/* For debugging; unspecified */
#define DT_TEXTREL		22	/* Reloc might modify .text */
#define DT_JMPREL		23	/* Address of PLT relocs */
#define DT_BIND_NOW		24	/* Process relocations of object */
#define DT_INIT_ARRAY		25	/* Array with addresses of init fct */
#define DT_FINI_ARRAY		26	/* Array with addresses of fini fct */
#define DT_INIT_ARRAYSZ		27	/* Size in bytes of DT_INIT_ARRAY */
#define DT_FINI_ARRAYSZ		28	/* Size in bytes of DT_FINI_ARRAY */
#define DT_RUNPATH		29	/* Library search path */
#define DT_FLAGS		30	/* Flags for the object being loaded */
#define DT_ENCODING		32	/* Start of encoded range */
#define DT_PREINIT_ARRAY	32	/* Array with addresses of preinit fct */
#define DT_PREINIT_ARRAYSZ	33	/* size in bytes of DT_PREINIT_ARRAY */
#define DT_SYMTAB_SHNDX		34	/* Address of SYMTAB_SHNDX section */
#define DT_NUM			35	/* Number used */

#include "elf/aa64.h"		// IWYU pragma: export
#include "elf/arm.h"		// IWYU pragma: export
#include "elf/ia32.h"		// IWYU pragma: export
#include "elf/riscv64.h"	// IWYU pragma: export
#include "elf/x64.h"		// IWYU pragma: export

typedef struct elf_dynamic {
	elf_sword d_tag;	/* dynamic entry type */
	union {
		elf_word d_val;	/* integer value */
		elf_addr d_ptr;	/* address value */
	};
} elf_dynamic;

struct elf32_rel {
	elf_addr r_offset;	/* address */
	elf_word r_info;	/* type and symbol index */
};

struct elf64_rel {
	elf_addr r_offset;	/* address */
	elf_xword r_info;	/* type and symbol index */
};

struct elf32_rela {
	elf_addr r_offset;	/* address */
	elf_word r_info;	/* type and symbol index */
	elf_sword r_addend;	/* addend */
};

#define ELF32_R_SYM(val)	((val) >> 8)
#define ELF32_R_TYPE(val)	((val) & 0xff)
#define ELF32_R_INFO(sym, type)	(((sym) << 8) + ((type) & 0xff))

struct elf64_rela {
	elf_addr r_offset;	/* address */
	elf_xword r_info;	/* type and symbol index */
	elf_sxword r_addend;	/* addend */
};

#define ELF64_R_SYM(val)	((val) >> 32)
#define ELF64_R_TYPE(val)	((val) & 0xffffffff)
#define ELF64_R_INFO(sym, type)	((((elf64_xword) (sym)) << 32) + (type))

struct elf32_sym {
	elf_word st_name;	/* symbol name, index in string table */
	elf_addr st_value;	/* symbol value (address) */
	elf_word st_size;	/* symbol size */
	uint8_t st_info;	/* symbol binding and type */
	uint8_t st_other;	/* visibility and other data */
	elf_half st_shndx;	/* symbol section index */
};

#define ELF32_ST_BIND(i)	((i)>>4)
#define ELF32_ST_TYPE(i)	((i)&0xf)
#define ELF32_ST_INFO(b,t)	(((b)<<4)+((t)&0xf))

struct elf64_sym {
	elf_word st_name;	/* symbol name, index in string table */
	uint8_t st_info;	/* symbol binding and type */
	uint8_t st_other;	/* visibility and other data */
	elf_half st_shndx;	/* symbol section index */
	elf_addr st_value;	/* symbol value (address) */
	elf_xword st_size;	/* symbol size */
};

#define ELF64_ST_BIND(i)	((i)>>4)
#define ELF64_ST_TYPE(i)	((i)&0xf)
#define ELF64_ST_INFO(b,t)	(((b)<<4)+((t)&0xf))

#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_LOPROC	13
#define STB_HIPROC	15

#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_LOPROC	13
#define STT_HIPROC	15

// vim:fenc=utf-8:tw=75:noet
