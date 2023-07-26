// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * fuzz-pe-relocate.c - fuzz our PE relocation code.
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

UINT8 mok_policy = 0;

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	uint8_t *data_copy;
	EFI_STATUS status = 0;
	size_t n = 0;
	PE_COFF_LOADER_IMAGE_CONTEXT context = { 0, };

	if (size < 1)
		return 0;

	data_copy = malloc(size+1);
	if (!data_copy)
		return -1;

	memcpy(data_copy, data, size);
	data_copy[size] = 0;

	status = read_header(data_copy, size, &context);

	free(data_copy);

	return 0;
}

// vim:fenc=utf-8:tw=75:noet
