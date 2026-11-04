// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * shim-unit-test.h - setup that always needs done on unit tests
 * Copyright Peter Jones <pjones@redhat.com>
 */

#pragma once

#ifdef SHIM_UNIT_TEST

# define STATIC

#else /* !SHIM_UNIT_TEST */

# define STATIC static

#endif /* SHIM_UNIT_TEST */

// vim:fenc=utf-8:tw=75:noet
