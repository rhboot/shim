// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * backtrace.h - sigh, backtrace.
 * Copyright Peter Jones <pjones@redhat.com>
 */

#pragma once
#ifndef SHIM_UNIT_TEST

extern void backtrace(unsigned int skip);

#endif /* SHIM_UNIT_TEST */
// vim:fenc=utf-8:tw=75:noet
