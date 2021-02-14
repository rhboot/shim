// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * sbat.c - parse SBAT data from the .rsrc section data
 */

#ifndef SBAT_H_
#define SBAT_H_

#include "shim.h"

struct sbat_var {
	const CHAR8 *component_generation;
	const CHAR8 *component_name_size;
	const CHAR8 *component_name;
	list_t list;
};

#define for_each_sbat_var(sbat, head) list_for_each(sbat, head)
#define for_each_sbat_var_safe(sbat, n, head) list_for_each_safe(sbat, n, head)

struct sbat_entry {
	const CHAR8 *component_name;
	const CHAR8 *component_generation;
	const CHAR8 *vendor_name;
	const CHAR8 *vendor_package_name;
	const CHAR8 *vendor_version;
	const CHAR8 *vendor_url;
};

struct sbat {
	unsigned int size;
	struct sbat_entry **entries;
};

EFI_STATUS parse_sbat_var(list_t *entries);

EFI_STATUS verify_sbat(struct sbat *sbat, list_t *entries);

EFI_STATUS parse_sbat(char *sbat_base, size_t sbat_size, char *buffer,
		      struct sbat *sbat);

#endif /* !SBAT_H_ */
// vim:fenc=utf-8:tw=75:noet
