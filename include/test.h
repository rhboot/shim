// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test.h - fake a bunch of EFI types so we can build test harnesses with libc
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifdef SHIM_UNIT_TEST
#ifndef TEST_H_
#define TEST_H_

#include <stdarg.h>

#if defined(__aarch64__)
#include <aa64/efibind.h>
#elif defined(__arm__)
#include <arm/efibind.h>
#elif defined(__i386__) || defined(__i486__) || defined(__i686__)
#include <ia32/efibind.h>
#elif defined(__x86_64__)
#include <x64/efibind.h>
#else
#error what arch is this
#endif

#include <efidef.h>

#include <efidevp.h>
#include <efiprot.h>
#include <eficon.h>
#include <efiapi.h>
#include <efierr.h>

#include <efipxebc.h>
#include <efinet.h>
#include <efiip.h>

#include <stdlib.h>

#define ZeroMem(buf, sz) memset(buf, 0, sz)
#define SetMem(buf, sz, value) memset(buf, value, sz)
#define CopyMem(dest, src, len) memcpy(dest, src, len)
#define CompareMem(dest, src, len) memcmp(dest, src, len)

#include <assert.h>

#define AllocateZeroPool(x) calloc(1, (x))
#define AllocatePool(x) malloc(x)
#define FreePool(x) free(x)
#define ReallocatePool(old, oldsz, newsz) realloc(old, newsz)

extern int debug;
#ifdef dprint
#undef dprint
#define dprint(fmt, ...) {( if (debug) printf("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); })
#endif

#define eassert(cond, fmt, ...)                                  \
	({                                                       \
		if (!(cond)) {                                   \
			printf("%s:%d:" fmt, __func__, __LINE__, \
			       ##__VA_ARGS__);                   \
		}                                                \
		assert(cond);                                    \
	})

#define assert_equal_return(a, b, status, fmt, ...)                        \
	({                                                                 \
		if (!((a) == (b))) {                                       \
			printf("%s:%d:" fmt, __func__, __LINE__, (a), (b), \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(a == b));             \
			return status;                                     \
		}                                                          \
	})

#define assert_return(cond, status, fmt, ...)                              \
	({                                                                 \
		if (!(cond)) {                                             \
			printf("%s:%d:" fmt, __func__, __LINE__,           \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(cond));               \
			return status;                                     \
		}                                                          \
	})

#define assert_goto(cond, label, fmt, ...)                                 \
	({                                                                 \
		if (!(cond)) {                                             \
			printf("%s:%d:" fmt, __func__, __LINE__,           \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(cond));               \
			goto label;                                        \
		}                                                          \
	})

#define assert_equal_goto(a, b, label, fmt, ...)                           \
	({                                                                 \
		if (!((a) == (b))) {                                       \
			printf("%s:%d:" fmt, __func__, __LINE__, (a), (b), \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(a == b));             \
			goto label;                                        \
		}                                                          \
	})

#define test(x, ...)                                    \
	({                                              \
		int rc;                                 \
		printf("running %s\n", __stringify(x)); \
		rc = x(__VA_ARGS__);                    \
		if (rc < 0)                             \
			status = 1;                     \
		printf("%s: %s\n", __stringify(x),      \
		       rc < 0 ? "failed" : "passed");   \
	})

#endif /* !TEST_H_ */
#endif /* SHIM_UNIT_TEST */
// vim:fenc=utf-8:tw=75:noet
