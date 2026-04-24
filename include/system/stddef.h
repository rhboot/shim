// SPDX-License-Identifier: BSD-2-Clause-Patent
#pragma once

#include_next <stddef.h>

#if __SIZE_WIDTH__ == 64
typedef __INT64_TYPE__ ssize_t;
#elif __SIZE_WIDTH__ == 32
typedef __INT32_TYPE__ ssize_t;
#elif __SIZE_WIDTH__ == 16
typedef __INT16_TYPE__ ssize_t;
#else
#error unsupported architecture
#endif

// vim:fenc=utf-8:tw=75:noet
