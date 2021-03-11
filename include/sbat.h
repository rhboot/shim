// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * sbat.c - parse SBAT data from the .sbat section data
 */

#ifndef SBAT_H_
#define SBAT_H_

#define SBAT_VAR_SIG "sbat,"
#define SBAT_VAR_VERSION "1,"
#define SBAT_VAR_DATE "2021030218"
#define SBAT_VAR SBAT_VAR_SIG SBAT_VAR_VERSION SBAT_VAR_DATE "\n"

#define UEFI_VAR_NV_BS \
	(EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)
#define UEFI_VAR_NV_BS_RT                                              \
	(EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | \
	 EFI_VARIABLE_RUNTIME_ACCESS)
#define UEFI_VAR_NV_BS_TIMEAUTH \
	(UEFI_VAR_NV_BS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)

#if defined(ENABLE_SHIM_DEVEL)
#define SBAT_VAR_NAME L"SBAT_DEVEL"
#define SBAT_VAR_NAME8 "SBAT_DEVEL"
#define SBAT_RT_VAR_NAME L"SbatRT_DEVEL"
#define SBAT_RT_VAR_NAME8 "SbatRT_DEVEL"
#define SBAT_VAR_ATTRS UEFI_VAR_NV_BS_RT
#else
#define SBAT_VAR_NAME L"SBAT"
#define SBAT_VAR_NAME8 "SBAT"
#define SBAT_RT_VAR_NAME L"SbatRT"
#define SBAT_RT_VAR_NAME8 "SbatRT"
#define SBAT_VAR_ATTRS UEFI_VAR_NV_BS
#endif

extern UINTN _sbat, _esbat;

struct sbat_var_entry {
	const CHAR8 *component_name;
	const CHAR8 *component_generation;
	/*
	 * This column is only actually on the "sbat" version entry
	 */
	const CHAR8 *sbat_datestamp;
	list_t list;
};
extern list_t sbat_var;
#define SBAT_VAR_COLUMNS ((sizeof (struct sbat_var_entry) - sizeof(list_t)) / sizeof(CHAR8 *))
#define SBAT_VAR_REQUIRED_COLUMNS (SBAT_VAR_COLUMNS - 1)

EFI_STATUS parse_sbat_var(list_t *entries);
void cleanup_sbat_var(list_t *entries);
EFI_STATUS set_sbat_uefi_variable(void);

struct sbat_section_entry {
	const CHAR8 *component_name;
	const CHAR8 *component_generation;
	const CHAR8 *vendor_name;
	const CHAR8 *vendor_package_name;
	const CHAR8 *vendor_version;
	const CHAR8 *vendor_url;
};
#define SBAT_SECTION_COLUMNS (sizeof (struct sbat_section_entry) / sizeof(CHAR8 *))

EFI_STATUS
parse_sbat_section(char *section_base, size_t section_size, size_t *n,
		   struct sbat_section_entry ***entriesp);
void cleanup_sbat_section_entries(size_t n, struct sbat_section_entry **entries);

EFI_STATUS verify_sbat(size_t n, struct sbat_section_entry **entries);

#ifdef SHIM_UNIT_TEST
EFI_STATUS parse_sbat_var_data(list_t *entries, UINT8 *data, UINTN datasize);
EFI_STATUS verify_sbat_helper(list_t *sbat_var, size_t n,
                              struct sbat_section_entry **entries);
#endif /* !SHIM_UNIT_TEST */
#endif /* !SBAT_H_ */
// vim:fenc=utf-8:tw=75:noet
