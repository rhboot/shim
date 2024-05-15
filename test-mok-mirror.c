// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-mok-mirror.c - try to test our mok mirroring code
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"
#include "mock-variables.h"
#include "test-data-efivars-1.h"

#include <err.h>
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"

EFI_STATUS
start_image(EFI_HANDLE image_handle UNUSED, CHAR16 *mm)
{
	printf("Attempted to launch %s\n", Str2str(mm));
	return EFI_SUCCESS;
}

#define N_TEST_VAR_OPS 40
struct test_var {
	/*
	 * The GUID, name, and attributes of the variables
	 */
	EFI_GUID guid;
	CHAR16 *name;
	UINT32 attrs;
	/*
	 * If the variable is required to be present, with the attributes
	 * specified above, for a test to pass
	 */
	bool must_be_present;
	/*
	 * If the variable is required to be absent, no matter what the
	 * attributes, for a test to pass
	 */
	bool must_be_absent;
	/*
	 * The number of operations on this variable that we've seen
	 */
	UINTN n_ops;
	/*
	 * the operations that have occurred on this variable
	 */
	mock_variable_op_t ops[N_TEST_VAR_OPS];
	/*
	 * the result codes of those operations
	 */
	EFI_STATUS results[N_TEST_VAR_OPS];
};

static struct test_var *test_vars;

struct mock_mok_variable_config_entry {
	/*
	 * The name of an entry we expect to see in the MokVars
	 * configuration table
	 */
	CHAR8 name[256];
	/*
	 * The size of its data
	 */
	UINT64 data_size;
	/*
	 * A pointer to what the data should be
	 */
	const unsigned char *data;
};

static void
setvar_post(CHAR16 *name, EFI_GUID *guid, UINT32 attrs,
	    UINTN size, VOID *data, EFI_STATUS *status,
	    mock_variable_op_t op, const char * const file,
	    const int line, const char * const func)
{
	if (!test_vars)
		return;

	for (UINTN i = 0; test_vars[i].name != NULL; i++) {
		struct test_var *tv = &test_vars[i];

		if (CompareGuid(&tv->guid, guid) != 0 ||
		    StrCmp(tv->name, name) != 0)
			continue;
		tv->ops[tv->n_ops] = op;
		tv->results[tv->n_ops] = *status;
		tv->n_ops += 1;
	}
}

static void
getvar_post(CHAR16 *name, EFI_GUID *guid,
	    UINT32 *attrs, UINTN *size,
	    VOID *data, EFI_STATUS *status,
	    const char * const file, const int line, const char * func)
{
	if (EFI_ERROR(*status) &&
	    (*status != EFI_NOT_FOUND &&
	     *status != EFI_BUFFER_TOO_SMALL)) {
		printf("%s:%d:%s():Getting "GUID_FMT"-%s ",
		       file, line, func,
		       GUID_ARGS(*guid), Str2str(name));
		if (attrs)
			printf("attrs:%s\n", format_var_attrs(*attrs));
		else
			printf("attrs:NULL\n");
		printf("failed:%s\n", efi_strerror(*status));
	}

	if (!test_vars)
		return;

	for (UINTN i = 0; test_vars[i].name != NULL; i++) {
		struct test_var *tv = &test_vars[i];

		if (CompareGuid(&tv->guid, guid) != 0 ||
		    StrCmp(tv->name, name) != 0)
			continue;
		tv->ops[tv->n_ops] = GET;
		tv->results[tv->n_ops] = *status;
		tv->n_ops += 1;
	}
}

