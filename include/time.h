// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * time.h - timekeeping things
 * Copyright Peter Jones <pjones@redhat.com>
 */

#pragma once

/*
 * If we're building unit tests, then all of the structs for time will come
 * from glibc, and all of these helpers are in libefivar.
 */
#ifndef SHIM_UNIT_TEST
#include "Cryptlib/Include/CrtLibTime.h"

extern int efi_time_to_tm(const EFI_TIME * const s, struct tm *d);
extern int tm_to_efi_time(const struct tm *const s, EFI_TIME *d, bool tzadj);
extern EFI_TIME *efi_gmtime_r(const time_t *time, EFI_TIME *result);
extern EFI_TIME *efi_gmtime(const time_t *time);
extern time_t efi_mktime(const EFI_TIME * const time);
#endif

extern void update_watchdog(void);

// vim:fenc=utf-8:tw=75:noet
