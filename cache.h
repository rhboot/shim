// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef CACHE_H_
#define CACHE_H_

#if defined(__GNUC__)
#define cache_invalidate(begin, end)  __builtin___clear_cache(begin, end)
#else /* __GNUC__ */
#error shim has no cache_invalidate() implementation for this compiler
#endif /* __GNUC__ */

#endif /* CACHE_H_ */
