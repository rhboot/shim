// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * stdarg.h - try to make consistent va_* handling for EFI
 */
#ifdef SHIM_UNIT_TEST
#include_next <stdarg.h>
#else
#ifndef _STDARG_H
#define _STDARG_H

#include <efistdarg.h>

#endif /* !_STDARG_H */
#endif
#ifndef SHIM_STDARG_H_
#define SHIM_STDARG_H_

typedef __builtin_ms_va_list ms_va_list;
#define ms_va_copy(dest, start)  __builtin_ms_va_copy(dest, start)
#define ms_va_start(marker, arg) __builtin_ms_va_start(marker, arg)
#define ms_va_arg(marker, type)  __builtin_va_arg(marker, type)
#define ms_va_end(marker)        __builtin_ms_va_end(marker)

typedef __builtin_va_list elf_va_list;
#define elf_va_copy(dest, start)  __builtin_va_copy(dest, start)
#define elf_va_start(marker, arg) __builtin_va_start(marker, arg)
#define elf_va_arg(marker, type)  __builtin_va_arg(marker, type)
#define elf_va_end(marker)        __builtin_va_end(marker)

#endif /* !SHIM_STDARG_H_ */
// vim:fenc=utf-8:tw=75:noet
