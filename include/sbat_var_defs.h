// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SBAT_VAR_DEFS_H_
#define SBAT_VAR_DEFS_H_

/*
 * This is the entry for the sbat data format
 */
#define SBAT_VAR_SIG           "sbat,"
#define SBAT_VAR_VERSION       "1,"
#define SBAT_VAR_ORIGINAL_DATE "2021030218"
#define SBAT_VAR_ORIGINAL \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_ORIGINAL_DATE "\n"

#if defined(ENABLE_SHIM_DEVEL)
#define SBAT_VAR_PREVIOUS_DATE "2021030218"
#define SBAT_VAR_PREVIOUS \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_PREVIOUS_DATE "\n"

#define SBAT_VAR_LATEST_DATE        "2022050100"
#define SBAT_VAR_LATEST_REVOCATIONS "component,2\nothercomponent,2\n"
#define SBAT_VAR_LATEST                                    \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_LATEST_DATE \
		"\n" SBAT_VAR_LATEST_REVOCATIONS
#else /* !ENABLE_SHIM_DEVEL */
/*
 * At this point we do not want shim to automatically apply a
 * previous revocation unless it is delivered by a separately
 * installed signed revocations binary.
 */
#define SBAT_VAR_PREVIOUS_DATE "2021030218"
#define SBAT_VAR_PREVIOUS \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_PREVIOUS_DATE "\n"

/*
 * Debian's grub.3 update was broken - some binaries included the SBAT
 * data update but not the security patches :-(
 */
#define SBAT_VAR_LATEST_DATE        "2023012900"
#define SBAT_VAR_LATEST_REVOCATIONS "shim,2\ngrub,3\ngrub.debian,4\n"
#define SBAT_VAR_LATEST                                    \
	SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_LATEST_DATE \
		"\n" SBAT_VAR_LATEST_REVOCATIONS
#endif /* ENABLE_SHIM_DEVEL */

#endif /* !SBAT_VAR_DEFS_H_ */
