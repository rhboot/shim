// SPDX-License-Identifier: BSD-2-Clause-Patent
/**
 * macros to build builtin wrappers
 */
#ifndef mkbi_cat_
#define mkbi_cat_(a, b) a##b
#endif
#ifdef SHIM_STRING_C_

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

#else /* ! SHIM_STRING_C_ */

#ifndef mkbi1_
#define mkbi1_(rtype, x, typea, a)			\
	static inline __attribute__((__unused__))	\
	rtype						\
        x(typea a)					\
	{						\
		return mkbi_cat_(__builtin_, x)(a);	\
	}
#endif

#ifndef mkbi2_
#define mkbi2_(rtype, x, typea, a, typeb, b)            \
	static inline __attribute__((__unused__))	\
	rtype						\
        x(typea a, typeb b)                             \
	{                                               \
		return mkbi_cat_(__builtin_, x)(a, b);	\
	}
#endif

#ifndef mkbi3_
#define mkbi3_(rtype, x, typea, a, typeb, b, typec, c)		\
	static inline __attribute__((__unused__))		\
	rtype							\
        x(typea a, typeb b, typec c)				\
	{							\
		return mkbi_cat_(__builtin_, x)(a, b,c);	\
	}
#endif

#ifdef SHIM_DEPRECATE_STRLEN
#ifndef mkdepbi_dep_
#define mkdepbi_dep_ __attribute__((__deprecated__))
#endif
#else /* !SHIM_DEPRECATE_STRLEN */
#ifndef mkdepbi_dep_
#define mkdepbi_dep_
#endif
#endif /* SHIM_DEPRECATE_STRLEN */

#ifndef mkdepbi1_
#define mkdepbi1_(rtype, x, typea, a)			\
	static inline __attribute__((__unused__))	\
	mkdepbi_dep_					\
	rtype						\
        x(typea a)					\
	{						\
		return mkbi_cat_(__builtin_, x)(a);	\
	}
#endif

#ifndef mkdepbi2_
#define mkdepbi2_(rtype, x, typea, a, typeb, b)         \
	static inline __attribute__((__unused__))	\
	mkdepbi_dep_					\
	rtype						\
        x(typea a, typeb b)                             \
	{                                               \
		return mkbi_cat_(__builtin_, x)(a, b);	\
	}
#endif
#endif /* SHIM_STRING_C_ */

// vim:fenc=utf-8:tw=75:noet
