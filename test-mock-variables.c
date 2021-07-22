// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-mock-variables.c - test our mock variable implementation (irony)
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"
#include "mock-variables.h"

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "test-data-efivars-0.h"

#pragma GCC diagnostic ignored "-Wunused-label"

void mock_print_guidname(EFI_GUID *guid, CHAR16 *name);
void mock_print_var_list(list_t *head);

static int
test_filter_out_helper(size_t nvars, const CHAR16 *varnames[nvars],
		       bool filter_out, UINTN expected_count)
{
	const char *mok_rt_vars[n_mok_state_variables];
	EFI_STATUS status;
	EFI_GUID guid = SHIM_LOCK_GUID;
	CHAR16 name[1024] = L"";
	UINTN sz;
	char asciiname[1024];
	bool found = false;
	int ret = 0;
	UINTN count = 0;

	for (size_t i = 0; i < n_mok_state_variables; i++) {
		mok_rt_vars[i] = mok_state_variables[i].rtname8;
	}

	sz = sizeof(name);
	status = RT->GetNextVariableName(&sz, name, &guid);
	assert_equal_return(status, EFI_NOT_FOUND, -1, "got %lx, expected %lx");

	mock_load_variables("test-data/efivars-1", mok_rt_vars, filter_out);

	while (true) {
		int rc = 0;

		sz = sizeof(name);
		status = RT->GetNextVariableName(&sz, name, &guid);
		if (status == EFI_NOT_FOUND)
			break;
		if (EFI_ERROR(status))
			return -1;

		count += 1;
		SetMem(asciiname, sizeof(asciiname), 0);
		for (UINTN i = 0; i < sizeof(asciiname); i++)
			asciiname[i] = name[i];
		for (UINTN i = 0; varnames[i] != NULL; i++) {
			if (sz == 0 || StrLen(varnames[i]) != sz-1)
				continue;
			if (StrCmp(name, varnames[i]) == 0) {
				found = true;
				if (filter_out) {
					rc = assert_false_as_expr(found, -1,
						"found=%u for undesired variable \"%s\"\n",
						asciiname);
					break;
				}
			}
		}
		if (!filter_out)
			rc = assert_true_as_expr(found, -1,
				"found=%u for undesired variable \"%s\"\n",
				asciiname);
		if (ret >= 0 && rc < 0)
			ret = rc;
	}

	mock_reset_variables();
	assert_equal_return(count, expected_count, -1, "%lu != %lu\n");
	assert_true_return(list_empty(&mock_variables), -1, "%lu != %lu\n");

	return ret;
}

static int
test_filter_out_true(void)
{
	const CHAR16 *varnames[] = {
		L"MokListRT",
		L"MokListXRT",
		L"SbatLevelRT",
		NULL
	};
	size_t nvars = sizeof(varnames) / sizeof(varnames[0]);

	return test_filter_out_helper(nvars, varnames, true, 3);
}

static int
test_filter_out_false(void)
{
	const CHAR16 *varnames[] = {
		L"MokListRT",
		L"MokListXRT",
		L"SbatLevelRT",
		NULL
	};
	size_t nvars = sizeof(varnames) / sizeof(varnames[0]);

	return test_filter_out_helper(nvars, varnames, false, 3);
}

