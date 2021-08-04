// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * mock-variables.h - a mock GetVariable/SetVariable/GNVN/etc
 *                    implementation for testing.
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_MOCK_VARIABLES_H_
#define SHIM_MOCK_VARIABLES_H_

#include "test.h"

EFI_STATUS EFIAPI mock_get_variable(CHAR16 *name, EFI_GUID *guid, UINT32 *attrs,
                                    UINTN *size, VOID *data);
EFI_STATUS EFIAPI mock_get_next_variable_name(UINTN *size, CHAR16 *name,
                                              EFI_GUID *guid);
EFI_STATUS EFIAPI mock_set_variable(CHAR16 *name, EFI_GUID *guid, UINT32 attrs,
                                    UINTN size, VOID *data);
EFI_STATUS EFIAPI mock_query_variable_info(UINT32 attrs,
                                           UINT64 *max_var_storage,
                                           UINT64 *remaining_var_storage,
                                           UINT64 *max_var_size);

EFI_STATUS EFIAPI mock_install_configuration_table(EFI_GUID *guid, VOID *table);

struct mock_variable_limits {
	UINT32 attrs;
	UINT64 *max_var_storage;
	UINT64 *remaining_var_storage;
	UINT64 *max_var_size;
	EFI_STATUS status;

	list_t list;
};

typedef enum {
	MOCK_SORT_DESCENDING,
	MOCK_SORT_PREPEND,
	MOCK_SORT_APPEND,
	MOCK_SORT_ASCENDING,
	MOCK_SORT_MAX_SENTINEL
} mock_sort_policy_t;

extern mock_sort_policy_t mock_variable_sort_policy;
extern mock_sort_policy_t mock_config_table_sort_policy;

#define MOCK_VAR_DELETE_ATTR_ALLOW_ZERO		0x01
#define MOCK_VAR_DELETE_ATTR_ALOW_MISMATCH	0x02

extern UINT32 mock_variable_delete_attr_policy;

extern list_t mock_default_variable_limits;
extern list_t *mock_qvi_limits;
extern list_t *mock_sv_limits;

struct mock_variable {
	CHAR16 *name;
	EFI_GUID guid;
	void *data;
	size_t size;
	uint32_t attrs;

	list_t list;
};

extern list_t mock_variables;

static inline void
dump_mock_variables(const char * const file,
		    const int line,
		    const char * const func)
{
	list_t *pos = NULL;
	printf("%s:%d:%s(): dumping variables\n", file, line, func);
	list_for_each(pos, &mock_variables) {
		struct mock_variable *var;

		var = list_entry(pos, struct mock_variable, list);
		printf("%s:%d:%s():  "GUID_FMT"-%s\n", file, line, func,
		       GUID_ARGS(var->guid), Str2str(var->name));
	}
}

static inline void
dump_mock_variables_if_wrong(const char * const file,
			     const int line,
			     const char * const func,
			     EFI_GUID *guid, CHAR16 *first)
{
	UINTN size = 0;
	CHAR16 buf[8192] = { 0, };
	EFI_STATUS status;

	size = sizeof(buf);
	buf[0] = L'\0';
	status = RT->GetNextVariableName(&size, buf, guid);
	if (EFI_ERROR(status)) {
		printf("%s:%d:%s() Can't dump variables: %lx\n",
		       __FILE__, __LINE__, __func__,
		       (unsigned long)status);
		return;
	}
	buf[size] = L'\0';
	if (StrCmp(buf, first) == 0)
		return;
	printf("%s:%d:%s():expected \"%s\" but got \"%s\".  Variables:\n",
	       file, line, func, Str2str(first), Str2str(buf));
	dump_mock_variables(file, line, func);
}

void mock_load_variables(const char *const dirname, const char *filters[],
                         bool filter_out);
void mock_install_query_variable_info(void);
void mock_uninstall_query_variable_info(void);
void mock_reset_variables(void);
void mock_reset_config_table(void);
void mock_finalize_vars_and_configs(void);

typedef enum {
	NONE = 0,
	CREATE,
	DELETE,
	APPEND,
	REPLACE,
	GET,
} mock_variable_op_t;

static inline const char *
format_var_op(mock_variable_op_t op)
{
	static const char *var_op_names[] = {
		"NONE",
		"CREATE",
		"DELETE",
		"APPEND",
		"REPLACE",
		"GET",
		NULL
	};

	return var_op_names[op];
}

typedef EFI_STATUS (mock_set_variable_pre_hook_t)(CHAR16 *name, EFI_GUID *guid,
						  UINT32 attrs, UINTN size,
						  VOID *data);
extern mock_set_variable_pre_hook_t *mock_set_variable_pre_hook;

typedef void (mock_set_variable_post_hook_t)(CHAR16 *name, EFI_GUID *guid,
					     UINT32 attrs, UINTN size,
					     VOID *data, EFI_STATUS *status,
					     mock_variable_op_t op,
					     const char * const file,
					     const int line,
					     const char * const func);
extern mock_set_variable_post_hook_t *mock_set_variable_post_hook;

typedef EFI_STATUS (mock_get_variable_pre_hook_t)(CHAR16 *name, EFI_GUID *guid,
						  UINT32 *attrs, UINTN *size,
						  VOID *data);
extern mock_get_variable_pre_hook_t *mock_get_variable_pre_hook;

typedef void (mock_get_variable_post_hook_t)(CHAR16 *name, EFI_GUID *guid,
					     UINT32 *attrs, UINTN *size,
					     VOID *data, EFI_STATUS *status,
					     const char * const file,
					     const int line,
					     const char * const func);
extern mock_get_variable_post_hook_t *mock_get_variable_post_hook;

typedef EFI_STATUS (mock_get_next_variable_name_pre_hook_t)(UINTN *size,
							    CHAR16 *name,
							    EFI_GUID *guid);
extern mock_get_next_variable_name_pre_hook_t
	*mock_get_next_variable_name_pre_hook;

typedef void (mock_get_next_variable_name_post_hook_t)(
			UINTN *size, CHAR16 *name, EFI_GUID *guid,
			EFI_STATUS *status, const char * const file,
			const int line, const char * const func);
extern mock_get_next_variable_name_post_hook_t
	*mock_get_next_variable_name_post_hook;

typedef EFI_STATUS (mock_query_variable_info_pre_hook_t)(
			UINT32 attrs, UINT64 *max_var_storage,
			UINT64 *remaining_var_storage, UINT64 *max_var_size);
extern mock_query_variable_info_pre_hook_t *mock_query_variable_info_pre_hook;

typedef void (mock_query_variable_info_post_hook_t)(
	UINT32 attrs, UINT64 *max_var_storage, UINT64 *remaining_var_storage,
	UINT64 *max_var_size, EFI_STATUS *status, const char * const file,
	const int line, const char * const func);
extern mock_query_variable_info_post_hook_t *mock_query_variable_info_post_hook;

#define MOCK_CONFIG_TABLE_ENTRIES 1024
extern EFI_CONFIGURATION_TABLE mock_config_table[MOCK_CONFIG_TABLE_ENTRIES];

#endif /* !SHIM_MOCK_VARIABLES_H_ */
// vim:fenc=utf-8:tw=75:noet
