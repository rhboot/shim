// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-mok-mirror.c - try to test our mok mirroring code
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"
#include "mock-variables.h"
#include "test-data-efivars-1.h"

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
	EFI_GUID guid;
	CHAR16 *name;
	UINT32 attrs;
	UINTN n_ops;
	bool must_be_present;
	bool must_be_absent;
	mock_variable_op_t ops[N_TEST_VAR_OPS];
	EFI_STATUS results[N_TEST_VAR_OPS];
};

static struct test_var *test_vars;

struct mock_mok_variable_config_entry {
	CHAR8 name[256];
	UINT64 data_size;
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

static int
test_mok_mirror_0(void)
{
	const char *mok_rt_vars[n_mok_state_variables];
	EFI_STATUS status;
	EFI_GUID guid = SHIM_LOCK_GUID;
	EFI_GUID mok_config_guid = MOK_VARIABLE_STORE;
	int ret = -1;

	struct test_var test_mok_mirror_0_vars[] = {
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
		{.guid = { 0, },
		 .name = NULL,
		}
	};

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
	test_vars = &test_mok_mirror_0_vars[0];

	import_mok_state(NULL);

	for (size_t i = 0; test_mok_mirror_0_vars[i].name != NULL; i++) {
		struct test_var *tv = &test_mok_mirror_0_vars[i];
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
			assert_goto(found == true, err,
				 "variable \"%s\" was not found.\n",
				 Str2str(tv->name));
		}

		if (tv->must_be_absent) {
			assert_goto(found == false, err,
				 "variable \"%s\" was found.\n",
				 Str2str(tv->name));
		}
	}

	uint8_t *pos = NULL;
	for (size_t i = 0; i < ST->NumberOfTableEntries; i++) {
		EFI_CONFIGURATION_TABLE *ct = &ST->ConfigurationTable[i];

		if (CompareGuid(&ct->VendorGuid, &mok_config_guid) != 0)
			continue;

		pos = (void *)ct->VendorTable;
		break;
	}

	assert_nonzero_goto(pos, err, "%p != 0\n");

	size_t i = 0;
	while (pos) {
		struct mock_mok_variable_config_entry *mock_entry =
			&test_mok_config_table[i];
		struct mok_variable_config_entry *mok_entry =
			(struct mok_variable_config_entry *)pos;

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
		pos += offsetof(struct mok_variable_config_entry, data)
		       + mok_entry->data_size;
		i += 1;
	}

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
			test(test_mok_mirror_0);
			mock_finalize_vars_and_configs();

			if (delete_policies[j] == 0)
				break;
		} while (++j);
	}

	return status;
}

// vim:fenc=utf-8:tw=75:noet
