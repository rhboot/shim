// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test.h - fake a bunch of EFI types so we can build test harnesses with libc
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifdef SHIM_UNIT_TEST
#ifndef TEST_H_
#define TEST_H_

#define _GNU_SOURCE

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#if defined(__aarch64__)
#include <aarch64/efibind.h>
#elif defined(__arm__)
#include <arm/efibind.h>
#elif defined(__i386__) || defined(__i486__) || defined(__i686__)
#include <ia32/efibind.h>
#elif defined(__x86_64__)
#include <x86_64/efibind.h>
#else
#error what arch is this
#endif

#include <efidef.h>

#include <efidevp.h>
#include <efiprot.h>
#include <eficon.h>
#include <efiapi.h>
#include <efierr.h>

#include <efipxebc.h>
#include <efinet.h>
#include <efiip.h>

#include <stdlib.h>

#include <assert.h>

#include <efiui.h>
#include <efilib.h>

#include <efivar/efivar.h>

#include <assert.h>

#include "list.h"

INTN StrCmp(IN CONST CHAR16 *s1,
	    IN CONST CHAR16 *s2);
INTN StrnCmp(IN CONST CHAR16 *s1,
	     IN CONST CHAR16 *s2,
	     IN UINTN len);
VOID StrCpy(IN CHAR16 *Dest,
	    IN CONST CHAR16 *Src);
VOID StrnCpy(IN CHAR16 *Dest,
	     IN CONST CHAR16 *Src,
	     IN UINTN Len);
CHAR16 *StrDuplicate(IN CONST CHAR16 *Src);
UINTN StrLen(IN CONST CHAR16 *s1);
UINTN StrSize(IN CONST CHAR16 *s1);
VOID StrCat(IN CHAR16 *Dest, IN CONST CHAR16 *Src);
CHAR16 *DevicePathToStr(EFI_DEVICE_PATH *DevPath);

extern EFI_SYSTEM_TABLE *ST;
extern EFI_BOOT_SERVICES *BS;
extern EFI_RUNTIME_SERVICES *RT;

#define GUID_FMT "%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"
#define GUID_ARGS(guid) \
	((EFI_GUID)guid).Data1, ((EFI_GUID)guid).Data2, ((EFI_GUID)guid).Data3, \
	((EFI_GUID)guid).Data4[1], ((EFI_GUID)guid).Data4[0], \
	((EFI_GUID)guid).Data4[2], ((EFI_GUID)guid).Data4[3], \
	((EFI_GUID)guid).Data4[4], ((EFI_GUID)guid).Data4[5], \
	((EFI_GUID)guid).Data4[6], ((EFI_GUID)guid).Data4[7]

static inline INT64
guidcmp_helper(const EFI_GUID * const guid0, const EFI_GUID * const guid1)
{
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s(): Comparing "GUID_FMT" to "GUID_FMT"\n",
	       __FILE__, __LINE__-1, __func__,
	       GUID_ARGS(*guid0), GUID_ARGS(*guid1));
#endif

	if (guid0->Data1 != guid1->Data1) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s(): returning 0x%"PRIx64"-0x%"PRIx64"->0x%"PRIx64"\n",
		       __FILE__, __LINE__-1, __func__,
		       (INT64)guid0->Data1, (INT64)guid1->Data1,
		       (INT64)guid0->Data1 - (INT64)guid1->Data1);
#endif
		return (INT64)guid0->Data1 - (INT64)guid1->Data1;
	}

	if (guid0->Data2 != guid1->Data2) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s(): returning 0x%"PRIx64"-0x%"PRIx64"->0x%"PRIx64"\n",
		       __FILE__, __LINE__-1, __func__,
		       (INT64)guid0->Data2, (INT64)guid1->Data2,
		       (INT64)guid0->Data2 - (INT64)guid1->Data2);
