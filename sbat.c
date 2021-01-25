// SPDX-License-Identifier: BSD/Tiano-XXX-FIXME
/*
 * sbat.c - parse SBAT data from the .sbat section data
 */

#include "sbat.h"
#include <string.h>

CHAR8 *get_sbat_field(CHAR8 *current, CHAR8 *end, const CHAR8 **field)
{
	CHAR8 *comma = strchr(current, ',');
	if (comma && comma + 1 > end)
		return NULL;

	*field = current;

	if (comma) {
		*comma = '\0';
		return comma + 1;
	}

	return NULL;
}

/*
 * Parse SBAT data from the .sbat section data
 */
int parse_sbat(char *sbat_base, size_t sbat_size, char *buffer,
	       struct sbat_metadata *sbat)
{
	CHAR8 *current = (CHAR8 *) sbat_base;
	CHAR8 *end = (CHAR8 *) sbat_base + sbat_size;

	while ((*current == '\r' || *current == '\n') && current < end)
		current++;

	if (current == end)
		return -1;

	while ((*end == '\r' || *end == '\n') && end < current)
		end--;

	*(end - 1) = '\0';

	current = get_sbat_field(current, end, &sbat->sbat_data_version);
	if (!sbat->sbat_data_version)
		return -1;

	current = get_sbat_field(current, end, &sbat->component_name);
	if (!sbat->component_name)
		return -1;

	current = get_sbat_field(current, end, &sbat->component_generation);
	if (!sbat->component_generation)
		return -1;

	current = get_sbat_field(current, end, &sbat->product_name);
	if (!sbat->product_name)
		return -1;

	current = get_sbat_field(current, end, &sbat->product_generation);
	if (!sbat->product_generation)
		return -1;

	current = get_sbat_field(current, end, &sbat->product_version);
	if (!sbat->product_version)
		return -1;

	current = get_sbat_field(current, end, &sbat->version_generation);
	if (!sbat->version_generation)
		return -1;

	return 0;
}

// vim:fenc=utf-8:tw=75:noet
