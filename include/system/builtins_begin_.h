// SPDX-License-Identifier: BSD-2-Clause-Patent
/**
 * macros to build function declarations with the same types as builtins
 * that we apparently really cannot depend on.
 */

/*
 * Clang's __builtin_whatever and __typeof__ are broken thusly:
 * In file included from MokManager.c:2:
 * In file included from shim.h:47:
 * include/system/string.h:29:1: error: builtin functions must be directly called
 * mkbi1_(long int, ffsl, long int, x)
 * ^
 */
#if defined(__clang__)

#ifndef mkbi1_
#define mkbi1_(rtype, x, typea, a) rtype x(typea a);
#endif

#ifndef mkbi2_
#define mkbi2_(rtype, x, typea, a, typeb, b) rtype x(typea a, typeb b);
#endif

#ifndef mkbi3_
#define mkbi3_(rtype, x, typea, a, typeb, b, typec, c) rtype x(typea a, typeb b, typec c);
#endif

#ifndef mkdepbi1_
#define mkdepbi1_(rtype, x, typea, a) rtype x(typea a);
#endif

#ifndef mkdepbi2_
#define mkdepbi2_(rtype, x, typea, a, typeb, b) rtype x(typea a, typeb b);
#endif

#else /* !__clang__ */

#ifndef mkbi_cat_
#define mkbi_cat_(a, b) a##b
#endif

#ifndef mkbi1_
#define mkbi1_(rtype, x, typea, a) __typeof__(mkbi_cat_(__builtin_, x)) x;
#endif

#ifndef mkbi2_
#define mkbi2_(rtype, x, typea, a, typeb, b) __typeof__(mkbi_cat_(__builtin_, x)) x;
#endif

#ifndef mkbi3_
#define mkbi3_(rtype, x, typea, a, typeb, b, typec, c) __typeof__(mkbi_cat_(__builtin_, x)) x;
#endif

#ifndef mkdepbi1_
#define mkdepbi1_(rtype, x, typea, a) __typeof__(mkbi_cat_(__builtin_, x)) x;
#endif

#ifndef mkdepbi2_
#define mkdepbi2_(rtype, x, typea, a, typeb, b) __typeof__(mkbi_cat_(__builtin_, x)) x;
#endif

#endif /* !__clang__ */

// vim:fenc=utf-8:tw=75:noet
