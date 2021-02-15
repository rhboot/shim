// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef COMPILER_H_
#define COMPILER_H_

#ifndef UNUSED
#define UNUSED __attribute__((__unused__))
#endif
#ifndef HIDDEN
#define HIDDEN __attribute__((__visibility__ ("hidden")))
#endif
#ifndef PUBLIC
#define PUBLIC __attribute__((__visibility__ ("default")))
#endif
#ifndef DESTRUCTOR
#define DESTRUCTOR __attribute__((destructor))
#endif
#ifndef CONSTRUCTOR
#define CONSTRUCTOR __attribute__((constructor))
#endif
#ifndef ALIAS
#define ALIAS(x) __attribute__((weak, alias (#x)))
#endif
#ifndef NONNULL
#endif
#define NONNULL(first, args...) __attribute__((__nonnull__(first, ## args)))
#ifndef PRINTF
#define PRINTF(first, args...) __attribute__((__format__(printf, first, ## args)))
#endif
#ifndef FLATTEN
#define FLATTEN __attribute__((__flatten__))
#endif
#ifndef PACKED
#define PACKED __attribute__((__packed__))
#endif
#ifndef VERSION
#define VERSION(sym, ver) __asm__(".symver " # sym "," # ver)
#endif
#ifndef NORETURN
#define NORETURN __attribute__((__noreturn__))
#endif
#ifndef ALIGNED
#define ALIGNED(n) __attribute__((__aligned__(n)))
#endif
#ifndef CLEANUP_FUNC
#define CLEANUP_FUNC(x) __attribute__((__cleanup__(x)))
#endif
#ifndef USED
#define USED __attribute__((__used__))
#endif
#ifndef SECTION
#define SECTION(x) __attribute__((__section__(x)))
#endif
#ifndef OPTIMIZE
#define OPTIMIZE(x) __attribute__((__optimize__(x)))
#endif

#ifndef __CONCAT
#define __CONCAT3(a, b, c) a ## b ## c
#endif
#ifndef CAT
#define CAT(a, b) __CONCAT(a, b)
#endif
#ifndef CAT3
#define CAT3(a, b, c) __CONCAT3(a, b, c)
#endif
#ifndef STRING
#define STRING(x) __STRING(x)
#endif

#ifndef WRITE_ONCE
#define WRITE_ONCE(var, val) \
        (*((volatile typeof(val) *)(&(var))) = (val))
#endif

#ifndef READ_ONCE
#define READ_ONCE(var) (*((volatile typeof(var) *)(&(var))))
#endif

#ifndef likely
#define likely(x)	__builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x)	__builtin_expect(!!(x), 0)
#endif

/* Are two types/vars the same type (ignoring qualifiers)? */
#ifndef __same_type
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#endif

/* Compile time object size, -1 for unknown */
#ifndef __compiletime_object_size
# define __compiletime_object_size(obj) -1
#endif
#ifndef __compiletime_warning
# define __compiletime_warning(message)
#endif
#ifndef __compiletime_error
# define __compiletime_error(message)
#endif

#ifndef __compiletime_assert
#define __compiletime_assert(condition, msg, prefix, suffix)		\
	do {								\
		extern void prefix ## suffix(void) __compiletime_error(msg); \
		if (!(condition))					\
			prefix ## suffix();				\
	} while (0)
#endif

#ifndef _compiletime_assert
#define _compiletime_assert(condition, msg, prefix, suffix) \
	__compiletime_assert(condition, msg, prefix, suffix)
#endif

/**
 * compiletime_assert - break build and emit msg if condition is false
 * @condition: a compile-time constant condition to check
 * @msg:       a message to emit if condition is false
 *
 * In tradition of POSIX assert, this macro will break the build if the
 * supplied condition is *false*, emitting the supplied error message if the
 * compiler has support to do so.
 */
#ifndef compiletime_assert
#define compiletime_assert(condition, msg) \
	_compiletime_assert(condition, msg, __compiletime_assert_, __LINE__ - 1)
#endif

/**
 * BUILD_BUG_ON_MSG - break compile if a condition is true & emit supplied
 *		      error message.
 * @condition: the condition which the compiler should know is false.
 *
 * See BUILD_BUG_ON for description.
 */
#ifndef BUILD_BUG_ON_MSG
#define BUILD_BUG_ON_MSG(cond, msg) compiletime_assert(!(cond), msg)
#endif

#ifndef ALIGN
#define __ALIGN_MASK(x, mask)   (((x) + (mask)) & ~(mask))
#define __ALIGN(x, a)           __ALIGN_MASK(x, (typeof(x))(a) - 1)
#define ALIGN(x, a)             __ALIGN((x), (a))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a)        __ALIGN((x) - ((a) - 1), (a))
#endif

#define MIN(a, b) ({(a) < (b) ? (a) : (b);})
#define MAX(a, b) ({(a) <= (b) ? (b) : (a);})

#endif /* !COMPILER_H_ */
// vim:fenc=utf-8:tw=75:et
