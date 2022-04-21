// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-str.c - test our string functions.
 */
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wunused-function"

#pragma GCC diagnostic warning "-Wcpp"

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

#include <stdio.h>

static int
test_strlen(void)
{
	const char s1[] = "abcd";
	const char s2[] = "";

	assert_equal_return(shim_strlen(s1), 4, -1, "got %d expected %d\n");
	assert_equal_return(strlen(s1), 4, -1, "got %d expected %d\n");
	assert_equal_return(shim_strlen(s2), 0, -1, "got %d expected %d\n");
	assert_equal_return(strlen(s2), 0, -1, "got %d expected %d\n");

	return 0;
}

static int
test_strnlen(void)
{
	const char s1[] = "abcd";
	const char s2[] = "";

	assert_equal_return(shim_strnlen(s1, 0), 0, -1, "got %d expected %d\n");
	assert_equal_return(strnlen(s1, 0), 0, -1, "got %d expected %d\n");
	assert_equal_return(shim_strnlen(s1, 1), 1, -1, "got %d expected %d\n");
	assert_equal_return(strnlen(s1, 1), 1, -1, "got %d expected %d\n");
	assert_equal_return(shim_strnlen(s1, 3), 3, -1, "got %d expected %d\n");
	assert_equal_return(strnlen(s1, 3), 3, -1, "got %d expected %d\n");
	assert_equal_return(shim_strnlen(s1, 4), 4, -1, "got %d expected %d\n");
	assert_equal_return(strnlen(s1, 4), 4, -1, "got %d expected %d\n");
	assert_equal_return(shim_strnlen(s1, 5), 4, -1, "got %d expected %d\n");
	assert_equal_return(strnlen(s1, 5), 4, -1, "got %d expected %d\n");
	assert_equal_return(shim_strnlen(s2, 0), 0, -1, "got %d expected %d\n");
	assert_equal_return(strnlen(s2, 0), 0, -1, "got %d expected %d\n");
	assert_equal_return(shim_strnlen(s2, 1), 0, -1, "got %d expected %d\n");
	assert_equal_return(strnlen(s2, 1), 0, -1, "got %d expected %d\n");

	return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"

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
#define s4 "sbat\314\234\014,"
#define s4sz 9
#define s4len 8
/*
 * same as s4 but with a UTF8 encoding error; one bit is cleared.
 */
#define s5 "sbat\314\034\014,"
#define s5sz 9
#define s5len 8

#define test_strcmp_helper(fn, invert_sign_errors, invert_encoding_errors)   \
	({                                                                   \
		printf("testing %s\n", #fn);                                 \
		int status_ = 0, rc_;                                        \
                                                                             \
		rc_ = assert_zero_as_expr(fn(s0, s0), -1, "\n");             \
		status_ = MIN(status_, rc_);                                 \
                                                                             \
		if (invert_sign_errors)                                      \
			rc_ = assert_positive_as_expr(fn(s0, s1), -1, "\n"); \
		else                                                         \
			rc_ = assert_negative_as_expr(fn(s0, s1), -1, "\n"); \
		status_ = MIN(status_, rc_);                                 \
		rc_ = assert_positive_as_expr(fn(s1, s0), -1, "\n");         \
		status_ = MIN(status_, rc_);                                 \
                                                                             \
		rc_ = assert_positive_as_expr(fn(s1, s2), -1, "\n");         \
		status_ = MIN(status_, rc_);                                 \
		if (invert_sign_errors)                                      \
			rc_ = assert_positive_as_expr(fn(s2, s1), -1, "\n"); \
		else                                                         \
			rc_ = assert_negative_as_expr(fn(s2, s1), -1, "\n"); \
		status_ = MIN(status_, rc_);                                 \
                                                                             \
		if (invert_sign_errors)                                      \
			rc_ = assert_positive_as_expr(fn(s2, s3), -1, "\n"); \
		else                                                         \
			rc_ = assert_negative_as_expr(fn(s2, s3), -1, "\n"); \
		status_ = MIN(status_, rc_);                                 \
		rc_ = assert_positive_as_expr(fn(s3, s2), -1, "\n");         \
		status_ = MIN(status_, rc_);                                 \
                                                                             \
		if (invert_encoding_errors)                                  \
			rc_ = assert_negative_as_expr(fn(s4, s5), -1, "\n"); \
		else                                                         \
			rc_ = assert_positive_as_expr(fn(s4, s5), -1, "\n"); \
		status_ = MIN(status_, rc_);                                 \
                                                                             \
		if (status_ < 0)                                             \
			printf("%s failed\n", #fn);                          \
		status_;                                                     \
	})

static int
test_strcmp(void)
{
	int status = 0;
	int rc;

	/*
	 * shim's strcmp
	 */
	rc = test_strcmp_helper(shim_strcmp, false, false);
	status = MIN(rc, status);

	/*
	 * libc's strcmp
	 */
	rc = test_strcmp_helper(strcmp, false, false);
	status = MIN(rc, status);

	return status;
}

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
#undef s4
#undef s4sz
#undef s4len
#undef s5
#undef s5sz
#undef s5len

/*
 * these are constants so that the failures are readable if you get
 * it wrong.
 */
#define s0 "sbAt,"
#define s0sz 6
#define s0len 5
#define s1 "sbaT,1,2021030218"
#define s1sz 18
#define s1len 17
#define s2 "sbAt,1,20210302"
#define s2sz 16
#define s2len 15
#define s3 "sbaT,1,20210303"
#define s3sz 16
#define s3len 15
#define s4 "sbat\314\234\014,"
#define s4sz 9
#define s4len 8
/*
 * same as s4 but with a UTF8 encoding error; one bit is cleared.
 */
#define s5 "sbat\314\034\014,"
#define s5sz 9
#define s5len 8

#define test_strcasecmp_helper(fn, invert_sign_errors, invert_encoding_errors) \
	({                                                                     \
		printf("testing %s\n", #fn);                                   \
		int status_ = 0, rc_;                                          \
                                                                               \
		rc_ = assert_zero_as_expr(fn(s0, s0), -1, "\n");               \
		status_ = MIN(status_, rc_);                                   \
                                                                               \
		if (invert_sign_errors)                                        \
			rc_ = assert_positive_as_expr(fn(s0, s1), -1, "\n");   \
		else                                                           \
			rc_ = assert_negative_as_expr(fn(s0, s1), -1, "\n");   \
		status_ = MIN(status_, rc_);                                   \
		rc_ = assert_positive_as_expr(fn(s1, s0), -1, "\n");           \
		status_ = MIN(status_, rc_);                                   \
                                                                               \
		rc_ = assert_positive_as_expr(fn(s1, s2), -1, "\n");           \
		status_ = MIN(status_, rc_);                                   \
		if (invert_sign_errors)                                        \
			rc_ = assert_positive_as_expr(fn(s2, s1), -1, "\n");   \
		else                                                           \
			rc_ = assert_negative_as_expr(fn(s2, s1), -1, "\n");   \
		status_ = MIN(status_, rc_);                                   \
                                                                               \
		if (invert_sign_errors)                                        \
			rc_ = assert_positive_as_expr(fn(s2, s3), -1, "\n");   \
		else                                                           \
			rc_ = assert_negative_as_expr(fn(s2, s3), -1, "\n");   \
		status_ = MIN(status_, rc_);                                   \
		rc_ = assert_positive_as_expr(fn(s3, s2), -1, "\n");           \
		status_ = MIN(status_, rc_);                                   \
                                                                               \
		if (invert_encoding_errors)                                    \
			rc_ = assert_negative_as_expr(fn(s4, s5), -1, "\n");   \
		else                                                           \
			rc_ = assert_positive_as_expr(fn(s4, s5), -1, "\n");   \
		status_ = MIN(status_, rc_);                                   \
                                                                               \
		if (status_ < 0)                                               \
			printf("%s failed\n", #fn);                            \
		status_;                                                       \
	})

static int
test_strcasecmp(void)
{
	int status = 0;
	int rc;

	/*
	 * shim's strcasecmp
	 */
	rc = test_strcasecmp_helper(shim_strcasecmp, false, false);
	status = MIN(rc, status);

	/*
	 * libc's strcasecmp
	 */
	rc = test_strcasecmp_helper(strcasecmp, false, false);
	status = MIN(rc, status);

	return status;
}

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
#undef s4
#undef s4sz
#undef s4len
#undef s5
#undef s5sz
#undef s5len

/*
 * these are constants so that the failures are readable if you get
 * it wrong.
 */
#define s0 "sbAt,"
#define s0sz 6
#define s0len 5
#define s1 "sbaT,1,2021030218"
#define s1sz 18
#define s1len 17
#define s2 "sbAt,1,20210302"
#define s2sz 16
#define s2len 15
#define s3 "sbaT,1,20210303"
#define s3sz 16
#define s3len 15
#define s4 "sbat\314\234\014,"
#define s4sz 9
#define s4len 8
/*
 * same as s4 but with a UTF8 encoding error; one bit is cleared.
 */
#define s5 "sbat\314\034\014,"
#define s5sz 9
#define s5len 8

#define test_strncasecmp_helper(fn, test_cmp_magnitude, invert_sign_errors,   \
                                invert_encoding_errors)                       \
	({                                                                    \
		printf("testing %s\n", #fn);                                  \
		int status_ = 0, rc_;                                         \
                                                                              \
		rc_ = assert_zero_as_expr(fn(s0, s0, s0len), -1, "\n");       \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_zero_as_expr(fn(s0, s0, s0sz), -1, "\n");        \
		status_ = MIN(status_, rc_);                                  \
                                                                              \
		rc_ = assert_zero_as_expr(fn(s0, s1, s0len), -1, "\n");       \
		status_ = MIN(status_, rc_);                                  \
		if (invert_sign_errors)                                       \
			rc_ = assert_positive_as_expr(fn(s0, s1, s0sz), -1,   \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_negative_as_expr(fn(s0, s1, s0sz), -1,   \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s0, s1, s0sz),          \
			                           s0[s0len] - s1[s0len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
		rc_ = assert_positive_as_expr(fn(s1, s0, s0sz), -1, "\n");    \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s1, s0, s0sz),          \
			                           s1[s0len] - s0[s0len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
                                                                              \
		rc_ = assert_positive_as_expr(fn(s1, s2, s1sz), -1, "\n");    \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s1, s2, s2sz),          \
			                           s1[s2len] - s2[s2len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
		rc_ = assert_positive_as_expr(fn(s1, s2, s1len), -1, "\n");   \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(                           \
				fn(s1, s2, s2len),                            \
				s1[s2len - 1] - s2[s2len - 1], -1,            \
				"expected %d got %d\n");                      \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
		if (invert_sign_errors)                                       \
			rc_ = assert_positive_as_expr(fn(s2, s1, s1sz), -1,   \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_negative_as_expr(fn(s2, s1, s1sz), -1,   \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s2, s1, s1sz),          \
			                           s2[s2len] - s1[s2len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
                                                                              \
		rc_ = assert_zero_as_expr(fn(s1, s2, s2len), -1, "\n");       \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_positive_as_expr(fn(s1, s2, s2sz), -1, "\n");    \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s1, s2, s2sz),          \
			                           s1[s2len] - s2[s2len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
                                                                              \
		if (invert_sign_errors)                                       \
			rc_ = assert_positive_as_expr(fn(s2, s3, s2sz), -1,   \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_negative_as_expr(fn(s2, s3, s2sz), -1,   \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_equal_as_expr(fn(s2, s3, s2sz),                  \
		                           s2[s2len - 1] - s3[s2len - 1], -1, \
		                           "expected %d got %d\n");           \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_equal_as_expr(fn(s2, s3, s2len),                 \
		                           s2[s2len - 1] - s3[s2len - 1], -1, \
		                           "expected %d got %d\n");           \
		status_ = MIN(status_, rc_);                                  \
		if (invert_sign_errors)                                       \
			rc_ = assert_positive_as_expr(fn(s2, s3, s2len), -1,  \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_negative_as_expr(fn(s2, s3, s2len), -1,  \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_zero_as_expr(fn(s2, s3, s2len - 1), -1, "\n");   \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_false_as_expr(fn(s1, s2, s2len), -1, "\n");      \
		status_ = MIN(status_, rc_);                                  \
                                                                              \
		if (invert_encoding_errors)                                   \
			rc_ = assert_negative_as_expr(fn(s4, s5, s4sz), -1,   \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_positive_as_expr(fn(s4, s5, s4sz), -1,   \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
                                                                              \
		if (status_ < 0)                                              \
			printf("%s failed\n", #fn);                           \
		status_;                                                      \
	})

static int
test_strncasecmp(void)
{
	int status = 0;
	int rc;

	/*
	 * shim's strncasecmp
	 */
	rc = test_strncasecmp_helper(shim_strncasecmp, true, false, false);
	status = MIN(rc, status);

	/*
	 * libc's strncasecmp
	 */
	rc = test_strncasecmp_helper(strncasecmp, false, false, false);
	status = MIN(rc, status);

	return status;
}

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
#undef s4
#undef s4sz
#undef s4len
#undef s5
#undef s5sz
#undef s5len

/*
 * copy-pasta from gnu-efi
 */
static inline UINTN
gnuefi_strncmp (
    IN CONST CHAR8    *s1,
    IN CONST CHAR8    *s2,
    IN UINTN    len
    )
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

/*
 * This is still broken, and fails the test case as written on arm.
 * We no longer use this, so we do not strictly need to run it.
 */
#if !defined(__arm__) && !defined(__aarch64__)
static inline INTN
gnuefi_signed_strncmp (
    IN CONST CHAR8    *s1,
    IN CONST CHAR8    *s2,
    IN UINTN    len
    )
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
#endif

static inline INTN
gnuefi_good_strncmp (
    IN CONST CHAR8    *s1p,
    IN CONST CHAR8    *s2p,
    IN UINTN    len
    )
{
    CONST UINT8 *s1 = (CONST UINT8 *)s1p;
    CONST UINT8 *s2 = (CONST UINT8 *)s2p;

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
#define s4 "sbat\314\234\014,"
#define s4sz 9
#define s4len 8
/*
 * same as s4 but with a UTF8 encoding error; one bit is cleared.
 */
#define s5 "sbat\314\034\014,"
#define s5sz 9
#define s5len 8

#define test_strncmp_helper(fn, test_cmp_magnitude, invert_sign_errors,       \
                            invert_encoding_errors)                           \
	({                                                                    \
		printf("testing %s\n", #fn);                                  \
		int status_ = 0, rc_;                                         \
                                                                              \
		rc_ = assert_zero_as_expr(fn(s0, s0, s0len), -1, "\n");       \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_zero_as_expr(fn(s0, s0, s0sz), -1, "\n");        \
		status_ = MIN(status_, rc_);                                  \
                                                                              \
		rc_ = assert_zero_as_expr(fn(s0, s1, s0len), -1, "\n");       \
		status_ = MIN(status_, rc_);                                  \
		if (invert_sign_errors)                                       \
			rc_ = assert_positive_as_expr(fn(s0, s1, s0sz), -1,   \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_negative_as_expr(fn(s0, s1, s0sz), -1,   \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s0, s1, s0sz),          \
			                           s0[s0len] - s1[s0len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
		rc_ = assert_positive_as_expr(fn(s1, s0, s0sz), -1, "\n");    \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s1, s0, s0sz),          \
			                           s1[s0len] - s0[s0len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
                                                                              \
		rc_ = assert_positive_as_expr(fn(s1, s2, s1sz), -1, "\n");    \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s1, s2, s2sz),          \
			                           s1[s2len] - s2[s2len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
		rc_ = assert_positive_as_expr(fn(s1, s2, s1len), -1, "\n");   \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(                           \
				fn(s1, s2, s2len),                            \
				s1[s2len - 1] - s2[s2len - 1], -1,            \
				"expected %d got %d\n");                      \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
		if (invert_sign_errors)                                       \
			rc_ = assert_positive_as_expr(fn(s2, s1, s1sz), -1,   \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_negative_as_expr(fn(s2, s1, s1sz), -1,   \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s2, s1, s1sz),          \
			                           s2[s2len] - s1[s2len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
                                                                              \
		rc_ = assert_zero_as_expr(fn(s1, s2, s2len), -1, "\n");       \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_positive_as_expr(fn(s1, s2, s2sz), -1, "\n");    \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(fn(s1, s2, s2sz),          \
			                           s1[s2len] - s2[s2len], -1, \
			                           "expected %d got %d\n");   \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
                                                                              \
		if (invert_sign_errors)                                       \
			rc_ = assert_positive_as_expr(fn(s2, s3, s2sz), -1,   \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_negative_as_expr(fn(s2, s3, s2sz), -1,   \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
		if (test_cmp_magnitude) {                                     \
			rc_ = assert_equal_as_expr(                           \
				fn(s2, s3, s2sz),                             \
				s2[s2len - 1] - s3[s2len - 1], -1,            \
				"expected %d got %d\n");                      \
			status_ = MIN(status_, rc_);                          \
			rc_ = assert_equal_as_expr(                           \
				fn(s2, s3, s2len),                            \
				s2[s2len - 1] - s3[s2len - 1], -1,            \
				"expected %d got %d\n");                      \
			status_ = MIN(status_, rc_);                          \
		}                                                             \
		if (invert_sign_errors)                                       \
			rc_ = assert_positive_as_expr(fn(s2, s3, s2len), -1,  \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_negative_as_expr(fn(s2, s3, s2len), -1,  \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_zero_as_expr(fn(s2, s3, s2len - 1), -1, "\n");   \
		status_ = MIN(status_, rc_);                                  \
		rc_ = assert_false_as_expr(fn(s1, s2, s2len), -1, "\n");      \
		status_ = MIN(status_, rc_);                                  \
                                                                              \
		if (invert_encoding_errors)                                   \
			rc_ = assert_negative_as_expr(fn(s4, s5, s4sz), -1,   \
			                              "\n");                  \
		else                                                          \
			rc_ = assert_positive_as_expr(fn(s4, s5, s4sz), -1,   \
			                              "\n");                  \
		status_ = MIN(status_, rc_);                                  \
                                                                              \
		if (status_ < 0)                                              \
			printf("%s failed\n", #fn);                           \
		status_;                                                      \
	})

static int
test_strncmp(void)
{
	int status = 0;
	int rc;

	/*
	 * shim's strncmp
	 */
	rc = test_strncmp_helper(shim_strncmp, true, false, false);
	status = MIN(rc, status);

	/*
	 * libc's strncmp
	 */
	/*
	 * Deliberately not testing the difference between these two
	 * comparisons for the symbol named "strncmp":
	 *   strncmp("b", "a", 1)
	 *   strncmp("c", "a", 1)
	 * glibc, shim_strncmp(), and even the gnuefi ones will give you 1
	 * and 2, respectively, as will glibc's, but valgrind swaps in its
	 * own implementation, in case you're doing something that's both
	 * clever and dumb with the result, and it'll return 1 for both of
	 * them.
	 */
	rc = test_strncmp_helper(strncmp, false, false, false);
	status = MIN(rc, status);

	/*
	 * gnu-efi's broken strncmpa
	 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wsign-compare"
	rc = test_strncmp_helper(gnuefi_strncmp, true, true, false);
	status = MIN(rc, status);
#pragma GCC diagnostic pop

	/*
	 * This is still broken, and fails the test case as written on arm.
	 * We no longer use this, so we do not strictly need to run it.
	 */
#if !defined(__arm__) && !defined(__aarch64__)
	/*
	 * gnu-efi's broken strncmpa with the return type fixed
	 */
	rc = test_strncmp_helper(gnuefi_signed_strncmp, true, false, true);
	status = MIN(rc, status);
#endif

	/*
	 * gnu-efi's strncmpa with the return type fixed and unsigned
	 * comparisons internally
	 */
	rc = test_strncmp_helper(gnuefi_good_strncmp, true, false, false);
	status = MIN(rc, status);

	return status;
}

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
#undef s4
#undef s4sz
#undef s4len
#undef s5
#undef s5sz
#undef s5len

/*
 * Put -Wshadow back how it was
 */
#pragma GCC diagnostic pop

static int
test_strchr(void)
{
	char s0[] = "abcbdbeb\0fbgb";

	assert_equal_return(strchr(s0, 'a'), s0, -1, "got %p expected %p\n");
	assert_equal_return(strchr(s0, 'b'), &s0[1], -1, "got %p expected %p\n");
	assert_equal_return(strchr(&s0[1], 'b'), &s0[1], -1, "got %p expected %p\n");
	assert_equal_return(strchr(&s0[2], 'b'), &s0[3], -1, "got %p expected %p\n");
	assert_equal_return(strchr(&s0[4], 'b'), &s0[5], -1, "got %p expected %p\n");
	assert_equal_return(strchr(&s0[6], 'b'), &s0[7], -1, "got %p expected %p\n");
	assert_equal_return(strchr(&s0[8], 'b'), NULL, -1, "got %p expected %p\n");

	assert_equal_return(shim_strchr(s0, 'a'), s0, -1, "got %p expected %p\n");
	assert_equal_return(shim_strchr(s0, 'b'), &s0[1], -1, "got %p expected %p\n");
	assert_equal_return(shim_strchr(&s0[1], 'b'), &s0[1], -1, "got %p expected %p\n");
	assert_equal_return(shim_strchr(&s0[2], 'b'), &s0[3], -1, "got %p expected %p\n");
	assert_equal_return(shim_strchr(&s0[4], 'b'), &s0[5], -1, "got %p expected %p\n");
	assert_equal_return(shim_strchr(&s0[6], 'b'), &s0[7], -1, "got %p expected %p\n");
	assert_equal_return(shim_strchr(&s0[8], 'b'), NULL, -1, "got %p expected %p\n");
	return 0;
}

static int
test_stpcpy(void)
{
	char s0[] = "0123456789abcdef";
	char s1[] = "xxxxxxxxxxxxxxxx";
	char *s;

	s0[0xa] = 0;
	assert_equal_return(stpcpy(s1, s0), &s1[0xa], -1, "got %p expected %p\n");
	assert_zero_return(memcmp(s0, s1, 11), -1, "\n");
	assert_zero_return(memcmp(&s1[11], "xxxxx", sizeof("xxxxx")), -1, "\n");

	memset(s1, 'x', sizeof(s1));
	s1[16] = 0;
	assert_equal_return(shim_stpcpy(s1, s0), &s1[0xa], -1, "got %p expected %p\n");
	assert_zero_return(memcmp(s0, s1, 11), -1, "\n");
	assert_zero_return(memcmp(&s1[11], "xxxxx", sizeof("xxxxx")), -1, "\n");

	return 0;
}

static int
test_strdup(void)
{
	char s0[] = "0123456789abcdef";
	char *s = NULL;

	s = strdup(s0);
	assert_equal_goto(strcmp(s0, s), 0, err, "\n");
	free(s);

	s = shim_strdup(s0);
	assert_equal_goto(strcmp(s0, s), 0, err, "\n");
	free(s);

	return 0;
err:
	if (s)
		free(s);
	return -1;
}

static int
test_strndup(void)
{
	char s0[] = "0123456789abcdef";
	char *s = NULL;

	s = strndup(s0, 18);
	assert_equal_goto(strcmp(s0, s), 0, err, "\n");
	free(s);
	s = strndup(s0, 15);
	assert_positive_goto(strcmp(s0, s), err, "\n");
	free(s);

	s = shim_strndup(s0, 18);
	assert_equal_goto(strcmp(s0, s), 0, err, "\n");
	free(s);
	s = strndup(s0, 15);
	assert_positive_goto(shim_strcmp(s0, s), err, "\n");
	free(s);

	return 0;
err:
	if (s)
		free(s);
	return -1;
}

static int
test_strchrnul_helper(__typeof__(strchrnul) fn)
{
	const char s0[] = "abcd\0fghi";

	assert_equal_return(fn(s0, 'a'), &s0[0], -1, "got %p expected %p\n");
	assert_equal_return(fn(s0, 'd'), &s0[3], -1, "got %p expected %p\n");
	assert_equal_return(fn(s0, '\000'), &s0[4], -1, "got %p expected %p\n");
	assert_equal_return(fn(s0, 'i'), &s0[4], -1, "got %p expected %p\n");

	return 0;
}

static int
test_strchrnul(void)
{
	const char s0[] = "abcd\0fghi";

	assert_equal_return(test_strchrnul_helper(shim_strchrnul),
			    test_strchrnul_helper(strchrnul),
			    -1, "got %d expected %d\n");

	assert_equal_return(strnchrnul(s0, 0, 'b'), &s0[0], -1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, -1, 'b'), &s0[1], 1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, 2, 'b'), &s0[1], -1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, 4, 'f'), &s0[3], -1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, 5, 'f'), &s0[4], -1, "got %p expected %p\n");
	assert_equal_return(strnchrnul(s0, 8, 'f'), &s0[4], -1, "got %p expected %p\n");

	assert_equal_return(strnchrnul(&s0[4], 1, 'f'), &s0[4], -1, "got %p expected %p\n");

	return 0;
}

static int
test_strrchr(void) {
	char s0[] = "abcbebfb";

	assert_equal_return(shim_strrchr(s0, '\n'), NULL, -1, "got %p expected %p\n");
	assert_equal_return(strrchr(s0, '\n'), NULL, -1, "got %p expected %p\n");
	assert_equal_return(shim_strrchr(s0, 'b'), &s0[7], -1, "got %p expected %p\n");
	assert_equal_return(strrchr(s0, 'b'), &s0[7], -1, "got %p expected %p\n");
	assert_equal_return(shim_strrchr(s0, 'c'), &s0[2], -1, "got %p expected %p\n");
	assert_equal_return(strrchr(s0, 'c'), &s0[2], -1, "got %p expected %p\n");

	return 0;
}

static int
test_strcpy(void)
{
	char s0[] = "0123456789abcdef\0000";
	char s1[sizeof(s0)];

	memset(s1, 0, sizeof(s1));
	assert_equal_return(strcpy(s1, s0), s1, -1, "got %p expected %p\n");
	assert_equal_return(strlen(s0), strlen(s1), -1, "got %d expected %d\n");

	memset(s1, 0, sizeof(s1));
	assert_equal_return(shim_strcpy(s1, s0), s1, -1, "got %p expected %p\n");
	assert_equal_return(strlen(s0), strlen(s1), -1, "got %d expected %d\n");

	memset(s1, 0, sizeof(s1));
	assert_equal_return(shim_strcpy(s1, s0), s1, -1, "got %p expected %p\n");
	assert_equal_return(strlen(s0), strlen(s1), -1, "got %d expected %d\n");

	return 0;
}

static int
test_strncpy(void)
{
	char s[] = "0123456789abcdef\0000";
	char s0[4096];
	char s1[4096];

	memset(s0, 0, sizeof(s0));
	memcpy(s0, s, sizeof(s));
#if __GNUC_PREREQ(8, 1)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
	memset(s1, 0, 4096);
	assert_equal_return(strncpy(s1, s0, 0), s1, -1, "got %p expected %p\n");
	assert_equal_return(strlen(s1), 0, -1, "got %d expected %d\n");
	memset(s1, 0, 4096);
	assert_equal_return(shim_strncpy(s1, s0, 0), s1, -1, "got %p expected %p\n");
	assert_equal_return(strlen(s1), 0, -1, "got %d expected %d\n");

	memset(s1, 0, 4096);
	assert_equal_return(strncpy(s1, s0, 1), s1, -1, "got %p expected %p\n");
	assert_equal_return(strlen(s1), 1, -1, "got %d expected %d\n");
	assert_equal_return(s1[0], s0[0], -1, "got %#02hhx, expected %#02hhx\n");
	assert_equal_return(s0[1], '1', -1, "got %#02hhx, expected %#02hhx\n");
	assert_equal_return(s1[1], '\0', -1, "got %#02hhx, expected %#02hhx\n");
	memset(s1, 0, 4096);
	assert_equal_return(shim_strncpy(s1, s0, 1), s1, -1, "got %p expected %p\n");
	assert_equal_return(strlen(s1), 1, -1, "got %d expected %d\n");
	assert_equal_return(s1[0], s0[0], -1, "got %#02hhx, expected %#02hhx\n");
	assert_equal_return(s0[1], '1', -1, "got %#02hhx, expected %#02hhx\n");
	assert_equal_return(s1[1], '\0', -1, "got %#02hhx, expected %#02hhx\n");

	memset(s1, 0, 4096);
	assert_equal_return(strncpy(s1, s0, 15), s1, -1, "got %p expected %p\n");
	assert_equal_return(s0[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[17], '0', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[18], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[15], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[17], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[18], '\000', -1, "got %#02hhx expected %02hhx\n");
	memset(s1, 0, 4096);
	assert_equal_return(shim_strncpy(s1, s0, 15), s1, -1, "got %p expected %p\n");
	assert_equal_return(s0[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[17], '0', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[18], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[15], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[17], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[18], '\000', -1, "got %#02hhx expected %02hhx\n");

	memset(s1, 0, 4096);
	assert_equal_return(strncpy(s1, s0, 16), s1, -1, "got %p expected %p\n");
	assert_equal_return(s0[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[17], '0', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[18], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[17], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[18], '\000', -1, "got %#02hhx expected %02hhx\n");
	memset(s1, 0, 4096);
	assert_equal_return(shim_strncpy(s1, s0, 16), s1, -1, "got %p expected %p\n");
	assert_equal_return(s0[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[17], '0', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[18], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[17], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[18], '\000', -1, "got %#02hhx expected %02hhx\n");

	memset(s1, 0, 4096);
	s1[17] = '0';
	s1[18] = '1';
	assert_equal_return(strncpy(s1, s0, 4096), s1, -1, "got %p expected %p\n");
	assert_equal_return(s0[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[17], '0', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[18], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[17], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[18], '\000', -1, "got %#02hhx expected %02hhx\n");
	memset(s1, 0, 4096);
	s1[17] = '0';
	s1[18] = '1';
	assert_equal_return(shim_strncpy(s1, s0, 4096), s1, -1, "got %p expected %p\n");
	assert_equal_return(s0[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[17], '0', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s0[18], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[14], 'e', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[15], 'f', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[16], '\000', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[17], '0', -1, "got %#02hhx expected %02hhx\n");
	assert_equal_return(s1[18], '1', -1, "got %#02hhx expected %02hhx\n");
#if __GNUC_PREREQ(8, 1)
# pragma GCC diagnostic pop
#endif
	return 0;
}

static int
test_strcat(void)
{
	char s[] = "0123456789abcdef\0000";
	char s0[4096];
	char s1[4096];
	char *s2;
	char s3[] = "0123456789abcdef0123456789abcdef\000\000\000\000\000";

	memset(s0, 0, sizeof(s0));
	memcpy(s0, s, sizeof(s));

	memset(s1, 0, 4096);
	assert_equal_return(strcat(s1, s0), s1, -1, "got %p expected %p\n");
	/* For unknown reasons, gcc 4.8.5 gives us this here:
	 * | In file included from shim.h:64:0,
	 * |                  from test-str.c:14:
	 * | test-str.c: In function 'test_strcat':
	 * | include/test.h:85:10: warning: array subscript is below array bounds [-Warray-bounds]
	 * |     printf("%s:%d:got %lld, expected zero " fmt, __func__, \
	 * |           ^
	 * | include/test.h:112:10: warning: array subscript is below array bounds [-Warray-bounds]
	 * |     printf("%s:%d:got %lld, expected < 0 " fmt, __func__, \
	 * |           ^
	 *
	 * This clearly isn't a useful error message, as it doesn't tell us
	 * /anything about the problem/, but also it isn't reported on
	 * later compilers, and it isn't clear that there's any problem
	 * when examining these functions.
	 *
	 * I don't know.
	 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
	assert_zero_return(strncmp(s1, s0, sizeof(s)-1), 0, -1, "\n");
	assert_negative_return(memcmp(s1, s0, sizeof(s)), 0, -1, "\n");
#pragma GCC diagnostic pop

	memset(s1, 0, 4096);
	assert_equal_return(strcat(s1, s0), s1, -1, "got %p expected %p\n");
	s2 = s1 + strlen(s1);
	assert_equal_return(s2, &s1[16], -1, "got %p expected %p\n");
	assert_equal_return(strcat(s2, s0), s2, -1, "got %p expected %p\n");
	assert_zero_return(strncmp(s1, s0, strlen(s)), -1, "got %p expected %p\n");
	assert_zero_return(strncmp(s2, s0, 2*(sizeof(s)-1)), -1, "\n");
	assert_positive_return(memcmp(s1, s0, 2*sizeof(s)-2), -1, "\n");
	assert_equal_return(memcmp(s1, s3, sizeof(s3)), 0,  -1, "expected %d got %d\n");

	return 0;
}

static int
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

static int
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

static int
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

static int
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

static int
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

static int
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

static int
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
	test(test_strlen);
	test(test_strnlen);
	test(test_strcmp);
	test(test_strncmp);
	test(test_strcasecmp);
	test(test_strncasecmp);
	test(test_strrchr);
	test(test_strcpy);
	test(test_strncpy);
	test(test_strcat);
	test(test_stpcpy);
	test(test_strdup);
	test(test_strndup);
	test(test_strchr);
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
