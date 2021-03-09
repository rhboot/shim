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

/*
 * copy-pasta from gnu-efi
 */
static inline UINTN
strncmpa (
    IN CONST CHAR8    *s1,
    IN CONST CHAR8    *s2,
    IN UINTN    len
    )
// compare strings
{
    while (*s1  &&  len) {
        if (*s1 != *s2) {
            break;
        }

        s1  += 1;
        s2  += 1;
        len -= 1;
    }

    return len ? *s1 - *s2 : 0;
}

int
test_strchrnul(void)
{
	const char s0[] = "abcd\0fghi";

	assert_equal_return(shim_strchrnul(s0, 'a'), &s0[0], -1, "got %p expected %p\n");
	assert_equal_return(shim_strchrnul(s0, 'd'), &s0[3], -1, "got %p expected %p\n");
	assert_equal_return(shim_strchrnul(s0, '\000'), &s0[4], -1, "got %p expected %p\n");
	assert_equal_return(shim_strchrnul(s0, 'i'), &s0[4], -1, "got %p expected %p\n");

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
test_strncmp(void)
{
	/*
	 * these are constants so that the failures are readable if you get
	 * it wrong.
	 */
#define s0 "sbat,"
#define s0sz 6
#define s0len 5
#define s1 "sbat,1,2021030218"
#define s1sz 18
#define s1len 17
#define s2 "sbat,1,20210302"
#define s2sz 16
#define s2len 15
#define s3 "sbat,1,20210303"
#define s3sz 16
#define s3len 15

	int diff;

	assert_zero_return(strncmp(s0, s0, s0len), -1, "\n");
	assert_zero_return(strncmp(s0, s0, s0sz), -1, "\n");

	assert_zero_return(strncmp(s0, s1, s0len), -1, "\n");
	assert_negative_return(strncmp(s0, s1, s0sz), -1, "\n");
	assert_equal_return(strncmp(s0, s1, s0sz), s0[s0len] - s1[s0len] , -1, "expected %d got %d\n");
	assert_positive_return(strncmp(s1, s0, s0sz), -1, "\n");
	assert_equal_return(strncmp(s1, s0, s0sz), s1[s0len] - s0[s0len] , -1, "expected %d got %d\n");

	assert_positive_return(strncmp(s1, s2, s1sz), -1, "\n");
	assert_equal_return(strncmp(s1, s2, s2sz), s1[s2len] - s2[s2len] , -1, "expected %d got %d\n");
	assert_positive_return(strncmp(s1, s2, s1len), -1, "\n");
	assert_equal_return(strncmp(s1, s2, s2len), s1[s2len-1] - s2[s2len-1] , -1, "expected %d got %d\n");
	assert_negative_return(strncmp(s2, s1, s1sz), -1, "\n");
	assert_equal_return(strncmp(s2, s1, s1sz), s2[s2len] - s1[s2len] , -1, "expected %d got %d\n");

	assert_zero_return(strncmp(s1, s2, s2len), -1, "\n");
	assert_positive_return(strncmp(s1, s2, s2sz), -1, "\n");
	assert_equal_return(strncmp(s1, s2, s2sz), s1[s2len] - s2[s2len] , -1, "expected %d got %d\n");

	assert_negative_return(strncmp(s2, s3, s2sz), -1, "\n");
	assert_equal_return(strncmp(s2, s3, s2sz), s2[s2len-1] - s3[s2len-1] , -1, "expected %d got %d\n");
	assert_equal_return(strncmp(s2, s3, s2len), s2[s2len-1] - s3[s2len-1] , -1, "expected %d got %d\n");
	assert_negative_return(strncmp(s2, s3, s2len), -1, "\n");
	assert_zero_return(strncmp(s2, s3, s2len - 1), -1, "\n");
	assert_false_return(strncmp(s1, s2, s2len), -1, "\n");

	/*
	 * Now test gnu-efi's version, but with a cast back to a sane type
	 */
#define strncmpa(a, b, c) ((INTN)strncmpa(a, b, c))

	assert_zero_return(strncmpa(s0, s0, s0len), -1, "\n");
	assert_zero_return(strncmpa(s0, s0, s0sz), -1, "\n");

	assert_zero_return(strncmpa(s0, s1, s0len), -1, "\n");
	assert_negative_return(strncmpa(s0, s1, s0sz), -1, "\n");
	assert_equal_return(strncmpa(s0, s1, s0sz), s0[s0len] - s1[s0len] , -1, "expected %d got %d\n");
	assert_positive_return(strncmpa(s1, s0, s0sz), -1, "\n");
	assert_equal_return(strncmpa(s1, s0, s0sz), s1[s0len] - s0[s0len] , -1, "expected %d got %d\n");

	assert_positive_return(strncmpa(s1, s2, s1sz), -1, "\n");
	assert_equal_return(strncmpa(s1, s2, s2sz), s1[s2len] - s2[s2len] , -1, "expected %d got %d\n");
	assert_positive_return(strncmpa(s1, s2, s1len), -1, "\n");
	assert_equal_return(strncmpa(s1, s2, s2len), s1[s2len-1] - s2[s2len-1] , -1, "expected %d got %d\n");
	assert_negative_return(strncmpa(s2, s1, s1sz), -1, "\n");
	assert_equal_return(strncmpa(s2, s1, s1sz), s2[s2len] - s1[s2len] , -1, "expected %d got %d\n");

	assert_zero_return(strncmpa(s1, s2, s2len), -1, "\n");
	assert_positive_return(strncmpa(s1, s2, s2sz), -1, "\n");
	assert_equal_return(strncmpa(s1, s2, s2sz), s1[s2len] - s2[s2len] , -1, "expected %d got %d\n");

	assert_negative_return(strncmpa(s2, s3, s2sz), -1, "\n");
	assert_equal_return(strncmpa(s2, s3, s2sz), s2[s2len-1] - s3[s2len-1] , -1, "expected %d got %d\n");
	assert_equal_return(strncmpa(s2, s3, s2len), s2[s2len-1] - s3[s2len-1] , -1, "expected %d got %d\n");
	assert_negative_return(strncmpa(s2, s3, s2len), -1, "\n");
	assert_zero_return(strncmpa(s2, s3, s2len - 1), -1, "\n");
	assert_false_return(strncmpa(s1, s2, s2len), -1, "\n");


	/*
	 * Once more, but with the casting /and the warnings/ turned off
	 *
	 * The ones marked with XXX I've inverted the test to make it work
	 * "correctly", because UINTN is what makes positive.
	 */
#undef strncmpa
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wsign-compare"

	assert_zero_return(strncmpa(s0, s0, s0len), -1, "\n");
	assert_zero_return(strncmpa(s0, s0, s0sz), -1, "\n");

	assert_zero_return(strncmpa(s0, s1, s0len), -1, "\n");
	/*XXX*/assert_positive_return(strncmpa(s0, s1, s0sz), -1, "\n");/*XXX*/
	assert_equal_return(strncmpa(s0, s1, s0sz), s0[s0len] - s1[s0len] , -1, "expected %d got %d\n");
	assert_positive_return(strncmpa(s1, s0, s0sz), -1, "\n");
	assert_equal_return(strncmpa(s1, s0, s0sz), s1[s0len] - s0[s0len] , -1, "expected %d got %d\n");

	assert_positive_return(strncmpa(s1, s2, s1sz), -1, "\n");
	assert_equal_return(strncmpa(s1, s2, s2sz), s1[s2len] - s2[s2len] , -1, "expected %d got %d\n");
	assert_positive_return(strncmpa(s1, s2, s1len), -1, "\n");
	assert_equal_return(strncmpa(s1, s2, s2len), s1[s2len-1] - s2[s2len-1] , -1, "expected %d got %d\n");
	/*XXX*/ assert_positive_return(strncmpa(s2, s1, s1sz), -1, "\n");/*XXX*/
	assert_equal_return(strncmpa(s2, s1, s1sz), s2[s2len] - s1[s2len] , -1, "expected %d got %d\n");

	assert_zero_return(strncmpa(s1, s2, s2len), -1, "\n");
	assert_positive_return(strncmpa(s1, s2, s2sz), -1, "\n");
	assert_equal_return(strncmpa(s1, s2, s2sz), s1[s2len] - s2[s2len] , -1, "expected %d got %d\n");

	assert_positive_return(strncmpa(s2, s3, s2sz), -1, "\n");
	assert_equal_return(strncmpa(s2, s3, s2sz), s2[s2len-1] - s3[s2len-1] , -1, "expected %d got %d\n");
	assert_equal_return(strncmpa(s2, s3, s2len), s2[s2len-1] - s3[s2len-1] , -1, "expected %d got %d\n");
	/*XXX*/assert_positive_return(strncmpa(s2, s3, s2len), -1,"\n");/*XXX*/
	assert_zero_return(strncmpa(s2, s3, s2len - 1), -1, "\n");
	assert_false_return(strncmpa(s1, s2, s2len), -1, "\n");

#pragma GCC diagnostic error "-Wsign-compare"
#pragma GCC diagnostic error "-Wtype-limits"
	return 0;

#undef s0
#undef s0sz
#undef s0len
#undef s1
#undef s1sz
#undef s1len
#undef s2
#undef s2sz
#undef s2len
#undef s3
#undef s3sz
#undef s3len
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
	test(test_strncmp);
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