static EFI_STATUS
check_variables(struct test_var *vars)
{
	for (size_t i = 0; vars[i].name != NULL; i++) {
		struct test_var *tv = &vars[i];
		list_t *pos = NULL;
		bool found = false;
		char buf[1];
		UINTN size = 0;
		UINT32 attrs = 0;
		bool present = false;

		list_for_each(pos, &mock_variables) {
			struct mock_variable *var;
			bool deleted;
			bool created;
			int gets = 0;

			var = list_entry(pos, struct mock_variable, list);
			if (CompareGuid(&tv->guid, &var->guid) != 0 ||
			    StrCmp(var->name, tv->name) != 0)
				continue;
			found = true;
			assert_equal_goto(var->attrs, tv->attrs, err,
					  "\"%s\": wrong attrs; got %s expected %s\n",
					  Str2str(tv->name),
					  format_var_attrs(var->attrs),
					  format_var_attrs(tv->attrs));
			for (UINTN j = 0; j < N_TEST_VAR_OPS
					  && tv->ops[j] != NONE; j++) {
				switch (tv->ops[j]) {
				case NONE:
				default:
					break;
				case CREATE:
					if (tv->results[j] == EFI_SUCCESS)
						created = true;
					break;
				case DELETE:
					assert_goto(tv->results[j] != EFI_SUCCESS, err,
							  "Tried to delete absent variable \"%s\"\n",
							  Str2str(tv->name));
					assert_goto(created == false, err,
						    "Deleted variable \"%s\" was previously created.\n",
						    Str2str(tv->name));
					break;
				case APPEND:
					assert_goto(false, err,
						    "No append action should have been tested\n");
					break;
				case REPLACE:
					assert_goto(false, err,
						    "No replace action should have been tested\n");
					break;
				case GET:
					if (tv->results[j] == EFI_SUCCESS)
						gets += 1;
					break;
				}
			}
			assert_goto(gets == 0 || gets == 1, err,
				    "Variable should not be read %d times.\n", gets);
		}
		if (tv->must_be_present) {
			/*
			 * This asserts if it isn't present, and if that's
			 * the case, then the attributes are already
			 * validated in the search loop
			 */
			assert_goto(found == true, err,
				 "variable \"%s\" was not found.\n",
				 Str2str(tv->name));
		}

		if (tv->must_be_absent) {
			/*
			 * deliberately does not check the attributes at
			 * this time.
			 */
			assert_goto(found == false, err,
				 "variable \"%s\" was found.\n",
				 Str2str(tv->name));
		}
	}

	return EFI_SUCCESS;
err:
	return EFI_INVALID_PARAMETER;
}

static EFI_STATUS
check_config_table(struct mock_mok_variable_config_entry *test_configs,
		   uint8_t *config_pos)
{
	size_t i = 0;
	struct mok_variable_config_entry *mok_entry = NULL;
	struct mock_mok_variable_config_entry *mock_entry = NULL;

	while (config_pos) {
		mock_entry = &test_configs[i];
		mok_entry = (struct mok_variable_config_entry *)config_pos;

		/*
		 * If the tables are different lengths, this will trigger.
		 */
		assert_equal_goto(mok_entry->name[0], mock_entry->name[0], err,
				  "mok.name[0] %ld != test.name[0] %ld\n");
		if (mock_entry->name[0] == 0)
			break;

		assert_nonzero_goto(mok_entry->name[0], err, "%ld != %ld\n");
		assert_zero_goto(strncmp(mok_entry->name, mock_entry->name,
					 sizeof(mock_entry->name)),
				 err, "%ld != %ld: strcmp(\"%s\",\"%s\")\n",
				 mok_entry->name, mock_entry->name);

		/*
		 * As of 7501b6bb449f ("mok: fix potential buffer overrun in
		 * import_mok_state"), we should not see any mok config
		 * variables with data_size == 0.
		 */
		assert_nonzero_goto(mok_entry->data_size, err, "%ld != 0\n");

		assert_equal_goto(mok_entry->data_size, mock_entry->data_size,
				  err, "%ld != %ld\n");
		assert_zero_goto(CompareMem(mok_entry->data, mock_entry->data,
					    mok_entry->data_size),
				 err, "%ld != %ld\n");
		config_pos += offsetof(struct mok_variable_config_entry, data)
			   + mok_entry->data_size;
		i += 1;
	}

	return EFI_SUCCESS;
err:
	warnx("Failed on entry %zu mok.name:\"%s\" mock.name:\"%s\"", i,
	      mok_entry->name, mock_entry->name);
	if ((mok_entry && mock_entry) && (!mok_entry->name[0] || !mock_entry->name[0]))
		warnx("Entry is missing in %s variable list.", mok_entry->name[0] ? "expected" : "found");

	return EFI_INVALID_PARAMETER;
}

