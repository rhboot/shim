// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-pe-util.c - test PE utilities
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

static int
test_is_page_aligned(void)
{
	assert_true_return(IS_PAGE_ALIGNED(0), -1, "\n");
	assert_false_return(IS_PAGE_ALIGNED(1), -1, "\n");
	assert_false_return(IS_PAGE_ALIGNED(4095), -1, "\n");
	assert_true_return(IS_PAGE_ALIGNED(4096), -1, "\n");
	assert_false_return(IS_PAGE_ALIGNED(4097), -1, "\n");

	return 0;
}

int
main(void)
{
	int status = 0;
	test(test_is_page_aligned);

	return status;
}