static int
test_gnvn_buf_size_0(void)
{
	UINTN size = 0;
	CHAR16 buf[6] = { 0, };
	EFI_STATUS status;
	EFI_GUID empty_guid = { 0, };
	int ret = -1;

	status = RT->GetNextVariableName(&size, &buf[0], &GV_GUID);
	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "0x%lx != 0x%lx\n");

	size = 1;
	status = RT->GetNextVariableName(&size, &buf[0], &GV_GUID);
	assert_equal_return(status, EFI_NOT_FOUND, -1, "0x%lx != 0x%lx\n");

	status = RT->SetVariable(L"test", &GV_GUID,
				 EFI_VARIABLE_BOOTSERVICE_ACCESS, 5, "test");
	assert_equal_return(status, EFI_SUCCESS, -1, "0x%lx != 0x%lx\n");

	size = 1;
	status = RT->GetNextVariableName(&size, &buf[0], &empty_guid);
	assert_equal_goto(status, EFI_NOT_FOUND, err, "0x%lx != 0x%lx\n");

	size = 1;
	status = RT->GetNextVariableName(&size, &buf[0], &GV_GUID);
	assert_equal_goto(status, EFI_BUFFER_TOO_SMALL, err, "0x%lx != 0x%lx\n");
	assert_equal_goto(size, StrSize(L"test"), err, "%zu != %zu\n");

	size = StrSize(L"test");
	status = RT->GetNextVariableName(&size, &buf[0], &GV_GUID);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");

	status = RT->SetVariable(L"testa", &GV_GUID,
				 EFI_VARIABLE_BOOTSERVICE_ACCESS, 5, "test");
	assert_equal_return(status, EFI_SUCCESS, -1, "0x%lx != 0x%lx\n");

	buf[0] = 0;
	size = 1;
	status = RT->GetNextVariableName(&size, &buf[0], &empty_guid);
	assert_equal_goto(status, EFI_NOT_FOUND, err, "0x%lx != 0x%lx\n");

	size = StrSize(L"test");
	StrCpy(buf, L"test");
	status = RT->GetNextVariableName(&size, &buf[0], &GV_GUID);
	switch (mock_variable_sort_policy) {
	case MOCK_SORT_DESCENDING:
	case MOCK_SORT_PREPEND:
		assert_equal_goto(status, EFI_NOT_FOUND, err, "0x%lx != 0x%lx\n");
		break;
	case MOCK_SORT_APPEND:
	case MOCK_SORT_ASCENDING:
		assert_equal_goto(status, EFI_BUFFER_TOO_SMALL, err, "0x%lx != 0x%lx\n");
		break;
	default:
		break;
	}

	size = StrSize(L"testa");
	StrCpy(buf, L"test");
	status = RT->GetNextVariableName(&size, &buf[0], &GV_GUID);
	switch (mock_variable_sort_policy) {
	case MOCK_SORT_DESCENDING:
	case MOCK_SORT_PREPEND:
		assert_equal_goto(status, EFI_NOT_FOUND, err, "0x%lx != 0x%lx\n");
		break;
	case MOCK_SORT_APPEND:
	case MOCK_SORT_ASCENDING:
		assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");
		break;
	default:
		break;
	}

	ret = 0;
err:
	mock_reset_variables();
	return ret;
}

static int
test_gnvn_helper(char *testvars)
{
	UINTN size = 0;
	CHAR16 buf[8192] = { 0, };
	EFI_STATUS status;
	EFI_GUID empty_guid = { 0, };
	int ret = -1;
	const char *mok_rt_vars[n_mok_state_variables];

	for (size_t i = 0; i < n_mok_state_variables; i++) {
		mok_rt_vars[i] = mok_state_variables[i].rtname8;
	}

	mock_load_variables(testvars, mok_rt_vars, true);

	size = sizeof(buf);
	buf[0] = L'\0';
	status = RT->GetNextVariableName(&size, buf, &GV_GUID);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");

#if defined(SHIM_DEBUG) && SHIM_DEBUG != 0
	dump_mock_variables(__FILE__, __LINE__, __func__);
#endif
	switch (mock_variable_sort_policy) {
	case MOCK_SORT_DESCENDING:
		dump_mock_variables_if_wrong(__FILE__, __LINE__, __func__,
					     &GV_GUID, L"dbxDefault");
		assert_zero_goto(StrCmp(buf, L"dbxDefault"), err, "0x%lx != 0x%lx\n");
		break;
	case MOCK_SORT_ASCENDING:
		dump_mock_variables_if_wrong(__FILE__, __LINE__, __func__,
					     &GV_GUID, L"Boot0000");
		assert_zero_goto(StrCmp(buf, L"Boot0000"), err, "0x%lx != 0x%lx buf:\"%s\"\n",
				 0, Str2str(buf));
		break;
	default:
		break;
	}

	size = sizeof(buf);
	buf[0] = 0;
	status = RT->GetNextVariableName(&size, buf, &EFI_SECURE_BOOT_DB_GUID);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");
	switch (mock_variable_sort_policy) {
	case MOCK_SORT_DESCENDING:
		assert_zero_goto(StrCmp(buf, L"dbx"), err, "0x%lx != 0x%lx\n");
		break;
	case MOCK_SORT_ASCENDING:
		assert_zero_goto(StrCmp(buf, L"db"), err, "0x%lx != 0x%lx\n");
		break;
	default:
		break;
	}

	ret = 0;
err:
	if (ret)
		mock_print_var_list(&mock_variables);
	mock_reset_variables();
	return ret;
}

static int
test_gnvn_0(void)
{
	return test_gnvn_helper("test-data/efivars-0");
}

static int
test_gnvn_1(void)
{
	return test_gnvn_helper("test-data/efivars-1");
}