#endif
		return (INT64)guid0->Data2 - (INT64)guid1->Data2;
	}

	if (guid0->Data3 != guid1->Data3) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s(): returning 0x%"PRIx64"-0x%"PRIx64"->0x%"PRIx64"\n",
		       __FILE__, __LINE__-1, __func__,
		       (INT64)guid0->Data3, (INT64)guid1->Data3,
		       (INT64)guid0->Data3 - (INT64)guid1->Data3);
#endif
		return (INT64)guid0->Data3 - (INT64)guid1->Data3;
	}

	/*
	 * This is out of order because that's also true in the string
	 * representation of it.
	 */
	if (guid0->Data4[1] != guid1->Data4[1]) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s(): returning 0x%"PRIx64"-0x%"PRIx64"->0x%"PRIx64"\n",
		       __FILE__, __LINE__-1, __func__,
		       (INT64)guid0->Data4[1], (INT64)guid1->Data4[1],
		       (INT64)guid0->Data4[1] - (INT64)guid1->Data4[1]);
#endif
		return (INT64)guid0->Data4[1] - (INT64)guid1->Data4[1];
	}

	if (guid0->Data4[0] != guid1->Data4[0]) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s(): returning 0x%"PRIx64"-0x%"PRIx64"->0x%"PRIx64"\n",
		       __FILE__, __LINE__-1, __func__,
		       (INT64)guid0->Data4[0], (INT64)guid1->Data4[0],
		       (INT64)guid0->Data4[0] - (INT64)guid1->Data4[0]);
#endif
		return (INT64)guid0->Data4[0] - (INT64)guid1->Data4[0];
	}

	for (UINTN i = 2; i < 8; i++) {
		if (guid0->Data4[i] != guid1->Data4[i]) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
			printf("%s:%d:%s(): returning 0x%"PRIx64"-0x%"PRIx64"->0x%"PRIx64"\n",
			       __FILE__, __LINE__-1, __func__,
			       (INT64)guid0->Data4[i], (INT64)guid1->Data4[i],
			       (INT64)guid0->Data4[i] - (INT64)guid1->Data4[i]);
#endif
			return (INT64)guid0->Data4[i] - (INT64)guid1->Data4[i];
		}
	}

#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s(): returning 0x0\n",
	       __FILE__, __LINE__-1, __func__);
#endif
	return 0;
}

static inline int
guidcmp(const EFI_GUID * const guid0, const EFI_GUID * const guid1)
{
	INT64 cmp;
	int ret;
	EFI_GUID empty;
	const EFI_GUID * const guida = guid0 ? guid0 : &empty;
	const EFI_GUID * const guidb = guid1 ? guid1 : &empty;

	SetMem(&empty, sizeof(EFI_GUID), 0);

	cmp = guidcmp_helper(guida, guidb);
	ret = cmp < 0 ? -1 : (cmp > 0 ? 1 : 0);
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():CompareGuid("GUID_FMT","GUID_FMT")->%lld (%d)\n",
	       __FILE__, __LINE__-1, __func__,
	       GUID_ARGS(*guida), GUID_ARGS(*guidb), cmp, ret);
#endif
	return ret;
}

#define CompareGuid(a, b) guidcmp(a, b)

