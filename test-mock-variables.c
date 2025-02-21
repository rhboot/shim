// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-mock-variables.c - test our mock variable implementation (irony)
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"
#include "mock-variables.h"

#include <errno.h>
#include <efivar/efivar.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "test-data-efivars-0.h"

#pragma GCC diagnostic ignored "-Wunused-label"

static const size_t guidstr_size = sizeof("8be4df61-93ca-11d2-aa0d-00e098032b8c");

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
		/*
		 * We don't want to filter out the variables we've added to
		 * mok mirroring that aren't really from mok; right now
		 * this is a reasonable heuristic for that.
		 */
		if (mok_state_variables[i].flags & MOK_VARIABLE_CONFIG_ONLY)
			continue;
		mok_rt_vars[i] = mok_state_variables[i].rtname8;
	}

	mock_load_variables(testvars, mok_rt_vars, true);

#if defined(SHIM_DEBUG) && SHIM_DEBUG != 0
	dump_mock_variables(__FILE__, __LINE__, __func__);
#endif

	/*
	 * This tests the sort policy, filtering for only variables in the
	 * EFI "global" namespace.  If ascending the first thing should
	 * be Boot0000, if descending it should be dbxDefault
	 */
#if defined(SHIM_DEBUG) && SHIM_DEBUG >= 1
	printf("Testing mock variable sorting in the global namespace\n");
#endif
	size = sizeof(buf);
	buf[0] = L'\0';
	status = RT->GetNextVariableName(&size, buf, &GV_GUID);
	assert_equal_goto(status, EFI_SUCCESS, err, "0x%lx != 0x%lx\n");

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

	/*
	 * Do it again but test for only variables in the Secure Boot
	 * policy guid namespace.  Ascending should be "db", descending
	 * "dbx".
	 */
#if defined(SHIM_DEBUG) && SHIM_DEBUG >= 1
	printf("Testing mock variable sorting in the Secure Boot GUID namespace\n");
#endif
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
		/*
		 * We don't want to filter out the variables we've added to
		 * mok mirroring that aren't really from mok; right now
		 * this is a reasonable heuristic for that.
		 */
		if (mok_state_variables[i].flags & MOK_VARIABLE_CONFIG_ONLY)
			continue;
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

static void
dump_config_table_if_wrong(const char * const func, int line, ...)
{
	va_list alist, blist;
	bool okay = true;
	size_t n = 0, m = 0;

	va_start(alist, line);
	va_copy(blist, alist);

	int idx = va_arg(alist, int);
	EFI_GUID *guid = va_arg(alist, EFI_GUID *);
	while (idx >= 0 && guid != NULL) {
		EFI_CONFIGURATION_TABLE *entry;
		if (idx < 0)
			goto nexta;

		n += 1;
		if (idx >= (int)ST->NumberOfTableEntries) {
			okay = false;
			goto nexta;
		}

		entry = &ST->ConfigurationTable[idx];
		if (CompareGuid(guid, &entry->VendorGuid) != 0)
			okay = false;

nexta:
		idx = va_arg(alist, int);
		guid = va_arg(alist, EFI_GUID *);
	}
	va_end(alist);

	if (okay)
		return;

	printf("%s:%d:%s(): %d entries:\n", __FILE__, line, func, ST->NumberOfTableEntries);
	idx = va_arg(blist, int);
	guid = va_arg(blist, EFI_GUID *);
	while (idx >= 0 && guid != NULL) {
		EFI_CONFIGURATION_TABLE *entry;

		if (idx >= (int)ST->NumberOfTableEntries) {
			printf("\t[%d]: invalid index for " GUID_FMT "\n",
			       idx, GUID_ARGS(*guid));
			goto nexta;
		}

		if (idx < 0) {
			printf("\t[%d]: " GUID_FMT "\n", idx, GUID_ARGS(*guid));
		} else {
			entry = &ST->ConfigurationTable[idx];
			printf("\t[%d]: %p ", idx, entry);
			printf("{.VendorGuid:" GUID_FMT ",", GUID_ARGS(entry->VendorGuid));
			printf("&.VendorTable:%p}\n", entry->VendorTable);
			if (CompareGuid(guid, &entry->VendorGuid) != 0)
				printf("\t\t\t  expected:" GUID_FMT "\n", GUID_ARGS(*guid));
		}
next:
		idx = va_arg(blist, int);
		guid = va_arg(blist, EFI_GUID *);
	}
	va_end(blist);

	if ((int)ST->NumberOfTableEntries - n == 0)
		return;

	printf("%d extra table entries:\n", ST->NumberOfTableEntries - n);
	for (m = n; m < ST->NumberOfTableEntries; m++) {
		EFI_CONFIGURATION_TABLE *entry;

		entry = &ST->ConfigurationTable[m];

		printf("\t[%d]: %p ", m, entry);
		printf("{.VendorGuid:" GUID_FMT ",", GUID_ARGS(entry->VendorGuid));
		printf("&.VendorTable:%p}\n", entry->VendorTable);
	}
}

