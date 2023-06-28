// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-csv.c - test our csv parser
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

#include <stdio.h>

int
test_csv_simple_fuzz(char *random_bin, size_t random_bin_len)
{
	list_t entry_list;
	size_t i;
	char *current, *end;
	list_t *pos = NULL;
	EFI_STATUS efi_status;

	INIT_LIST_HEAD(&entry_list);

	current = &random_bin[0];
	current = current + 1 - 1;
	end = current + random_bin_len - 1;
	*end = '\0';

	efi_status = parse_csv_data(current, end, 7, &entry_list);
	if (efi_status != EFI_SUCCESS)
		return 0;
	if (list_size(&entry_list) <= 1)
		goto fail;

	i = 0;
	list_for_each(pos, &entry_list) {
		struct csv_row *csv_row;

		csv_row = list_entry(pos, struct csv_row, list);
		i++;
	}

	free_csv_list(&entry_list);

	return 0;
fail:
	free_csv_list(&entry_list);
	return -1;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	int rc;
	uint8_t *data_copy;

	if (size < 1)
		return 0;

	data_copy = malloc(size);
	if (!data_copy)
		return -1;

	memcpy(data_copy, data, size);
	rc = test_csv_simple_fuzz((char *)data_copy, size);
	free(data_copy);

	return rc; // Values other than 0 and -1 are reserved for future use.
}

// vim:fenc=utf-8:tw=75:noet