static int
test_mok_mirror(struct test_var *vars,
		struct mock_mok_variable_config_entry *configs,
		EFI_STATUS expected_status)
{
	EFI_STATUS status;
	EFI_GUID mok_config_guid = MOK_VARIABLE_STORE;
	int ret = -1;

	status = import_mok_state(NULL);
	assert_equal_goto(status, expected_status, err,
			  "got 0x%016lx, expected 0x%016lx\n",
			  expected_status);

	test_vars = vars;

	status = check_variables(vars);
	if (EFI_ERROR(status))
		goto err;

	uint8_t *pos = NULL;
	for (size_t i = 0; i < ST->NumberOfTableEntries; i++) {
		EFI_CONFIGURATION_TABLE *ct = &ST->ConfigurationTable[i];

		if (CompareGuid(&ct->VendorGuid, &mok_config_guid) != 0)
			continue;

		pos = (void *)ct->VendorTable;
		break;
	}

	assert_nonzero_goto(pos, err, "%p != 0\n");

	status = check_config_table(configs, pos);
	if (EFI_ERROR(status))
		goto err;

	ret = 0;
err:
	for (UINTN k = 0; k < n_mok_state_variables; k++) {
		struct mok_state_variable *v =
			&mok_state_variables[k];
		if (v->data_size && v->data) {
			free(v->data);
			v->data = NULL;
			v->data_size = 0;
		}
	}

	test_vars = NULL;

	return ret;
}

/*
 * This tests mirroring of mok variables on fairly optimistic conditions:
 * there's enough space for everything, and so we expect to see all the
 * RT variables for which we have data mirrored
 */
static int
test_mok_mirror_with_enough_space(void)
{
	const char *mok_rt_vars[n_mok_state_variables];
	EFI_STATUS status;
	EFI_GUID guid = SHIM_LOCK_GUID;
	int ret = -1;

	struct test_var test_mok_mirror_with_enough_space_vars[] = {
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokList",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .ops = { NONE, },
		},
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokListRT",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokListX",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .ops = { NONE, },
		},
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokListXRT",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		{.guid = SHIM_LOCK_GUID,
		 .name = L"SbatLevel",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .ops = { NONE, },
		},
		{.guid = SHIM_LOCK_GUID,
		 .name = L"SbatLevelRT",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokIgnoreDB",
		 .must_be_absent = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokSBState",
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .ops = { NONE, },
		},
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokSBStateRT",
		 .must_be_absent = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		{.guid = SHIM_LOCK_GUID,
		 .name = L"HSIStatus",
		 .attrs = 0,
		 .ops = { NONE, },
		},
		{.guid = { 0, },
		 .name = NULL,
		}
	};

	/*
	 * We must see the supplied values of MokListRT, MokListXRT, and
	 * SbatLevelRT in the config table
	 */
	struct mock_mok_variable_config_entry test_mok_config_table[] = {
		{.name = "MokListRT",
		 .data_size = sizeof(test_data_efivars_1_MokListRT),
		 .data = test_data_efivars_1_MokListRT
		},
		{.name = "MokListXRT",
		 .data_size = sizeof(test_data_efivars_1_MokListXRT),
		 .data = test_data_efivars_1_MokListXRT
		},
		{.name = "SbatLevelRT",
		 .data_size = sizeof(test_data_efivars_1_SbatLevelRT),
		 .data = test_data_efivars_1_SbatLevelRT
		},
		{.name = "MokListTrustedRT",
		 .data_size = sizeof(test_data_efivars_1_MokListTrustedRT),
		 .data = test_data_efivars_1_MokListTrustedRT
		},
		{.name = "HSIStatus",
		 .data_size = sizeof(test_data_efivars_1_HSIStatus),
		 .data = test_data_efivars_1_HSIStatus
		},
		{.name = { 0, },
		 .data_size = 0,
		 .data = NULL,
		}
	};

	for (size_t i = 0; i < n_mok_state_variables; i++) {
		mok_rt_vars[i] = mok_state_variables[i].rtname8;
	}

	mock_load_variables("test-data/efivars-1", mok_rt_vars, true);

	mock_set_variable_post_hook = setvar_post;
	mock_get_variable_post_hook = getvar_post;

	ret = test_mok_mirror(&test_mok_mirror_with_enough_space_vars[0],
			      test_mok_config_table,
			      EFI_SUCCESS);

	mock_set_variable_post_hook = NULL;
	mock_get_variable_post_hook = NULL;
	return ret;
}

