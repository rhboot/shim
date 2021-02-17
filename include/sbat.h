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

struct sbat_entry {
	const CHAR8 *component_name;
	const CHAR8 *component_generation;
	const CHAR8 *vendor_name;
	const CHAR8 *vendor_package_name;
	const CHAR8 *vendor_version;
	const CHAR8 *vendor_url;
};

EFI_STATUS parse_sbat(char *sbat_base, size_t sbat_size, size_t *sbats, struct sbat_entry ***sbat);
void cleanup_sbat_entries(size_t n, struct sbat_entry **entries);

EFI_STATUS verify_sbat(size_t n, struct sbat_entry **entries);

#endif /* !SBAT_H_ */
// vim:fenc=utf-8:tw=75:noet
