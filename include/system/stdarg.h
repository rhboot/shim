// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * stdarg.h - try to make consistent va_* handling for EFI
 */
#ifndef _STDARG_H

/*
 * clang doesn't know about __builtin_sysv_va_list, apparently.
 */
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wcpp"
typedef __builtin_va_list __builtin_sysv_va_list;
#warning clang builds may not work at all for anything other than scan-build
#pragma GCC diagnostic pop
#endif

#ifndef GNU_EFI_USE_EXTERNAL_STDARG
#define GNU_EFI_USE_EXTERNAL_STDARG
#endif

#ifdef SHIM_UNIT_TEST
#include_next <stdarg.h>
#endif

#if defined(__aarch64__) || defined(__arm__) || defined(__i386__) || \
	defined(__i486__) || defined(__i686__) || defined(__COVERITY__)

typedef __builtin_va_list ms_va_list;
typedef __builtin_va_list __builtin_ms_va_list;
#define ms_va_copy(dest, start)  __builtin_va_copy(dest, start)
#define ms_va_start(marker, arg) __builtin_va_start(marker, arg)
#define ms_va_arg(marker, type)  __builtin_va_arg(marker, type)
#define ms_va_end(marker)        __builtin_va_end(marker)

typedef __builtin_va_list sysv_va_list;
#define sysv_va_copy(dest, start)  __builtin_va_copy(dest, start)
#define sysv_va_start(marker, arg) __builtin_va_start(marker, arg)
#define sysv_va_arg(marker, type)  __builtin_va_arg(marker, type)
#define sysv_va_end(marker)        __builtin_va_end(marker)
/*
 * OpenSSL's X509ConstructCertificateStack needs this.
 */
typedef __builtin_va_list VA_LIST;
#define VA_COPY(dest, start)  __builtin_va_copy(dest, start)
#define VA_START(marker, arg) __builtin_va_start(marker, arg)
#define VA_END(marker)        __builtin_va_end(marker)
#define VA_ARG(marker, type)  __builtin_va_arg(marker, type)

#elif defined(__x86_64__)

typedef __builtin_ms_va_list ms_va_list;
#define ms_va_copy(dest, start)  __builtin_ms_va_copy(dest, start)
#define ms_va_start(marker, arg) __builtin_ms_va_start(marker, arg)
#define ms_va_arg(marker, type)  __builtin_va_arg(marker, type)
#define ms_va_end(marker)        __builtin_ms_va_end(marker)
typedef __builtin_sysv_va_list sysv_va_list;
#define sysv_va_copy(dest, start)  __builtin_sysv_va_copy(dest, start)
#define sysv_va_start(marker, arg) __builtin_sysv_va_start(marker, arg)
#define sysv_va_arg(marker, type)  __builtin_va_arg(marker, type)
#define sysv_va_end(marker)        __builtin_sysv_va_end(marker)
/*
 * OpenSSL's X509ConstructCertificateStack needs this.
 */
typedef __builtin_ms_va_list VA_LIST;
#define VA_COPY(dest, start)  __builtin_ms_va_copy(dest, start)
#define VA_START(marker, arg) __builtin_ms_va_start(marker, arg)
#define VA_END(marker)        __builtin_ms_va_end(marker)
#define VA_ARG(marker, type)  __builtin_va_arg(marker, type)

#else
#error what arch is this
#endif

#ifndef _STDARG_H
#define _STDARG_H
#endif /* !_STDARG_H #2 */

#endif /* !_STDARG_H */
// vim:fenc=utf-8:tw=75:noet
