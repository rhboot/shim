// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-pe-reloc.c - attempt to test relocate_coff()
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

static int
test_image_address(void)
{
	char image[4];
	void *ret;

	assert_equal_return(ImageAddress(image, sizeof(image), 0), &image[0], -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress(image, sizeof(image), 4), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)1, 2, 3), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)-1ull, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)0, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)1, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)2, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)3, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");

	return 0;
}

int
main(void)
{
	int status = 0;
	test(test_image_address);

	return status;
}

// vim:fenc=utf-8:tw=75:noet
