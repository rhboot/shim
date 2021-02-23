// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * sbat.c - parse SBAT data from the .rsrc section data
 */

#ifndef SBAT_H_
#define SBAT_H_

extern UINTN _sbat, _esbat;

struct sbat_var {
	const CHAR8 *component_name;
	const CHAR8 *component_generation;
	list_t list;
};
extern list_t sbat_var;

EFI_STATUS parse_sbat_var(list_t *entries);
void cleanup_sbat_var(list_t *entries);

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

#endif /* !SBAT_H_ */
// vim:fenc=utf-8:tw=75:noet