static int
test_mok_mirror_setvar_out_of_resources(void)
{
	const char *mok_rt_vars[n_mok_state_variables];
	EFI_STATUS status;
	EFI_GUID guid = SHIM_LOCK_GUID;
	EFI_GUID mok_config_guid = MOK_VARIABLE_STORE;
	int ret = -1;

	/*
	 * These sizes are picked specifically so that MokListRT will fail
	 * to get mirrored with the test data in test-data/efivars-1
	 */
	list_t mock_obnoxious_variable_limits;
	UINT64 obnoxious_max_var_storage = 0xffe4;
	UINT64 obnoxious_remaining_var_storage = 919+0x3c;
	UINT64 obnoxious_max_var_size = 919;

	struct mock_variable_limits obnoxious_limits[] = {
		{.attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS,
		 .max_var_storage = &obnoxious_max_var_storage,
		 .remaining_var_storage = &obnoxious_remaining_var_storage,
		 .max_var_size = &obnoxious_max_var_size,
		 .status = EFI_SUCCESS,
		},
		{.attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .max_var_storage = &obnoxious_max_var_storage,
		 .remaining_var_storage = &obnoxious_remaining_var_storage,
		 .max_var_size = &obnoxious_max_var_size,
		 .status = EFI_SUCCESS,
		},
		{.attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .max_var_storage = &obnoxious_max_var_storage,
		 .remaining_var_storage = &obnoxious_remaining_var_storage,
		 .max_var_size = &obnoxious_max_var_size,
		 .status = EFI_SUCCESS,
		},
		{.attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .max_var_storage = &obnoxious_max_var_storage,
		 .remaining_var_storage = &obnoxious_remaining_var_storage,
		 .max_var_size = &obnoxious_max_var_size,
		 .status = EFI_SUCCESS,
		},
		{.attrs = 0, }
	};

	struct test_var test_mok_mirror_enospc_vars[] = {
		/*
		 * We must to see a BS|NV MokList
		 */
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokList",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .ops = { NONE, },
		},
		/*
		 * We must *NOT* see a BS|RT MokListRT
		 */
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokListRT",
		 .must_be_absent = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		/*
		 * We must see a BS|NV MokListX
		 */
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokListX",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .ops = { NONE, },
		},
		/*
		 * We must see a BS|RT MokListXRT
		 */
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokListXRT",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		/*
		 * We must see a BS|NV SbatLevel
		 */
		{.guid = SHIM_LOCK_GUID,
		 .name = L"SbatLevel",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .ops = { NONE, },
		},
		/*
		 * We must see a BS|RT SbatLevelRT
		 */
		{.guid = SHIM_LOCK_GUID,
		 .name = L"SbatLevelRT",
		 .must_be_present = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		/*
		 * We must not see a MokIgnoreDB
		 */
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokIgnoreDB",
		 .must_be_absent = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		/*
		 * We must not see MokSBState
		 */
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokSBState",
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_NON_VOLATILE,
		 .ops = { NONE, },
		},
		/*
		 * We must not see MokSBStateRT
		 */
		{.guid = SHIM_LOCK_GUID,
		 .name = L"MokSBStateRT",
		 .must_be_absent = true,
		 .attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			  EFI_VARIABLE_RUNTIME_ACCESS,
		 .ops = { NONE, },
		},
		{.guid = { 0, },
		 .name = NULL,
		}
	};

	/*
	 * We must see the supplied values of MokListRT, MokListXRT, and
	 * SbatLevelRT in the config table
	 */
	struct mock_mok_variable_config_entry test_mok_config_table[] = {
		{.name = "MokListRT",
		 .data_size = sizeof(test_data_efivars_1_MokListRT),
		 .data = test_data_efivars_1_MokListRT
		},
		{.name = "MokListXRT",
		 .data_size = sizeof(test_data_efivars_1_MokListXRT),
		 .data = test_data_efivars_1_MokListXRT
		},
		{.name = "SbatLevelRT",
		 .data_size = sizeof(test_data_efivars_1_SbatLevelRT),
		 .data = test_data_efivars_1_SbatLevelRT
		},
		{.name = "MokListTrustedRT",
		 .data_size = sizeof(test_data_efivars_1_MokListTrustedRT),
		 .data = test_data_efivars_1_MokListTrustedRT
		},
		{.name = "HSIStatus",
		 .data_size = sizeof(test_data_efivars_1_HSIStatus),
		 .data = test_data_efivars_1_HSIStatus
		},
		{.name = { 0, },
		 .data_size = 0,
		 .data = NULL,
		}
	};

	UINT64 max_storage_sz = 0;
	UINT64 max_var_sz = 0;
	UINT64 remaining_sz = 0;

	for (size_t i = 0; i < n_mok_state_variables; i++) {
		mok_rt_vars[i] = mok_state_variables[i].rtname8;
	}

	mock_load_variables("test-data/efivars-1", mok_rt_vars, true);

	mock_set_variable_post_hook = setvar_post;
	mock_get_variable_post_hook = getvar_post;

	mock_set_usage_limits(&mock_obnoxious_variable_limits,
			      &obnoxious_limits[0]);

	ret = test_mok_mirror(&test_mok_mirror_enospc_vars[0],
			      test_mok_config_table,
			      EFI_OUT_OF_RESOURCES);

	mock_set_default_usage_limits();

	mock_set_variable_post_hook = NULL;
	mock_get_variable_post_hook = NULL;
	return ret;
}