static int
test_get_variable_0(void)
{
	UINTN size = 0;
	uint8_t buf[8192] = { 0, };
	EFI_STATUS status;
	EFI_GUID empty_guid = { 0, };
	UINT32 attrs = 0;
	int ret = -1;
	int cmp;
	const char *mok_rt_vars[n_mok_state_variables];

	for (size_t i = 0; i < n_mok_state_variables; i++) {
		mok_rt_vars[i] = mok_state_variables[i].rtname8;
	}

	mock_load_variables("test-data/efivars-1", mok_rt_vars, true);

	size = 0;
	status = RT->GetVariable(L"Boot0000", &GV_GUID, &attrs, &size, buf);
	assert_equal_goto(status, EFI_BUFFER_TOO_SMALL, err, "0x%lx != 0x%lx\n");

	size = sizeof(buf);
	status = RT->GetVariable(L"Boot0000", &GV_GUID, &attrs, &size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");
	assert_equal_goto(size, sizeof(test_data_efivars_0_Boot0000), err, "%zu != %zu\n");
	assert_zero_goto(memcmp(buf, test_data_efivars_0_Boot0000, size), err, "%zu != %zu\n");

	ret = 0;
err:
	if (ret)
		mock_print_var_list(&mock_variables);
	mock_reset_variables();
	return ret;
}

static int
test_set_variable_0(void)
{
	UINTN size = 0;
	uint8_t buf[8192] = { 0, };
	EFI_STATUS status;
	EFI_GUID empty_guid = { 0, };
	UINT32 attrs = 0;
	int ret = -1;
	UINT32 bs_rt_nv = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE;

	size = 4;
	strcpy(buf, "foo");
	status = RT->SetVariable(L"tmp", &GV_GUID, bs_rt_nv, size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");

	size = sizeof(buf);
	SetMem(buf, sizeof(buf), 0);
	status = RT->GetVariable(L"tmp", &GV_GUID, &attrs, &size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");
	assert_equal_goto(size, 4, err, "%zu != %zu\n");
	assert_zero_goto(memcmp(buf, "foo", 4), err, "0x%lx != 0x%lx\n");

	size = 5;
	strcpy(buf, "bang");
	status = RT->SetVariable(L"tmp", &GV_GUID,
				 EFI_VARIABLE_NON_VOLATILE |
				 EFI_VARIABLE_BOOTSERVICE_ACCESS |
				 EFI_VARIABLE_RUNTIME_ACCESS,
				 size, buf);
	size = sizeof(buf);
	SetMem(buf, sizeof(buf), 0);
	status = RT->GetVariable(L"tmp", &GV_GUID, &attrs, &size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");
	assert_equal_goto(size, 5, err, "%zu != %zu\n");
	assert_zero_goto(memcmp(buf, "bang", 5), err, "%d != %d\n");

	size = 5;
	strcpy(buf, "foop");
	status = RT->SetVariable(L"tmp", &GV_GUID, bs_rt_nv, size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");

	size = sizeof(buf);
	SetMem(buf, sizeof(buf), 0);
	status = RT->GetVariable(L"tmp", &GV_GUID, &attrs, &size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");
	assert_equal_goto(size, 5, err, "%zu != %zu\n");
	assert_zero_goto(memcmp(buf, "foop", 5), err, "%d != %d\n");

	size = 0;
	strcpy(buf, "");
	status = RT->SetVariable(L"tmp", &GV_GUID, bs_rt_nv | EFI_VARIABLE_APPEND_WRITE, size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");

	size = sizeof(buf);
	SetMem(buf, sizeof(buf), 0);
	status = RT->GetVariable(L"tmp", &GV_GUID, &attrs, &size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");
	assert_equal_goto(size, 5, err, "%zu != %zu\n");
	assert_zero_goto(memcmp(buf, "foop", 5), err, "%d != %d\n");

	size = 5;
	strcpy(buf, "poof");
	status = RT->SetVariable(L"tmp", &GV_GUID, bs_rt_nv | EFI_VARIABLE_APPEND_WRITE, size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");

	size = sizeof(buf);
	SetMem(buf, sizeof(buf), 0);
	status = RT->GetVariable(L"tmp", &GV_GUID, &attrs, &size, buf);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");
	assert_equal_goto(size, 10, err, "%zu != %zu\n");
	assert_zero_goto(memcmp(buf, "foop\0poof", 10), err, "%d != %d\n");
	ret = 0;
err:
	if (ret)
		mock_print_var_list(&mock_variables);
	mock_reset_variables();
	return ret;
}

int
main(void)
{
	int status = 0;
	setbuf(stdout, NULL);

	const char *policies[] = {
		"MOCK_SORT_DESCENDING",
		"MOCK_SORT_PREPEND",
		"MOCK_SORT_APPEND",
		"MOCK_SORT_ASCENDING",
		"MOCK_SORT_MAX_SENTINEL"
	};

	test(test_filter_out_true);
	test(test_filter_out_false);

	for (int i = 0; i < MOCK_SORT_MAX_SENTINEL; i++) {
		mock_variable_sort_policy = i;
		printf("%s: setting variable sort policy to %s\n",
		       program_invocation_short_name, policies[i]);

		test(test_gnvn_buf_size_0);
		test(test_gnvn_0);
		test(test_gnvn_1);
	}

	test(test_get_variable_0);
	test(test_set_variable_0);
	return status;
}

// vim:fenc=utf-8:tw=75:noet
