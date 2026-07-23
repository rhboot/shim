// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * backtrace.c - sigh, backtrace.
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

#ifndef __arm__
#include <elf.h>

#define MAX_STACK_FRAME 102400

extern const void ImageBase;
extern const void _text;
extern const void _data;
extern const void _dynstr, _dynstr_end;
extern const void _dynsym, _dynsym_end;

static bool
elf_sym_matches(uintptr_t needle, elf_sym *hay)
{
	uintptr_t vma = &_text - &ImageBase;
	uintptr_t start = hay->st_value;
	uintptr_t end = hay->st_value + hay->st_size;
	bool found = false;

	if (needle == 0)
		return false;

	needle -= (uintptr_t)&_text - vma;

	if (hay &&
	    hay->st_shndx == 3 &&
	    start <= needle &&
	    end >= needle)
		found = true;

#if defined(DEBUG_BACKTRACE)
	dprint(L"%p %a between %p and %p\n", needle, found ? "is" : "is not", start, end);
#endif
	return found;
}

static int
get_name(elf_word st_name, char **name)
{
	uint64_t limit = &_dynstr_end - &_dynstr;
	char *dynstr = (char *)&_dynstr;

	if (st_name > limit)
		return -1;

	*name = &dynstr[st_name];
	return 0;
}

#if defined(DEBUG_BACKTRACE)
static char
elf_bind_decode(uint8_t st_info)
{
	switch (ELF_ST_BIND(st_info)) {
	case STB_LOCAL:
		return 'l';
	case STB_GLOBAL:
		return 'g';
	case STB_WEAK:
		return 'w';
	default:
		break;
	}
	return '?';
}

static char
elf_type_decode(uint8_t st_info)
{
	switch (ELF_ST_TYPE(st_info)) {
	case STT_NOTYPE:
		return ' ';
	case STT_OBJECT:
		return 'O';
	case STT_FUNC:
		return 'F';
	case STT_SECTION:
		return 'S';
	case STT_FILE:
		return 'f';
	default:
		break;
	}
	return '?';
}
#endif /* DEBUG_BACKTRACE */

static int
get_symbol(uintptr_t func, char **symbol, uintptr_t *symaddr)
{
	elf_sym *sym = NULL;

#if defined(DEBUG_BACKTRACE)
	static bool once = false;

	if (!once) {
		dprint(L"_dynsym:%p _dynsym_end:%p _dynstr:%p _dynstr_end:%p\n",
		       &_dynsym, &_dynsym_end, &_dynstr, &_dynstr_end);
		once = true;
	}
	dprint(L"Looking for 0x%p\n", func);
#endif
	for (elf_sym *hay = ((elf_sym *)&_dynsym)+1; (uintptr_t)hay < (uintptr_t)&_dynsym_end; hay++) {
		if (hay->st_shndx != 3)
			continue;
#if defined(DEBUG_BACKTRACE)
		dprint(L"entry %p %c %c %lu 0x%lx %lu\n",
		       (void *)(hay->st_value), elf_bind_decode(hay->st_info),
		       elf_type_decode(hay->st_info), hay->st_shndx, hay->st_size, hay->st_name);
#endif
		if (elf_sym_matches(func, hay)) {
#if defined(DEBUG_BACKTRACE)
			dprint(L"found! %p %c %c %lu 0x%lx %lu\n",
			       (void *)(hay->st_value), elf_bind_decode(hay->st_info),
			       elf_type_decode(hay->st_info), hay->st_shndx, hay->st_size, hay->st_name);
#endif
			sym = hay;
			break;
		}
	}
	if (!sym)
		return -1;

	*symaddr = (uintptr_t)&ImageBase + sym->st_value;
#if defined(DEBUG_BACKTRACE)
	dprint(L"_dynsym:%p ImageBase:%p %p+0x%lx:%p\n",
	       &_dynsym, &ImageBase, &ImageBase, sym->st_value, *symaddr);
#endif

	get_name(sym->st_name, symbol);

	return 0;
}

