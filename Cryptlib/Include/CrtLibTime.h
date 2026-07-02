// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * CrtLibTime.h - stdlib time structure and type definitions.
 *
 * Copyright (c) 2010 - 2022, Intel Corporation. All rights reserved.
 * Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.
 * Copyright (c) 2022, Loongson Technology Corporation Limited. All rights reserved.
 */

#pragma once

//
// Basic types mapping
//
typedef INT64   time_t;

//
// Structures Definitions
//
struct tm {
  int     tm_sec;    /* seconds after the minute [0-60] */
  int     tm_min;    /* minutes after the hour [0-59] */
  int     tm_hour;   /* hours since midnight [0-23] */
  int     tm_mday;   /* day of the month [1-31] */
  int     tm_mon;    /* months since January [0-11] */
  int     tm_year;   /* years since 1900 */
  int     tm_wday;   /* days since Sunday [0-6] */
  int     tm_yday;   /* days since January 1 [0-365] */
  int     tm_isdst;  /* Daylight Savings Time flag */
  long    tm_gmtoff; /* offset from CUT in seconds */
  char    *tm_zone;  /* timezone abbreviation */
};

struct timeval {
  long    tv_sec;   /* time value, in seconds */
  long    tv_usec;  /* time value, in microseconds */
};
