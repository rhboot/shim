// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SBAT_VAR_DEFS_H_
#define SBAT_VAR_DEFS_H_

#define QUOTEVAL(s) QUOTE(s)
#define QUOTE(s) #s

/*
 * This is the entry for the sbat data format
 */
#define SBAT_VAR_SIG "sbat,"
#define SBAT_VAR_VERSION "1,"
#define SBAT_VAR_ORIGINAL_DATE "2021030218"
#define SBAT_VAR_ORIGINAL \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_ORIGINAL_DATE "\n"

#if defined(ENABLE_SHIM_DEVEL)
#define SBAT_VAR_AUTOMATIC_DATE "2021030218"
#define SBAT_VAR_AUTOMATIC \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_AUTOMATIC_DATE "\n"

#define SBAT_VAR_LATEST_DATE "2022050100"
#define SBAT_VAR_LATEST_REVOCATIONS "component,2\nothercomponent,2\n"
#define SBAT_VAR_LATEST \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_LATEST_DATE "\n" \
	SBAT_VAR_LATEST_REVOCATIONS
#else /* !ENABLE_SHIM_DEVEL */

/*
 * Some distros may want to apply revocations from 2022052400
 * or 2022111500 automatically. They can be selected by setting
 * SBAT_AUTOMATIC_DATE=<datestamp> at build time. Otherwise the
 * default is to apply the second to most recent revocations
 * automatically. Distros that need to manage automatic updates
 * externally from shim can choose the epoch 2021030218 emtpy
 * revocations.
 */
#ifndef SBAT_AUTOMATIC_DATE
#define SBAT_AUTOMATIC_DATE 2023012900
#endif /* SBAT_AUTOMATIC_DATE */
#if SBAT_AUTOMATIC_DATE == 2021030218
#define SBAT_VAR_AUTOMATIC_REVOCATIONS
#elif SBAT_AUTOMATIC_DATE == 2022052400
#define SBAT_VAR_AUTOMATIC_REVOCATIONS "grub,2\n"
#elif SBAT_AUTOMATIC_DATE == 2022111500
#define SBAT_VAR_AUTOMATIC_REVOCATIONS "shim,2\ngrub,3\n"
#elif SBAT_AUTOMATIC_DATE == 2023012900
#define SBAT_VAR_AUTOMATIC_REVOCATIONS "shim,2\ngrub,3\ngrub.debian,4\n"
#else
#error "Unknown SBAT_AUTOMATIC_DATE"
#endif /* SBAT_AUTOMATIC_DATE == */
#define SBAT_VAR_AUTOMATIC_DATE QUOTEVAL(SBAT_AUTOMATIC_DATE)
#define SBAT_VAR_AUTOMATIC \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_AUTOMATIC_DATE "\n" \
	SBAT_VAR_AUTOMATIC_REVOCATIONS

/*
 * Revocations for January 2024 shim CVEs
 */
#define SBAT_VAR_LATEST_DATE "2024010900"
#define SBAT_VAR_LATEST_REVOCATIONS "shim,4\ngrub,3\ngrub.debian,4\n"
#define SBAT_VAR_LATEST \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_LATEST_DATE "\n" \
	SBAT_VAR_LATEST_REVOCATIONS
#endif /* ENABLE_SHIM_DEVEL */
#endif /* !SBAT_VAR_DEFS_H_ */
