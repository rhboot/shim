// SPDX-License-Identifier: BSD/Tiano-XXX-FIXME
/*
 * sbat.c - parse SBAT data from the .rsrc section data
 */

#ifndef SBAT_H_
#define SBAT_H_

#include "shim.h"

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

EFI_STATUS parse_sbat(char *sbat_base, size_t sbat_size, char *buffer,
		      struct sbat *sbat);

#endif /* !SBAT_H_ */
// vim:fenc=utf-8:tw=75:noet