int
main(void)
{
	int status = 0;
	setbuf(stdout, NULL);

	const char *sort_policy_names[] = {
		"MOCK_SORT_DESCENDING",
		"MOCK_SORT_PREPEND",
		"MOCK_SORT_APPEND",
		"MOCK_SORT_ASCENDING",
		"MOCK_SORT_MAX_SENTINEL"
	};

	const char *del_policy_names[] = {
		"MOCK_VAR_DELETE_ATTR_ALLOW_ZERO",
		"MOCK_VAR_DELETE_ATTR_ALOW_MISMATCH",
		"MOCK_VAR_DELETE_ATTR_ALLOW_ZERO|MOCK_VAR_DELETE_ATTR_ALOW_MISMATCH",
		"MOCK_VAR_DELETE_ATTR_ALLOW_NONE",
		NULL
	};

	int delete_policies[] = {
		MOCK_VAR_DELETE_ATTR_ALLOW_ZERO,
		MOCK_VAR_DELETE_ATTR_ALOW_MISMATCH,
		MOCK_VAR_DELETE_ATTR_ALLOW_ZERO
		| MOCK_VAR_DELETE_ATTR_ALOW_MISMATCH,
		0
	};

	for (int i = 0; i < MOCK_SORT_MAX_SENTINEL; i++) {
		mock_variable_sort_policy = i;
		mock_config_table_sort_policy = i;
		int j = 0;

		printf("%s: setting variable sort policy to %s\n",
		       program_invocation_short_name, sort_policy_names[i]);
		do {
			printf("%s: setting delete policy to %s\n",
			       program_invocation_short_name,
			       del_policy_names[j]);

			mock_variable_delete_attr_policy = delete_policies[j];
			test(test_mok_mirror_with_enough_space);
			mock_finalize_vars_and_configs();

			test(test_mok_mirror_setvar_out_of_resources);
			mock_finalize_vars_and_configs();

			if (delete_policies[j] == 0)
				break;
		} while (++j);
	}

	return status;
}

// vim:fenc=utf-8:tw=75:noet
