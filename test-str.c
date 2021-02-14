// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-str.c - test our string functions.
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

#include <stdio.h>

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic error "-Wnonnull"

int
test_strchrnul(void)
{
	const char s0[] = "abcd\0fghi";

	assert_equal_return(strchrnul(s0, 'a'), &s0[0], -1, "got %p expected %p\n");
	assert_equal_return(strchrnul(s0, 'd'), &s0[3], -1, "got %p expected %p\n");
	assert_equal_return(strchrnul(s0, '\000'), &s0[4], -1, "got %p expected %p\n");
	assert_equal_return(strchrnul(s0, 'i'), &s0[4], -1, "got %p expected %p\n");

	assert_equal_return(strnchrnul(s0, 0, 'b'), &s0[0], -1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, -1, 'b'), &s0[1], 1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, 2, 'b'), &s0[1], -1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, 4, 'f'), &s0[3], -1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, 5, 'f'), &s0[4], -1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, 8, 'f'), &s0[4], -1, "got %p expected %p\n");

	assert_equal_return(strnchrnul(&s0[4], 1, 'f'), &s0[4], -1, "got %p expected %p\n");

	return 0;
}

int
test_strntoken_null(void) {
	bool ret;
	char *token = NULL;
	char state;

	char *delims = alloca(3);
	memcpy(delims, ",.", 3);

	ret = strntoken(NULL, 1, delims, &token, &state);
	assert_equal_return(ret, false, -1, "got %d expected %d\n");
	return 0;
}

int
test_strntoken_size_0(void)
{
	const char s1[] = "abc,def,.,gh,";
	char s2[] = "abc,def,.,gh,";
	char *token = NULL;
	bool ret;
	size_t max;
	char *s = s2;
	size_t tokensz;
	char state;

	ret = strntoken(s, 0, ",.", &token, &state);
	assert_equal_return(ret, false, -1, "got %d expected %d\n");
	assert_equal_return(token, NULL, -1, "got %p expected %p\n");
	assert_equal_return(memcmp(s, "abc,def,.,gh,", sizeof(s2)), 0, 1, "got %d expected %d\n");

	return 0;
}

int
test_strntoken_empty_size_1(void)
{
	char s1[] = "";
	char *s;
	bool ret;
	char *token = NULL;
	char *prevtok = NULL;
	size_t max;
	size_t tokensz;
	char state;

	s = s1;
	max = 1;

	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, -1, "got %d expected %d\n");
	assert_equal_return(token[0], '\0', -1, "got %#hhx expected %#hhx\n");
	prevtok = token;

	tokensz = strlen(token) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(s, &s1[1], -1, "got %p expected %p\n");
	assert_equal_return(max, 0, -1, "got %d expected %d\n");

	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, false, -1, "got %d expected %d\n");
	assert_equal_return(token, prevtok, -1, "got %p expected %p\n");

	return 0;
}

