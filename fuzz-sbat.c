// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * fuzz-sbat-section.c - fuzz our .sbat parsing code
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

#include <stdio.h>

list_t sbat_var;

BOOLEAN
secure_mode() {
	return 1;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	uint8_t *data_copy;
	EFI_STATUS status = 0;
	size_t n = 0;
	struct sbat_section_entry **entries = NULL;

	if (size < 1)
		return 0;

	data_copy = malloc(size+1);
	if (!data_copy)
		return -1;

	memcpy(data_copy, data, size);
	data_copy[size] = 0;
	status = parse_sbat_section(data_copy, size, &n, &entries);
	cleanup_sbat_section_entries(n, entries);

	free(data_copy);

	return 0;
}

// vim:fenc=utf-8:tw=75:noet
