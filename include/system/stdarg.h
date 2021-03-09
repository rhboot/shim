// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * stdarg.h - try to make consistent va_* handling for EFI
 */
#ifdef SHIM_UNIT_TEST
#include_next <stdarg.h>
#else
#ifndef _STDARG_H
#define _STDARG_H

#ifndef GNU_EFI_USE_EXTERNAL_STDARG
#define GNU_EFI_USE_EXTERNAL_STDARG
#endif

#if defined(__aarch64__) || defined(__arm__) || defined(__i386__) || \
	defined(__i486__) || defined(__i686__) || defined(SHIM_UNIT_TEST)
typedef __builtin_va_list va_list;
#define va_copy(dest, start)  __builtin_va_copy(dest, start)
#define va_start(marker, arg) __builtin_va_start(marker, arg)
#define va_arg(marker, type)  __builtin_va_arg(marker, type)
#define va_end(marker)        __builtin_va_end(marker)
#elif defined(__x86_64__)
typedef __builtin_ms_va_list va_list;
#define va_copy(dest, start)  __builtin_ms_va_copy(dest, start)
#define va_start(marker, arg) __builtin_ms_va_start(marker, arg)
#define va_arg(marker, type)  __builtin_va_arg(marker, type)
#define va_end(marker)        __builtin_ms_va_end(marker)
#else
#error what arch is this
#endif

typedef va_list VA_LIST;
#define VA_COPY(dest, start)  va_copy(dest, start)
#define VA_START(marker, arg) va_start(marker, arg)
#define VA_END(marker)        va_end(marker)
#define VA_ARG(marker, type)  va_arg(marker, type)

#endif /* !_STDARG_H */
#endif /* !SHIM_UNIT_TEST */
// vim:fenc=utf-8:tw=75:noet