static inline char *
efi_strerror(EFI_STATUS status)
{
	static char buf0[1024];
	static char buf1[1024];
	char *out;
	static int n;

	out = n++ % 2 ? buf0 : buf1;
	if (n > 1)
		n -= 2;
	SetMem(out, 1024, 0);

	switch (status) {
	case EFI_SUCCESS:
		strcpy(out, "EFI_SUCCESS");
		break;
	case EFI_LOAD_ERROR:
		strcpy(out, "EFI_LOAD_ERROR");
		break;
	case EFI_INVALID_PARAMETER:
		strcpy(out, "EFI_INVALID_PARAMETER");
		break;
	case EFI_UNSUPPORTED:
		strcpy(out, "EFI_UNSUPPORTED");
		break;
	case EFI_BAD_BUFFER_SIZE:
		strcpy(out, "EFI_BAD_BUFFER_SIZE");
		break;
	case EFI_BUFFER_TOO_SMALL:
		strcpy(out, "EFI_BUFFER_TOO_SMALL");
		break;
	case EFI_NOT_READY:
		strcpy(out, "EFI_NOT_READY");
		break;
	case EFI_DEVICE_ERROR:
		strcpy(out, "EFI_DEVICE_ERROR");
		break;
	case EFI_WRITE_PROTECTED:
		strcpy(out, "EFI_WRITE_PROTECTED");
		break;
	case EFI_OUT_OF_RESOURCES:
		strcpy(out, "EFI_OUT_OF_RESOURCES");
		break;
	case EFI_VOLUME_CORRUPTED:
		strcpy(out, "EFI_VOLUME_CORRUPTED");
		break;
	case EFI_VOLUME_FULL:
		strcpy(out, "EFI_VOLUME_FULL");
		break;
	case EFI_NO_MEDIA:
		strcpy(out, "EFI_NO_MEDIA");
		break;
	case EFI_MEDIA_CHANGED:
		strcpy(out, "EFI_MEDIA_CHANGED");
		break;
	case EFI_NOT_FOUND:
		strcpy(out, "EFI_NOT_FOUND");
		break;
	case EFI_ACCESS_DENIED:
		strcpy(out, "EFI_ACCESS_DENIED");
		break;
	case EFI_NO_RESPONSE:
		strcpy(out, "EFI_NO_RESPONSE");
		break;
	case EFI_NO_MAPPING:
		strcpy(out, "EFI_NO_MAPPING");
		break;
	case EFI_TIMEOUT:
		strcpy(out, "EFI_TIMEOUT");
		break;
	case EFI_NOT_STARTED:
		strcpy(out, "EFI_NOT_STARTED");
		break;
	case EFI_ALREADY_STARTED:
		strcpy(out, "EFI_ALREADY_STARTED");
		break;
	case EFI_ABORTED:
		strcpy(out, "EFI_ABORTED");
		break;
	case EFI_ICMP_ERROR:
		strcpy(out, "EFI_ICMP_ERROR");
		break;
	case EFI_TFTP_ERROR:
		strcpy(out, "EFI_TFTP_ERROR");
		break;
	case EFI_PROTOCOL_ERROR:
		strcpy(out, "EFI_PROTOCOL_ERROR");
		break;
	case EFI_INCOMPATIBLE_VERSION:
		strcpy(out, "EFI_INCOMPATIBLE_VERSION");
		break;
	case EFI_SECURITY_VIOLATION:
		strcpy(out, "EFI_SECURITY_VIOLATION");
		break;
	case EFI_CRC_ERROR:
		strcpy(out, "EFI_CRC_ERROR");
		break;
	case EFI_END_OF_MEDIA:
		strcpy(out, "EFI_END_OF_MEDIA");
		break;
	case EFI_END_OF_FILE:
		strcpy(out, "EFI_END_OF_FILE");
		break;
	case EFI_INVALID_LANGUAGE:
		strcpy(out, "EFI_INVALID_LANGUAGE");
		break;
	case EFI_COMPROMISED_DATA:
		strcpy(out, "EFI_COMPROMISED_DATA");
		break;
	default:
		sprintf(out, "0x%lx", status);
		break;
	}
	return out;
}

static inline char *
Str2str(CHAR16 *in)
{
	static char buf0[1024];
	static char buf1[1024];
	char *out;
	static int n;

	out = n++ % 2 ? buf0 : buf1;
	if (n > 1)
		n -= 2;
	SetMem(out, 1024, 0);
	for (UINTN i = 0; i < 1023 && in[i]; i++)
		out[i] = in[i];
	return out;
}

