// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * time.c - timekeeping things
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

int
efi_time_to_tm(const EFI_TIME * const s, struct tm *d)
{
	if (!s || !d) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * This seems wrong but it works with what's currently in
	 * TimerWrapper.c.  Specifically I think Year should subtract 1900
	 * and Month should subtract 1 here to match difference between the
	 * EFI spec and posix.
	 */
	d->tm_year = s->Year;
	d->tm_mon = s->Month;
	d->tm_mday = s->Day;
	d->tm_hour = s->Hour;
	d->tm_min = s->Minute;
	/*
           * Just ignore EFI's range problem here and pretend we're in UTC
           * not UT1.
           */
	d->tm_sec = s->Second;
	d->tm_isdst = (s->Daylight & EFI_TIME_IN_DAYLIGHT) ? 1 : 0;

	return 0;
}

int
tm_to_efi_time(const struct tm *const s, EFI_TIME *d, bool tzadj)
{
	if (!s || !d) {
		errno = EINVAL;
		return -1;
	}

	d->Pad2 = 0;
	d->Daylight = s->tm_isdst ? EFI_TIME_IN_DAYLIGHT : 0;
	d->TimeZone = 0;
	d->Nanosecond = 0;
	d->Pad1 = 0;
	d->Second = s->tm_sec < 60 ? s->tm_sec : 59;
	d->Minute = s->tm_min;
	d->Hour = s->tm_hour;
	d->Day = s->tm_mday;
	/*
	 * This seems wrong but it works with what's currently in
	 * TimerWrapper.c.  Specifically I think Year should add 1900
	 * and Month should add 1 here to match difference between the
	 * EFI spec and posix.
	 */
	d->Month = s->tm_mon;
	d->Year = s->tm_year;

	if (tzadj) {
		d->TimeZone = timezone / 60;
	}

	return 0;
}

EFI_TIME *
efi_gmtime_r(const time_t *time, EFI_TIME *result)
{
	struct tm tm = { 0, };

	if (!time || !result) {
		errno = EINVAL;
		return NULL;
	}

	gmtime_r(time, &tm);
	tm_to_efi_time(&tm, result, false);

	return result;
}

EFI_TIME *
efi_gmtime(const time_t *time)
{
	static EFI_TIME ret;

	if (!time) {
		errno = EINVAL;
		return NULL;
	}

	efi_gmtime_r(time, &ret);

	return &ret;
}

time_t
efi_mktime(const EFI_TIME * const time)
{
	struct tm tm = { 0, };
	time_t ret;

	if (!time) {
		errno = EINVAL;
		return (time_t)-1;
	}

	efi_time_to_tm(time, &tm);
	ret = mktime(&tm);

	return ret;
}

#define TIMEOUT_SLOP	60
#define TIMEOUT		(TIMEOUT_SLOP * 5)

void
update_watchdog(void)
{
	EFI_STATUS efi_status;
	EFI_TIME efi_time = { 0, };
	EFI_TIME_CAPABILITIES efi_time_caps = { 0, };
	time_t this_time = 0;
	static time_t last_time = 0;

	efi_status = RT->GetTime(&efi_time, &efi_time_caps);
	if (EFI_ERROR(efi_status)) {
		BS->SetWatchdogTimer(TIMEOUT, 0x31337, 0, NULL);
		return;
	}

	this_time = efi_mktime(&efi_time);
	if (this_time < last_time ||
	    this_time < TIMEOUT_SLOP ||
	    this_time - TIMEOUT_SLOP > last_time) {
		if (verbose) {
			log_debug_print(L"updating watchdog at %llu from %llu to %llu\n",
					this_time, last_time+TIMEOUT, this_time+TIMEOUT);
			console_print(L"updating watchdog at %llu from %llu to %llu\n",
				      this_time, last_time+TIMEOUT, this_time+TIMEOUT);
		}
		last_time = this_time;
		BS->SetWatchdogTimer(TIMEOUT, 0x31337, 0, NULL);
	}
}

// vim:fenc=utf-8:tw=75:noet