static int
test_install_config_table_0(void)
{
	int ret = -1;
	EFI_STATUS status;

	/*
	 * These three guids are chosen on purpose: they start with "a",
	 * "b", and "c", respective to their variable names, so you can
	 * identify them when dumped.
	 */
	EFI_GUID aguid = SECURITY_PROTOCOL_GUID;
	char astr[guidstr_size];
	void *astrp = &astr[0];
	int aidx = -1;

	EFI_GUID bguid = EFI_HTTP_BINDING_GUID;
	char bstr[guidstr_size];
	void *bstrp = &bstr[0];
	int bidx = -1;

	EFI_GUID cguid = MOK_VARIABLE_STORE;
	char cstr[guidstr_size];
	void *cstrp = &cstr[0];
	int cidx = -1;

	EFI_GUID lip = LOADED_IMAGE_PROTOCOL;

	EFI_GUID guids[3];

	char tmpstr[guidstr_size];

	sprintf(astrp, GUID_FMT, GUID_ARGS(aguid));
	sprintf(bstrp, GUID_FMT, GUID_ARGS(bguid));
	sprintf(cstrp, GUID_FMT, GUID_ARGS(cguid));

	assert_equal_return(ST->NumberOfTableEntries, 0, -1, "%lu != %lu\n");

	/*
	 * test installing one
	 */
	status = BS->InstallConfigurationTable(&bguid, bstrp);
	assert_equal_return(status, EFI_SUCCESS, -1, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 1, err, "%lu != %lu\n");

	sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[0].VendorGuid));
	assert_zero_goto(CompareGuid(&ST->ConfigurationTable[0].VendorGuid, &bguid),
			 err, "%d != 0 (%s != %s)\n", tmpstr, bstr);
	assert_equal_goto(ST->ConfigurationTable[0].VendorTable,
			  bstrp, err, "%p != %p\n");

	/*
	 * test re-installing the same one
	 */
	status = BS->InstallConfigurationTable(&bguid, bstrp);
	assert_equal_goto(status, EFI_SUCCESS, err, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 1, err, "%lu != %lu\n");

	sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[0].VendorGuid));
	assert_zero_goto(CompareGuid(&ST->ConfigurationTable[0].VendorGuid, &bguid),
			 err, "%d != 0 (%s != %s)\n", tmpstr, bstr);
	assert_equal_goto(ST->ConfigurationTable[0].VendorTable,
			  bstrp, err, "%p != %p\n");

	/*
	 * Test installing a second one
	 */
	status = BS->InstallConfigurationTable(&aguid, astrp);
	assert_equal_goto(status, EFI_SUCCESS, err, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 2, err, "%lu != %lu\n");

	switch (mock_config_table_sort_policy) {
		case MOCK_SORT_DESCENDING:
			aidx = 1;
			bidx = 0;
			break;
		case MOCK_SORT_PREPEND:
			aidx = 0;
			bidx = 1;
			break;
		case MOCK_SORT_APPEND:
			aidx = 1;
			bidx = 0;
			break;
		case MOCK_SORT_ASCENDING:
			aidx = 0;
			bidx = 1;
			break;
		default:
			break;
	}

	dump_config_table_if_wrong(__func__, __LINE__,
				   aidx, &aguid,
				   bidx, &bguid,
				   cidx, &cguid,
				   -1, NULL);

	sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[aidx].VendorGuid));
	assert_zero_goto(CompareGuid(&ST->ConfigurationTable[aidx].VendorGuid, &aguid),
			 err, "%d != 0 (%s != %s)\n", tmpstr, astr);
	assert_equal_goto(ST->ConfigurationTable[aidx].VendorTable, astrp,
			  err, "%p != %p\n");

	sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[bidx].VendorGuid));
	assert_zero_goto(CompareGuid(&ST->ConfigurationTable[bidx].VendorGuid, &bguid),
			 err, "%d != 0 (%s != %s)\n", tmpstr, bstr);
	assert_equal_goto(ST->ConfigurationTable[bidx].VendorTable, bstrp,
			  err, "%p != %p\n");

	/*
	 * Test installing a third one
	 */
	status = BS->InstallConfigurationTable(&cguid, cstrp);
	assert_equal_goto(status, EFI_SUCCESS, err, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 3, err, "%lu != %lu\n");

	switch (mock_config_table_sort_policy) {
		case MOCK_SORT_DESCENDING:
			aidx = 2;
			bidx = 1;
			cidx = 0;
			break;
		case MOCK_SORT_PREPEND:
			aidx = 1;
			bidx = 2;
			cidx = 0;
			break;
		case MOCK_SORT_APPEND:
			aidx = 1;
			bidx = 0;
			cidx = 2;
			break;
		case MOCK_SORT_ASCENDING:
			aidx = 0;
			bidx = 1;
			cidx = 2;
			break;
		default:
			break;
	}

	dump_config_table_if_wrong(__func__, __LINE__,
				   aidx, &aguid,
				   bidx, &bguid,
				   cidx, &cguid,
				   -1, NULL);

	sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[aidx].VendorGuid));
	assert_zero_goto(CompareGuid(&ST->ConfigurationTable[aidx].VendorGuid, &aguid),
			 err, "%d != 0 (%s != %s)\n", tmpstr, astr);
	assert_equal_goto(ST->ConfigurationTable[aidx].VendorTable, astrp,
			  err, "%p != %p\n");
	memcpy(&guids[aidx], &aguid, sizeof(EFI_GUID));

	sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[bidx].VendorGuid));
	assert_zero_goto(CompareGuid(&ST->ConfigurationTable[bidx].VendorGuid, &bguid),
			 err, "%d != 0 (%s != %s)\n", tmpstr, bstr);
	assert_equal_goto(ST->ConfigurationTable[bidx].VendorTable, bstrp,
			  err, "%p != %p\n");
	memcpy(&guids[bidx], &bguid, sizeof(EFI_GUID));

	sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[cidx].VendorGuid));
	assert_zero_goto(CompareGuid(&ST->ConfigurationTable[cidx].VendorGuid, &cguid),
			 err, "%d != 0 (%s != %s)\n", tmpstr, cstr);
	assert_equal_goto(ST->ConfigurationTable[cidx].VendorTable, cstrp,
			  err, "%p != %p\n");
	memcpy(&guids[cidx], &cguid, sizeof(EFI_GUID));

	/*
	 * Test removing NULL guid
	 */
	status = BS->InstallConfigurationTable(NULL, NULL);
	assert_equal_goto(status, EFI_INVALID_PARAMETER, err, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 3, err, "%lu != %lu\n");

	/*
	 * Test removing a guid that's not present
	 */
	status = BS->InstallConfigurationTable(&lip, NULL);
	assert_equal_goto(status, EFI_NOT_FOUND, err, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 3, err, "%lu != %lu\n");

	/*
	 * Test removing the middle one
	 */
	status = BS->InstallConfigurationTable(&guids[1], NULL);
	assert_equal_goto(status, EFI_SUCCESS, err, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 2, err, "%lu != %lu\n");

	switch (mock_config_table_sort_policy) {
		case MOCK_SORT_DESCENDING:
			aidx = 1;
			bidx = -1;
			cidx = 0;
			break;
		case MOCK_SORT_PREPEND:
			aidx = -1;
			bidx = 1;
			cidx = 0;
			break;
		case MOCK_SORT_APPEND:
			aidx = -1;
			bidx = 0;
			cidx = 1;
			break;
		case MOCK_SORT_ASCENDING:
			aidx = 0;
			bidx = -1;
			cidx = 1;
			break;
		default:
			break;
	}

	dump_config_table_if_wrong(__func__, __LINE__,
				   aidx, &aguid,
				   bidx, &bguid,
				   cidx, &cguid,
				   -1, NULL);

	if (aidx >= 0) {
		sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[aidx].VendorGuid));
		assert_zero_goto(CompareGuid(&ST->ConfigurationTable[aidx].VendorGuid, &aguid),
				 err, "%d != 0 (%s != %s)\n", tmpstr, astr);
		assert_equal_goto(ST->ConfigurationTable[aidx].VendorTable, astrp,
				  err, "%p != %p\n");
		memcpy(&guids[aidx], &aguid, sizeof(EFI_GUID));
	}

	if (bidx >= 0) {
		sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[bidx].VendorGuid));
		assert_zero_goto(CompareGuid(&ST->ConfigurationTable[bidx].VendorGuid, &bguid),
				 err, "%d != 0 (%s != %s)\n", tmpstr, bstr);
		assert_equal_goto(ST->ConfigurationTable[bidx].VendorTable, bstrp,
				  err, "%p != %p\n");
		memcpy(&guids[bidx], &bguid, sizeof(EFI_GUID));
	}

	if (cidx >= 0) {
		sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[cidx].VendorGuid));
		assert_zero_goto(CompareGuid(&ST->ConfigurationTable[cidx].VendorGuid, &cguid),
				 err, "%d != 0 (%s != %s)\n", tmpstr, cstr);
		assert_equal_goto(ST->ConfigurationTable[cidx].VendorTable, cstrp,
				  err, "%p != %p\n");
		memcpy(&guids[cidx], &cguid, sizeof(EFI_GUID));
	}

	/*
	 * Test removing the lowest one
	 */
	status = BS->InstallConfigurationTable(&guids[0], NULL);
	assert_equal_goto(status, EFI_SUCCESS, err, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 1, err, "%lu != %lu\n");

	switch (mock_config_table_sort_policy) {
		case MOCK_SORT_DESCENDING:
			aidx = 0;
			bidx = -1;
			cidx = -1;
			break;
		case MOCK_SORT_PREPEND:
			aidx = -1;
			bidx = 0;
			cidx = -1;
			break;
		case MOCK_SORT_APPEND:
			aidx = -1;
			bidx = -1;
			cidx = 0;
			break;
		case MOCK_SORT_ASCENDING:
			aidx = -1;
			bidx = -1;
			cidx = 0;
			break;
		default:
			break;
	}

	dump_config_table_if_wrong(__func__, __LINE__,
				   aidx, &aguid,
				   bidx, &bguid,
				   cidx, &cguid,
				   -1, NULL);

	if (aidx >= 0) {
		sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[aidx].VendorGuid));
		assert_zero_goto(CompareGuid(&ST->ConfigurationTable[aidx].VendorGuid, &aguid),
				 err, "%d != 0 (%s != %s)\n", tmpstr, astr);
		assert_equal_goto(ST->ConfigurationTable[aidx].VendorTable, astrp,
				  err, "%p != %p\n");
		memcpy(&guids[aidx], &aguid, sizeof(EFI_GUID));
	}

	if (bidx >= 0) {
		sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[bidx].VendorGuid));
		assert_zero_goto(CompareGuid(&ST->ConfigurationTable[bidx].VendorGuid, &bguid),
				 err, "%d != 0 (%s != %s)\n", tmpstr, bstr);
		assert_equal_goto(ST->ConfigurationTable[bidx].VendorTable, bstrp,
				  err, "%p != %p\n");
		memcpy(&guids[bidx], &bguid, sizeof(EFI_GUID));
	}

	if (cidx >= 0) {
		sprintf(tmpstr, GUID_FMT, GUID_ARGS(ST->ConfigurationTable[cidx].VendorGuid));
		assert_zero_goto(CompareGuid(&ST->ConfigurationTable[cidx].VendorGuid, &cguid),
				 err, "%d != 0 (%s != %s)\n", tmpstr, cstr);
		assert_equal_goto(ST->ConfigurationTable[cidx].VendorTable, cstrp,
				  err, "%p != %p\n");
		memcpy(&guids[cidx], &cguid, sizeof(EFI_GUID));
	}

	/*
	 * Test removing the last one
	 */
	status = BS->InstallConfigurationTable(&guids[0], NULL);
	assert_equal_goto(status, EFI_SUCCESS, err, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 0, err, "%lu != %lu\n");

	/*
	 * Test removing it again
	 */
	status = BS->InstallConfigurationTable(&guids[0], NULL);
	assert_equal_goto(status, EFI_NOT_FOUND, err, "%lx != %lx\n");
	assert_equal_goto(ST->NumberOfTableEntries, 0, err, "%lu != %lu\n");

	ret = 0;
err:
	while (ST->NumberOfTableEntries)
		BS->InstallConfigurationTable(&ST->ConfigurationTable[0].VendorGuid, NULL);
	mock_reset_config_table();

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
		mock_config_table_sort_policy = i;
		printf("%s: setting variable sort policy to %s\n",
		       program_invocation_short_name, policies[i]);

		test(test_gnvn_buf_size_0);
		test(test_gnvn_0);
		test(test_gnvn_1);

		test(test_install_config_table_0);
	}

	test(test_get_variable_0);
	test(test_set_variable_0);
	return status;
}

// vim:fenc=utf-8:tw=75:noet