static inline char *
format_var_attrs(UINT32 attr)
{
	static char buf0[1024];
	static char buf1[1024];
	static int n;
	int pos = 0;
	bool found = false;
	char *buf, *bufp;

	buf = n++ % 2 ? buf0 : buf1;
	if (n > 1)
		n -= 2;
	SetMem(buf, sizeof(buf0), 0);
	bufp = &buf[0];
	for (UINT32 i = 0; i < 8; i++) {
		switch((1ul << i) & attr) {
		case EFI_VARIABLE_NON_VOLATILE:
			if (found)
				bufp = stpcpy(bufp, "|");
			bufp = stpcpy(bufp, "NV");
			attr &= ~EFI_VARIABLE_NON_VOLATILE;
			found = true;
			break;
		case EFI_VARIABLE_BOOTSERVICE_ACCESS:
			if (found)
				bufp = stpcpy(bufp, "|");
			bufp = stpcpy(bufp, "BS");
			attr &= ~EFI_VARIABLE_BOOTSERVICE_ACCESS;
			found = true;
			break;
		case EFI_VARIABLE_RUNTIME_ACCESS:
			if (found)
				bufp = stpcpy(bufp, "|");
			bufp = stpcpy(bufp, "RT");
			attr &= ~EFI_VARIABLE_RUNTIME_ACCESS;
			found = true;
			break;
		case EFI_VARIABLE_HARDWARE_ERROR_RECORD:
			if (found)
				bufp = stpcpy(bufp, "|");
			bufp = stpcpy(bufp, "HER");
			attr &= ~EFI_VARIABLE_HARDWARE_ERROR_RECORD;
			found = true;
			break;
		case EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS:
			if (found)
				bufp = stpcpy(bufp, "|");
			bufp = stpcpy(bufp, "AUTH");
			attr &= ~EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
			found = true;
			break;
		case EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS:
			if (found)
				bufp = stpcpy(bufp, "|");
			bufp = stpcpy(bufp, "TIMEAUTH");
			attr &= ~EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
			found = true;
			break;
		case EFI_VARIABLE_APPEND_WRITE:
			if (found)
				bufp = stpcpy(bufp, "|");
			bufp = stpcpy(bufp, "APPEND");
			attr &= ~EFI_VARIABLE_APPEND_WRITE;
			found = true;
			break;
		case EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS:
			if (found)
				bufp = stpcpy(bufp, "|");
			bufp = stpcpy(bufp, "ENHANCED_AUTH");
			attr &= ~EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS;
			found = true;
			break;
		default:
			break;
		}
	}
	if (attr) {
		if (found)
			bufp = stpcpy(bufp, "|");
		snprintf(bufp, bufp - buf - 1, "0x%x", attr);
	}
	return &buf[0];
}

extern int debug;
#ifdef dprint
#undef dprint
#define dprint(fmt, ...) {( if (debug) printf("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); })
#endif

void EFIAPI mock_efi_void();
EFI_STATUS EFIAPI mock_efi_success();
EFI_STATUS EFIAPI mock_efi_unsupported();
EFI_STATUS EFIAPI mock_efi_not_found();
void init_efi_system_table(void);
void reset_efi_system_table(void);
void print_traceback(int skip);

