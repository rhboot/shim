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
#include <aarch64/efibind.h>
#elif defined(__arm__)
#include <arm/efibind.h>
#elif defined(__i386__) || defined(__i486__) || defined(__i686__)
#include <ia32/efibind.h>
#elif defined(__x86_64__)
#include <x86_64/efibind.h>
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

#define assert_true_as_expr(a, status, fmt, ...)                           \
	({                                                                 \
		int rc_ = 0;                                               \
		if (!(a)) {                                                \
			printf("%s:%d:got %lld, expected nonzero " fmt,    \
			       __func__, __LINE__, (long long)(a),         \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(!(a)));               \
			rc_ = status;                                      \
		}                                                          \
		rc_;                                                       \
	})
#define assert_nonzero_as_expr(a, ...) assert_true_as_expr(a, ##__VA_ARGS__)

#define assert_false_as_expr(a, status, fmt, ...)                              \
	({                                                                     \
		int rc_ = 0;                                                   \
		if (a) {                                                       \
			printf("%s:%d:got %lld, expected zero " fmt, __func__, \
			       __LINE__, (long long)(a), ##__VA_ARGS__);       \
			printf("%s:%d:Assertion `%s' failed.\n", __func__,     \
			       __LINE__, __stringify(a));                      \
			rc_ = status;                                          \
		}                                                              \
		rc_;                                                           \
	})
#define assert_zero_as_expr(a, ...) assert_false_as_expr(a, ##__VA_ARGS__)

#define assert_positive_as_expr(a, status, fmt, ...)                          \
	({                                                                    \
		int rc_ = 0;                                                  \
		if ((a) <= 0) {                                               \
			printf("%s:%d:got %lld, expected > 0 " fmt, __func__, \
			       __LINE__, (long long)(a), ##__VA_ARGS__);      \
			printf("%s:%d:Assertion `%s' failed.\n", __func__,    \
			       __LINE__, __stringify((a) <= 0));              \
			rc_ = status;                                         \
		}                                                             \
		rc_;                                                          \
	})

#define assert_negative_as_expr(a, status, fmt, ...)                          \
	({                                                                    \
		int rc_ = 0;                                                  \
		if ((a) >= 0) {                                               \
			printf("%s:%d:got %lld, expected < 0 " fmt, __func__, \
			       __LINE__, (long long)(a), ##__VA_ARGS__);      \
			printf("%s:%d:Assertion `%s' failed.\n", __func__,    \
			       __LINE__, __stringify((a) >= 0));              \
			rc_ = status;                                         \
		}                                                             \
		rc_;                                                          \
	})

#define assert_equal_as_expr(a, b, status, fmt, ...)                       \
	({                                                                 \
		int rc_ = 0;                                               \
		if (!((a) == (b))) {                                       \
			printf("%s:%d:" fmt, __func__, __LINE__, (a), (b), \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(a == b));             \
			rc_ = status;                                      \
		}                                                          \
		rc_;                                                       \
	})

#define assert_as_expr(cond, status, fmt, ...)                             \
	({                                                                 \
		int rc_ = 0;                                               \
		if (!(cond)) {                                             \
			printf("%s:%d:" fmt, __func__, __LINE__,           \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(cond));               \
			rc_ = status;                                      \
		}                                                          \
		rc_;                                                       \
	})

#define assert_true_return(a, status, fmt, ...)                               \
	({                                                                    \
		int rc_ = assert_true_as_expr(a, status, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                                 \
			return rc_;                                           \
	})
#define assert_nonzero_return(a, ...) assert_true_return(a, ##__VA_ARGS__)

#define assert_false_return(a, status, fmt, ...)                               \
	({                                                                     \
		int rc_ = assert_false_as_expr(a, status, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                                  \
			return rc_;                                            \
	})
#define assert_zero_return(a, ...) assert_false_return(a, ##__VA_ARGS__)

#define assert_positive_return(a, status, fmt, ...)               \
	({                                                        \
		int rc_ = assert_positive_as_expr(a, status, fmt, \
		                                  ##__VA_ARGS__); \
		if (rc_ != 0)                                     \
			return rc_;                               \
	})

#define assert_negative_return(a, status, fmt, ...)               \
	({                                                        \
		int rc_ = assert_negative_as_expr(a, status, fmt, \
		                                  ##__VA_ARGS__); \
		if (rc_ != 0)                                     \
			return rc_;                               \
	})

#define assert_equal_return(a, b, status, fmt, ...)               \
	({                                                        \
		int rc_ = assert_equal_as_expr(a, b, status, fmt, \
		                               ##__VA_ARGS__);    \
		if (rc_ != 0)                                     \
			return rc_;                               \
	})

#define assert_return(cond, status, fmt, ...)                               \
	({                                                                  \
		int rc_ = assert_as_expr(cond, status, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                               \
			return rc_;                                         \
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

#define assert_negative_goto(a, label, fmt, ...)                              \
	({                                                                    \
		int rc_ = assert_negative_as_expr(a, -1, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                                 \
			goto label;                                           \
	})

#define assert_positive_goto(a, label, fmt, ...)                              \
	({                                                                    \
		int rc_ = assert_positive_as_expr(a, -1, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                                 \
			goto label;                                           \
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
