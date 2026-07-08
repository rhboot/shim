// SPDX-License-Identifier: BSD-2-Clause-Patent

#pragma once

#include "shim.h"

extern void EFIAPI hexdumpf(const char *file, int line, const char *func,
                            const CHAR16 *const fmt, const void *data,
                            unsigned long size, size_t at, ...);
extern void hexdump(const char *file, int line, const char *func,
                    const void *data, unsigned long size);

extern void hexdumpat(const char *file, int line, const char *func,
                      const void *data, unsigned long size, size_t at);

#if defined(SHIM_UNIT_TEST)
#define LogHexDump(data, ...)
#define dhexdump(data, ...)
#define dhexdumpat(data, ...)
#define dhexdumpf(fmt, ...)
#else
#define LogHexdump(data, sz) LogHexdump_(__FILE__, __LINE__, __func__, data, sz)
#define dhexdump(data, sz)   hexdump(__FILE__, __LINE__, __func__, data, sz)
#define dhexdumpat(data, sz, at) \
	hexdumpat(__FILE__, __LINE__ - 1, __func__, data, sz, at)
#define dhexdumpf(fmt, data, sz, at, ...) \
	hexdumpf(__FILE__, __LINE__ - 1, __func__, fmt, data, sz, at, ##__VA_ARGS__)
#endif

// vim:fenc=utf-8:tw=75:noet
