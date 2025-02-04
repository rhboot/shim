// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SBAT_VAR_DEFS_H_
#define SBAT_VAR_DEFS_H_

#define QUOTEVAL(s) QUOTE(s)
#define QUOTE(s) #s

/*
 * SbatLevel Epoch and SHIM_DEVEL definitions are here
 * Actual revocations are now soley defined in
 * SbatLevel_Variable.txt
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
#endif /* ENABLE_SHIM_DEVEL */

#endif /* !SBAT_VAR_DEFS_H_ */