void
backtrace(unsigned int skip)
{
	bool done = false;
	int rc;

#if defined(DEBUG_BACKTRACE)
	dprint(L"ImageBase:%p\n", &ImageBase);
	dprint(L"Backtrace .text:%p .data:%p\n", &_text, &_data);
	dprint(L"_text - ImageBase: %p\n", (void *)((uintptr_t)&_text - (uintptr_t)&ImageBase));
	dprint(L"backtrace function is at %p\n", backtrace);
#endif

	do {
		void *func = NULL;
		char *sym = NULL;
		uintptr_t addr = 0;

		switch(skip) {
		case 0:
			func = __builtin_return_address(0);
			break;
		/*
		 * Clang takes the "some platforms can't deal with nonzero
		 * arguments" thing "seriously" by just not ever
		 * implementing it:
		 *
		 * backtrace.c:166:11: error: calling '__builtin_return_address' with a nonzero argument is unsafe [-Werror,-Wframe-address]
		 * 166 |                         func = __builtin_return_address(1);
		 *     |                                ^~~~~~~~~~~~~~~~~~~~~~~~~~~
		 */
#ifndef __clang__
		case 1:
			func = __builtin_return_address(1);
			break;
		case 2:
			func = __builtin_return_address(2);
			break;
		case 3:
			func = __builtin_return_address(3);
			break;
		case 4:
			func = __builtin_return_address(4);
			break;
		case 5:
			func = __builtin_return_address(5);
			break;
		case 6:
			func = __builtin_return_address(6);
			break;
		case 7:
			func = __builtin_return_address(7);
			break;
		case 8:
			func = __builtin_return_address(8);
			break;
		case 9:
			func = __builtin_return_address(9);
			break;
		case 10:
			func = __builtin_return_address(10);
			break;
		case 11:
			func = __builtin_return_address(11);
			break;
		case 12:
			func = __builtin_return_address(12);
			break;
		case 13:
			func = __builtin_return_address(13);
			break;
		case 14:
			func = __builtin_return_address(14);
			break;
		case 15:
			func = __builtin_return_address(15);
			break;
		case 16:
			func = __builtin_return_address(16);
			break;
		case 17:
			func = __builtin_return_address(17);
			break;
		case 18:
			func = __builtin_return_address(18);
			break;
		case 19:
			func = __builtin_return_address(19);
			break;
		case 20:
			func = __builtin_return_address(20);
			break;
		case 21:
			func = __builtin_return_address(21);
			break;
		case 22:
			func = __builtin_return_address(22);
			break;
		case 23:
			func = __builtin_return_address(23);
			break;
		case 24:
			func = __builtin_return_address(24);
			break;
		case 25:
			func = __builtin_return_address(25);
			break;
		case 26:
			func = __builtin_return_address(26);
			break;
		case 27:
			func = __builtin_return_address(27);
			break;
		case 28:
			func = __builtin_return_address(28);
			break;
		case 29:
			func = __builtin_return_address(29);
			break;
		case 30:
			func = __builtin_return_address(30);
#endif /* __clang__ */
			done = true;
			break;
		}
		if (func == NULL) {
#if defined(DEBUG_BACKTRACE)
			dprint(L"__builtin_return_address(%u) == NULL\n", skip);
#endif
			break;
		}
		func = __builtin_extract_return_addr(func);
		if (func == NULL) {
#if defined(DEBUG_BACKTRACE)
			dprint(L"__builtin_extract_return_addr(__builtin_return_address(%u)) == NULL\n", skip);
#endif
			break;
		}
		rc = get_symbol((uintptr_t)func, &sym, &addr);
		if (rc >= 0 && sym) {
			dprint(L"%p (%a+0x%lx)\n", func, sym, (uintptr_t)func - addr);
		} else {
			dprint(L"%p\n", func);
		}
		skip += 1;
	} while (!done);
}
#else /* __arm__ */
void
backtrace(unsigned int skip UNUSED)
{

}
#endif

// vim:fenc=utf-8:tw=75:noet
