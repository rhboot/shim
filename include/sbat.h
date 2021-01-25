// SPDX-License-Identifier: BSD/Tiano-XXX-FIXME
/*
 * sbat.c - parse SBAT data from the .rsrc section data
 */

#ifndef SBAT_H_
#define SBAT_H_

#include "shim.h"

struct sbat_metadata {
	const CHAR8 *sbat_data_version;
	const CHAR8 *component_name;
	const CHAR8 *component_generation;
	const CHAR8 *product_name;
	const CHAR8 *product_generation;
	const CHAR8 *product_version;
	const CHAR8 *version_generation;
};

int parse_sbat(char *sbat_base, size_t sbat_size, char *buffer,
	       struct sbat_metadata *sbat);

#endif /* !SBAT_H_ */
// vim:fenc=utf-8:tw=75:noet