int
test_strntoken_size_1(void)
{
	char s1[] = ",";
	char *s;
	bool ret;
	char *token = NULL;
	char *prevtok = NULL;
	size_t max;
	size_t tokensz;
	char state;

	s = s1;
	max = 1;

	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, -1, "got %d expected %d\n");
	assert_equal_return(token[0], '\0', -1, "got %#hhx expected %#hhx\n");
	prevtok = token;

	tokensz = strlen(token) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(s, &s1[1], -1, "got %p expected %p\n");
	assert_equal_return(max, 0, -1, "got %d expected %d\n");

	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, false, -1, "got %d expected %d\n");
	assert_equal_return(token, prevtok, -1, "got %p expected %p\n");
	assert_equal_return(token[0], '\0', -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_strntoken_size_2(void)
{
	char s1[] = ",";
	char *s;
	bool ret;
	char *token = NULL;
	char *prevtok = NULL;
	size_t max;
	size_t tokensz;
	char state;

	s = s1;
	max = 2;

	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, -1, "got %d expected %d\n");
	assert_equal_return(token[0], '\0', -1, "got %#hhx expected %#hhx\n");

	tokensz = strlen(token) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(s, &s1[1], -1, "got %p expected %p\n");
	assert_equal_return(max, 1, -1, "got %d expected %d\n");

	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, -1, "got %d expected %d\n");
	assert_equal_return(token[0], '\0', -1, "got %#hhx expected %#hhx\n");
	prevtok = token;

	tokensz = strlen(token) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(s, &s1[2], -1, "got %p expected %p\n");
	assert_equal_return(max, 0, -1, "got %d expected %d\n");

	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, false, -1, "got %d expected %d\n");
	assert_equal_return(token, prevtok, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_strntoken_no_ascii_nul(void)
{
	const char s1[] = "abc,def,.,gh,";
	char s2[] = "abc,def,.,gh,";
	char *token = NULL;
	bool ret;
	size_t max;
	char *s = s2;
	size_t tokensz;
	char state;

	s = s2;
	max = sizeof(s2) - 1;
	assert_equal_return(max, 13, -1, "got %d expected %d\n");
	/*
	 * s="abc,def,.,gh," -> "abc\0def,.,gh,"
	 *                       ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, -1, "got %d expected %d\n");
	assert_equal_return(token, s, -1, "got %p expected %p\n");
	assert_equal_return(s[2], 'c', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[3], '\0', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[4], 'd', -1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 9, -1, "got %d expected %d\n");

	/*
	 * s="def,.,gh," -> "def\0.,gh,"
	 *                   ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, -1, "got %d expected %d\n");
	assert_equal_return(token, s, -1, "got %p expected %p\n");
	assert_equal_return(s[2], 'f', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[3], '\0', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[4], '.', -1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 5, -1, "got %d expected %d\n");

	/*
	 * s=".,gh," -> "\0,gh,"
	 *               ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, -1, "got %d expected %d\n");
	assert_equal_return(token, s, -1, "got %p expected %p\n");
	assert_equal_return(s[0], '\0', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[1], ',', -1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 4, -1, "got %d expected %d\n");

	/*
	 * s=",gh," -> "\0gh,"
	 *              ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, -1, "got %d expected %d\n");
	assert_equal_return(token, s, -1, "got %p expected %p\n");
	assert_equal_return(s[0], '\0', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[1], 'g', -1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 3, -1, "got %d expected %d\n");

	/*
	 * s="gh," -> "gh\0"
	 *             ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, -1, "got %d expected %d\n");
	assert_equal_return(token, s, -1, "got %p expected %p\n");
	assert_equal_return(s[0], 'g', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[1], 'h', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[2], '\0', -1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 0, -1, "got %d expected %d\n");

	char *prevtok = token;

	/*
	 * s="" -> ""
	 *          ^ token, but max is 0
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, false, -1, "got %d expected %d\n");
	assert_equal_return(token, prevtok, -1, "got %p expected %p\n");
	assert_equal_return(s[0], '\0', -1, "got %#hhx expected %#hhx\n");

	s[0] = 'x';
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, false, -1, "got %d expected %d\n");
	assert_equal_return(token, prevtok, -1, "got %p expected %p\n");
	assert_equal_return(s[0], 'x', -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_strntoken_with_ascii_nul(void)
{
	const char s1[] = "abc,def,.,gh,";
	char s2[] = "abc,def,.,gh,";
	char *token = NULL;
	bool ret;
	size_t max;
	char *s = s2;
	size_t tokensz;
	char s3[] = "abc,def,.,gh,";
	char state;

	s = s2;
	max = sizeof(s2);
	assert_equal_return(max, 14, 1, "got %d expected %d\n");
	/*
	 * s="abc,def,.,gh," -> "abc\0def,.,gh,"
	 *                       ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, 1, "got %d expected %d\n");
	assert_equal_return(s[2], 'c', 1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[3], '\0', 1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[4], 'd', 1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 10, 1, "got %d expected %d\n");

	/*
	 * s="def,.,gh," -> "def\0.,gh,"
	 *                   ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, 1, "got %d expected %d\n");
	assert_equal_return(token, s, 1, "got %p expected %p\n");
	assert_equal_return(s[2], 'f', 1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[3], '\0', 1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[4], '.', 1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 6, 1, "got %d expected %d\n");

	/*
	 * s=".,gh," -> "\0,gh,"
	 *               ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, 1, "got %d expected %d\n");
	assert_equal_return(token, s, 1, "got %p expected %p\n");
	assert_equal_return(s[0], '\0', 1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[1], ',', 1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 5, 1, "got %d expected %d\n");

	/*
	 * s=",gh," -> "\0gh,"
	 *              ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, 1, "got %d expected %d\n");
	assert_equal_return(token, s, 1, "got %p expected %p\n");
	assert_equal_return(s[0], '\0', 1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[1], 'g', 1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 4, 1, "got %d expected %d\n");

	/*
	 * s="gh," -> "gh\0"
	 *             ^ token
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, 1, "got %d expected %d\n");
	assert_equal_return(token, s, 1, "got %p expected %p\n");
	assert_equal_return(s[0], 'g', 1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[1], 'h', 1, "got %#hhx expected %#hhx\n");
	assert_equal_return(s[2], '\0', 1, "got %#hhx expected %#hhx\n");

	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 1, 1, "got %d expected %d\n");

	/*
	 * s="" -> ""
	 *          ^ token, max is 1
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, true, 1, "got %d expected %d\n");
	assert_equal_return(token, s, 1, "got %p expected %p\n");
	assert_equal_return(s[0], '\0', 1, "got %#hhx expected %#hhx\n");

	char *prevtok = token;
	tokensz = strnlen(token, max) + 1;
	s += tokensz;
	max -= tokensz;
	assert_equal_return(max, 0, 1, "got %d expected %d\n");

	/*
	 * s="" -> ""
	 *          ^ token, max is 0
	 */
	ret = strntoken(s, max, ",.", &token, &state);
	assert_equal_return(ret, false, 1, "got %d expected %d\n");
	assert_equal_return(token, prevtok, 1, "got %p expected %p\n");

	return 0;
}

int
main(void)
{
	int status = 0;
	test(test_strchrnul);
	test(test_strntoken_null);
	test(test_strntoken_size_0);
	test(test_strntoken_empty_size_1);
	test(test_strntoken_size_1);
	test(test_strntoken_size_2);
	test(test_strntoken_no_ascii_nul);
	test(test_strntoken_with_ascii_nul);
	return status;
}

// vim:fenc=utf-8:tw=75:noet