#define eassert(cond, fmt, ...)                                  \
	({                                                       \
		if (!(cond)) {                                   \
			printf("%s:%d:" fmt, __func__, __LINE__, \
			       ##__VA_ARGS__);                   \
		}                                                \
		assert(cond);                                    \
	})

#define assert_true_as_expr(a, status, fmt, ...)                              \
	({                                                                    \
		__typeof__(status) rc_ = 0;                                   \
		if (!(a)) {                                                   \
			printf("%s:%d:got %lld, expected nonzero " fmt,       \
			       __func__, __LINE__, (long long)(uintptr_t)(a), \
			       ##__VA_ARGS__);                                \
			printf("%s:%d:Assertion `%s' failed.\n", __func__,    \
			       __LINE__, __stringify(!(a)));                  \
			rc_ = status;                                         \
		}                                                             \
		rc_;                                                          \
	})
#define assert_nonzero_as_expr(a, ...) assert_true_as_expr(a, ##__VA_ARGS__)

#define assert_false_as_expr(a, status, fmt, ...)                              \
	({                                                                     \
		__typeof__(status) rc_ = (__typeof__(status))0;                \
		if (a) {                                                       \
			printf("%s:%d:got %lld, expected zero " fmt, __func__, \
			       __LINE__, (long long)(a), ##__VA_ARGS__);       \
			printf("%s:%d:Assertion `%s' failed.\n", __func__,     \
			       __LINE__, __stringify(a));                      \
			rc_ = status;                                          \
		}                                                              \
		rc_;                                                           \
	})
#define assert_zero_as_expr(a, ...) assert_false_as_expr(a, ##__VA_ARGS__)

#define assert_positive_as_expr(a, status, fmt, ...)                          \
	({                                                                    \
		__typeof__(status) rc_ = (__typeof__(status))0;               \
		if ((a) <= 0) {                                               \
			printf("%s:%d:got %lld, expected > 0 " fmt, __func__, \
			       __LINE__, (long long)(a), ##__VA_ARGS__);      \
			printf("%s:%d:Assertion `%s' failed.\n", __func__,    \
			       __LINE__, __stringify((a) <= 0));              \
			rc_ = status;                                         \
		}                                                             \
		rc_;                                                          \
	})

#define assert_negative_as_expr(a, status, fmt, ...)                          \
	({                                                                    \
		__typeof__(status) rc_ = (__typeof__(status))0;               \
		if ((a) >= 0) {                                               \
			printf("%s:%d:got %lld, expected < 0 " fmt, __func__, \
			       __LINE__, (long long)(a), ##__VA_ARGS__);      \
			printf("%s:%d:Assertion `%s' failed.\n", __func__,    \
			       __LINE__, __stringify((a) >= 0));              \
			rc_ = status;                                         \
		}                                                             \
		rc_;                                                          \
	})

#define assert_equal_as_expr(a, b, status, fmt, ...)                       \
	({                                                                 \
		__typeof__(status) rc_ = (__typeof__(status))0;            \
		if (!((a) == (b))) {                                       \
			printf("%s:%d:" fmt, __func__, __LINE__, (a), (b), \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(a == b));             \
			rc_ = status;                                      \
		}                                                          \
		rc_;                                                       \
	})

#define assert_not_equal_as_expr(a, b, status, fmt, ...)                   \
	({                                                                 \
		__typeof__(status) rc_ = (__typeof__(status))0;            \
		if (!((a) != (b))) {                                       \
			printf("%s:%d:" fmt, __func__, __LINE__, (a), (b), \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(a != b));             \
			rc_ = status;                                      \
		}                                                          \
		rc_;                                                       \
	})

#define assert_as_expr(cond, status, fmt, ...)                             \
	({                                                                 \
		__typeof__(status) rc_ = (__typeof__(status))0;            \
		if (!(cond)) {                                             \
			printf("%s:%d:" fmt, __func__, __LINE__,           \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(cond));               \
			rc_ = status;                                      \
		}                                                          \
		rc_;                                                       \
	})

#define assert_true_return(a, status, fmt, ...)                             \
	({                                                                  \
		__typeof__(status) rc_ =                                    \
			assert_true_as_expr(a, status, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                               \
			return rc_;                                         \
	})
#define assert_nonzero_return(a, ...) assert_true_return(a, ##__VA_ARGS__)

#define assert_false_return(a, status, fmt, ...)                             \
	({                                                                   \
		__typeof__(status) rc_ =                                     \
			assert_false_as_expr(a, status, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                                \
			return rc_;                                          \
	})
#define assert_zero_return(a, ...) assert_false_return(a, ##__VA_ARGS__)

#define assert_positive_return(a, status, fmt, ...)               \
	({                                                        \
		__typeof__(status) rc_ = assert_positive_as_expr( \
			a, status, fmt, ##__VA_ARGS__);           \
		if (rc_ != 0)                                     \
			return rc_;                               \
	})

#define assert_negative_return(a, status, fmt, ...)               \
	({                                                        \
		__typeof__(status) rc_ = assert_negative_as_expr( \
			a, status, fmt, ##__VA_ARGS__);           \
		if (rc_ != 0)                                     \
			return rc_;                               \
	})

#define assert_equal_return(a, b, status, fmt, ...)            \
	({                                                     \
		__typeof__(status) rc_ = assert_equal_as_expr( \
			a, b, status, fmt, ##__VA_ARGS__);     \
		if (rc_ != 0)                                  \
			return rc_;                            \
	})

#define assert_not_equal_return(a, b, status, fmt, ...)            \
	({                                                         \
		__typeof__(status) rc_ = assert_not_equal_as_expr( \
			a, b, status, fmt, ##__VA_ARGS__);         \
		if (rc_ != 0)                                      \
			return rc_;                                \
	})

#define assert_return(cond, status, fmt, ...)                             \
	({                                                                \
		__typeof__(status) rc_ =                                  \
			assert_as_expr(cond, status, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                             \
			return rc_;                                       \
	})

#define assert_goto(cond, label, fmt, ...)                                 \
	({                                                                 \
		if (!(cond)) {                                             \
			printf("%s:%d:" fmt, __func__, __LINE__,           \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(cond));               \
			goto label;                                        \
		}                                                          \
	})

#define assert_equal_goto(a, b, label, fmt, ...)                           \
	({                                                                 \
		if (!((a) == (b))) {                                       \
			printf("%s:%d:" fmt, __func__, __LINE__, (a), (b), \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(a == b));             \
			goto label;                                        \
		}                                                          \
	})

#define assert_not_equal_goto(a, b, label, fmt, ...)                       \
	({                                                                 \
		if (!((a) != (b))) {                                       \
			printf("%s:%d:" fmt, __func__, __LINE__, (a), (b), \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(a != b));             \
			goto label;                                        \
		}                                                          \
	})

#define assert_true_goto(a, label, fmt, ...)                               \
	({                                                                 \
		if (!(a)) {                                                \
			printf("%s:%d:" fmt, __func__, __LINE__, (a),      \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(!(a)));               \
			goto label;                                        \
		}                                                          \
	})
#define assert_nonzero_goto(a, ...) assert_true_goto(a, ##__VA_ARGS__)

#define assert_false_goto(a, label, fmt, ...)                              \
	({                                                                 \
		if (a) {                                                   \
			printf("%s:%d:" fmt, __func__, __LINE__, (a),      \
			       ##__VA_ARGS__);                             \
			printf("%s:%d:Assertion `%s' failed.\n", __func__, \
			       __LINE__, __stringify(a));                  \
			goto label;                                        \
		}                                                          \
	})
#define assert_zero_goto(a, ...) assert_false_goto(a, ##__VA_ARGS__)

#define assert_negative_goto(a, label, fmt, ...)                              \
	({                                                                    \
		int rc_ = assert_negative_as_expr(a, -1, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                                 \
			goto label;                                           \
	})

#define assert_positive_goto(a, label, fmt, ...)                              \
	({                                                                    \
		int rc_ = assert_positive_as_expr(a, -1, fmt, ##__VA_ARGS__); \
		if (rc_ != 0)                                                 \
			goto label;                                           \
	})

#define test(x, ...)                                    \
	({                                              \
		int rc;                                 \
		printf("running %s\n", __stringify(x)); \
		rc = x(__VA_ARGS__);                    \
		if (rc < 0)                             \
			status = 1;                     \
		printf("%s: %s\n", __stringify(x),      \
		       rc < 0 ? "failed" : "passed");   \
	})

#endif /* !TEST_H_ */
#endif /* SHIM_UNIT_TEST */
// vim:fenc=utf-8:tw=75:noet
