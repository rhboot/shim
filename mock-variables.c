// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * mock-variables.c - a mock GetVariable/SetVariable/GNVN/etc
 *                    implementation for testing.
 * Copyright Peter Jones <pjones@redhat.com>
 */
#include "shim.h"
#include "mock-variables.h"

#include <dirent.h>
#include <efivar/efivar.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

list_t mock_default_variable_limits;
list_t *mock_qvi_limits = &mock_default_variable_limits;
list_t *mock_sv_limits = &mock_default_variable_limits;

list_t mock_variables = LIST_HEAD_INIT(mock_variables);

mock_sort_policy_t mock_variable_sort_policy = MOCK_SORT_APPEND;
mock_sort_policy_t mock_config_table_sort_policy = MOCK_SORT_APPEND;

UINT32 mock_variable_delete_attr_policy;

mock_set_variable_pre_hook_t *mock_set_variable_pre_hook = NULL;
mock_set_variable_post_hook_t *mock_set_variable_post_hook = NULL;
mock_get_variable_pre_hook_t *mock_get_variable_pre_hook = NULL;
mock_get_variable_post_hook_t *mock_get_variable_post_hook = NULL;
mock_get_next_variable_name_pre_hook_t *mock_get_next_variable_name_pre_hook = NULL;
mock_get_next_variable_name_post_hook_t *mock_get_next_variable_name_post_hook = NULL;
mock_query_variable_info_pre_hook_t *mock_query_variable_info_pre_hook = NULL;
mock_query_variable_info_post_hook_t *mock_query_variable_info_post_hook = NULL;

static EFI_STATUS
mock_sv_pre_hook(CHAR16 *name, EFI_GUID *guid, UINT32 attrs, UINTN size,
		 VOID *data)
{
	EFI_STATUS status = EFI_SUCCESS;
	if (mock_set_variable_pre_hook)
		status = mock_set_variable_pre_hook(name, guid,
						    attrs, size, data);
	return status;
}

static void
mock_sv_post_hook_(CHAR16 *name, EFI_GUID *guid, UINT32 attrs, UINTN size,
		   VOID *data, EFI_STATUS *status, mock_variable_op_t op,
		   const char * const file, const int line,
		   const char * const func)
{
	if (mock_set_variable_post_hook)
		mock_set_variable_post_hook(name, guid, attrs, size,
		                            data, status, op, file, line, func);
}
#define mock_sv_post_hook(name, guid, attrs, size, data, status, op) \
	mock_sv_post_hook_(name, guid, attrs, size, data, status, op,\
			   __FILE__, __LINE__, __func__)

static EFI_STATUS
mock_gv_pre_hook(CHAR16 *name, EFI_GUID *guid, UINT32 *attrs, UINTN *size,
                 VOID *data)
{
	EFI_STATUS status = EFI_SUCCESS;
	if (mock_get_variable_pre_hook)
		status = mock_get_variable_pre_hook(name, guid,
						    attrs, size, data);
	return status;
}

static void
mock_gv_post_hook_(CHAR16 *name, EFI_GUID *guid, UINT32 *attrs, UINTN *size,
		   VOID *data, EFI_STATUS *status, const char * const file,
		   const int line, const char * const func)
{
	if (mock_get_variable_post_hook)
		mock_get_variable_post_hook(name, guid, attrs, size, data,
					    status, file, line, func);
}
#define mock_gv_post_hook(name, guid, attrs, size, data, status) \
	mock_gv_post_hook_(name, guid, attrs, size, data, status,\
			   __FILE__, __LINE__, __func__)

static EFI_STATUS
mock_gnvn_pre_hook(UINTN *size, CHAR16 *name, EFI_GUID *guid)
{
	EFI_STATUS status = EFI_SUCCESS;
	if (mock_get_next_variable_name_pre_hook)
		status = mock_get_next_variable_name_pre_hook(size, name, guid);
	return status;
}

static void
mock_gnvn_post_hook_(UINTN *size, CHAR16 *name, EFI_GUID *guid,
		     EFI_STATUS *status, const char * const file,
		     const int line, const char * const func)
{
	if (mock_get_next_variable_name_post_hook)
		mock_get_next_variable_name_post_hook(size, name, guid, status,
						      file, line, func);
}
#define mock_gnvn_post_hook(size, name, guid, status) \
	mock_gnvn_post_hook_(size, name, guid, status,\
			     __FILE__, __LINE__, __func__)

static EFI_STATUS
mock_qvi_pre_hook(UINT32 attrs, UINT64 *max_var_storage,
                  UINT64 *remaining_var_storage, UINT64 *max_var_size)
{
	EFI_STATUS status = EFI_SUCCESS;
	if (mock_query_variable_info_pre_hook)
		status = mock_query_variable_info_pre_hook(
					attrs, max_var_storage,
					remaining_var_storage, max_var_size);
	return status;
}

static void
mock_qvi_post_hook_(UINT32 attrs, UINT64 *max_var_storage,
		    UINT64 *remaining_var_storage, UINT64 *max_var_size,
		    EFI_STATUS *status, const char * const file,
		    const int line, const char * const func)
{
	if (mock_query_variable_info_post_hook)
		mock_query_variable_info_post_hook(attrs, max_var_storage,
		                                   remaining_var_storage,
		                                   max_var_size, status,
						   file, line, func);
}
#define mock_qvi_post_hook(attrs, max_var_storage, remaining_var_storage,\
			   max_var_size, status) \
	mock_qvi_post_hook_(attrs, max_var_storage, remaining_var_storage,\
			    max_var_size, status, \
			    __FILE__, __LINE__, __func__)

static const size_t guidstr_size = sizeof("8be4df61-93ca-11d2-aa0d-00e098032b8c");

static int
variable_limits_cmp(const struct mock_variable_limits * const v0,
		    const struct mock_variable_limits * const v1)
{
	UINT32 mask = EFI_VARIABLE_NON_VOLATILE |
		      EFI_VARIABLE_BOOTSERVICE_ACCESS |
		      EFI_VARIABLE_RUNTIME_ACCESS;

	return (v0->attrs & mask) - (v1->attrs & mask);
}

static INT64
variable_cmp(const struct mock_variable * const v0,
	     const struct mock_variable * const v1)
{
	INT64 ret;
	if (v0 == NULL || v1 == NULL)
		return (uintptr_t)v0 - (uintptr_t)v1;

	ret = CompareGuid(&v0->guid, &v1->guid);
	ret <<= 8ul;
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():  "GUID_FMT" %s "GUID_FMT" (0x%011"PRIx64" %"PRId64")\n",
	       __FILE__, __LINE__-1, __func__,
	       GUID_ARGS(v0->guid),
	       ret < 0 ? "<" : (ret > 0 ? ">" : "="),
	       GUID_ARGS(v1->guid),
	       (UINT64)ret & 0x1fffffffffful,
	       ret);
#endif
	if (ret != 0) {
		return ret;
	}

	ret = StrCmp(v0->name, v1->name);
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():   \"%s\" %s \"%s\" (0x%02hhx (%d)\n",
	       __FILE__, __LINE__-1, __func__,
	       Str2str(v0->name),
	       ret < 0 ? "<" : (ret > 0 ? ">" : "=="),
	       Str2str(v1->name),
	       ret, ret);
#endif
	return ret;
}

static char *
list2var(list_t *pos)
{
	static char buf0[1024];
	static char buf1[1024];
	char *out;
	static int n;
	struct mock_variable *var;

	out = n++ % 2 ? buf0 : buf1;
	if (n > 1)
		n -= 2;
	SetMem(out, 1024, 0);
	if (pos == &mock_variables) {
		strcpy(out, "list tail");
		return out;
	}
	var = list_entry(pos, struct mock_variable, list);
	snprintf(out, 1023, GUID_FMT"-%s",
		 GUID_ARGS(var->guid),
		 Str2str(var->name));
	return out;
}

EFI_STATUS EFIAPI
mock_get_variable(CHAR16 *name, EFI_GUID *guid, UINT32 *attrs, UINTN *size,
                  VOID *data)
{
	list_t *pos = NULL;
	struct mock_variable goal = {
		.name = name,
		.guid = *guid,
	};
	struct mock_variable *result = NULL;
	EFI_STATUS status;

	status = mock_gv_pre_hook(name, guid, attrs, size, data);
	if (EFI_ERROR(status))
		return status;

	if (name == NULL || guid == NULL || size == NULL) {
		status = EFI_INVALID_PARAMETER;
		mock_gv_post_hook(name, guid, attrs, size, data, &status);
		return status;
	}

	list_for_each(pos, &mock_variables) {
		struct mock_variable *var;

		var = list_entry(pos, struct mock_variable, list);
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s(): varcmp("GUID_FMT"-%s, "GUID_FMT"-%s)\n",
		       __FILE__, __LINE__-1, __func__,
		       GUID_ARGS(goal.guid), Str2str(goal.name),
		       GUID_ARGS(var->guid), Str2str(var->name));
#endif
		if (variable_cmp(&goal, var) == 0) {
			if (attrs != NULL)
				*attrs = var->attrs;
			if (var->size > *size) {
				*size = var->size;
				status = EFI_BUFFER_TOO_SMALL;
				mock_gv_post_hook(name, guid, attrs, size, data,
				                  &status);
				return status;
			}
			if (data == NULL) {
				status = EFI_INVALID_PARAMETER;
				mock_gv_post_hook(name, guid, attrs, size, data,
						  &status);
				return status;
			}
			*size = var->size;
			memcpy(data, var->data, var->size);
			status = EFI_SUCCESS;
			mock_gv_post_hook(name, guid, attrs, size, data,
			                  &status);
			return status;
		}
	}

	status = EFI_NOT_FOUND;
	mock_gv_post_hook(name, guid, attrs, size, data, &status);
	return status;
}

static EFI_STATUS
mock_gnvn_set_result(UINTN *size, CHAR16 *name, EFI_GUID *guid,
		     struct mock_variable *result)
{
	EFI_STATUS status;

	if (*size < StrSize(result->name)) {
		*size = StrSize(result->name);
		status = EFI_BUFFER_TOO_SMALL;
		mock_gnvn_post_hook(size, name, guid, &status);
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():  returning %lx\n",
		       __FILE__, __LINE__-1, __func__, status);
#endif
		return status;
	}

	*size = StrLen(result->name) + 1;
	StrCpy(name, result->name);
	memcpy(guid, &result->guid, sizeof(EFI_GUID));

	status = EFI_SUCCESS;
	mock_gnvn_post_hook(size, name, guid, &status);
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():  returning %lx\n",
	       __FILE__, __LINE__-1, __func__, status);
#endif
	return status;
}

EFI_STATUS EFIAPI
mock_get_next_variable_name(UINTN *size, CHAR16 *name, EFI_GUID *guid)
{
	list_t *pos = NULL;
	struct mock_variable goal = {
		.name = name,
		.guid = *guid,
	};
	struct mock_variable *result = NULL;
	bool found = false;
	EFI_STATUS status;

	status = mock_gnvn_pre_hook(size, name, guid);
	if (EFI_ERROR(status))
		return status;

	if (size == NULL || name == NULL || guid == NULL) {
		status = EFI_INVALID_PARAMETER;
		mock_gnvn_post_hook(size, name, guid, &status);
		return status;
	}

	for (size_t i = 0; i < *size; i++) {
		if (name[i] == 0) {
			found = true;
			break;
		}
	}

	if (found == false) {
		status = EFI_INVALID_PARAMETER;
		mock_gnvn_post_hook(size, name, guid, &status);
		return status;
	}

	found = false;
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():searching for "GUID_FMT"%s%s\n",
	       __FILE__, __LINE__-1, __func__,
	       GUID_ARGS(*guid),
	       name[0] == 0 ? "" : "-",
	       name[0] == 0 ? "" : Str2str(name));
#endif
	list_for_each(pos, &mock_variables) {
		struct mock_variable *var;

		var = list_entry(pos, struct mock_variable, list);
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():  candidate var:%p &var->guid:%p &var->list:%p\n",
			__FILE__, __LINE__-1, __func__, var, &var->guid, &var->list);
#endif
		if (name[0] == 0) {
			if (CompareGuid(&var->guid, guid) == 0) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
				printf("%s:%d:%s():  found\n",
				       __FILE__, __LINE__-1, __func__);
#endif
				result = var;
				found = true;
				break;
			}
		} else {
			if (found) {
				if (CompareGuid(&var->guid, guid) == 0) {
					result = var;
					break;
				}
				continue;
			}

#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s(): varcmp("GUID_FMT"-%s, "GUID_FMT"-%s)\n",
		       __FILE__, __LINE__-1, __func__,
		       GUID_ARGS(goal.guid), Str2str(goal.name),
		       GUID_ARGS(var->guid), Str2str(var->name));
#endif
			if (variable_cmp(&goal, var) == 0) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
				printf("%s:%d:%s():  found\n",
				       __FILE__, __LINE__-1, __func__);
#endif
				found = true;
			}
		}
	}
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	if (result) {
		printf("%s:%d:%s():  found:%d result:%p &result->guid:%p &result->list:%p\n"
		       __FILE__, __LINE__-1, __func__, found, result,
		       &result->guid, &result->list);
		printf("%s:%d:%s():  "GUID_FMT"-%s\n",
		       __FILE__, __LINE__-1, __func__, GUID_ARGS(result->guid),
		       Str2str(result->name));
	} else {
		printf("%s:%d:%s():  not found\n",
		       __FILE__, __LINE__-1, __func__);
	}
#endif

	if (!found) {
		if (name[0] == 0)
			status = EFI_NOT_FOUND;
		else
			status = EFI_INVALID_PARAMETER;
		mock_gnvn_post_hook(size, name, guid, &status);
		return status;
	}

	if (!result) {
		status = EFI_NOT_FOUND;
		mock_gnvn_post_hook(size, name, guid, &status);
		return status;
	}

	return mock_gnvn_set_result(size, name, guid, result);
}

static void
free_var(struct mock_variable *var)
{
	if (!var)
		return;
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s(): var:%p &var->guid:%p ",
	       __FILE__, __LINE__-1, __func__,
	       var, var ? &var->guid : NULL);
	if (var)
		printf(GUID_FMT"-%s", GUID_ARGS(var->guid),
		       var->name ? Str2str(var->name) : "");
	printf("\n");
#endif
	list_del(&var->list);
	if (var->size && var->data)
		free(var->data);
	SetMem(var, sizeof(*var), 0);
	free(var);
}

static bool
mock_sv_attrs_match(UINT32 old, UINT32 new)
{
	UINT32 mask = ~((UINT32)EFI_VARIABLE_APPEND_WRITE);

	return (old & mask) == (new & mask);
}

static EFI_STATUS
mock_sv_adjust_usage_data(UINT32 attrs, size_t size, ssize_t change)
{
	const UINT32 bs = EFI_VARIABLE_BOOTSERVICE_ACCESS;
	const UINT32 bs_nv = bs | EFI_VARIABLE_NON_VOLATILE;
	const UINT32 bs_rt = bs | EFI_VARIABLE_RUNTIME_ACCESS;
	const UINT32 bs_rt_nv = bs_nv | bs_rt;
	struct mock_variable_limits goal = {
		.attrs = attrs & bs_rt_nv,
	};
	struct mock_variable_limits *qvi_limits = NULL;
	struct mock_variable_limits *sv_limits = NULL;
	list_t var, *pos = NULL;
	UINT64 remaining;

	list_for_each(pos, mock_qvi_limits) {
		struct mock_variable_limits *candidate;

		candidate = list_entry(pos, struct mock_variable_limits, list);
		if (variable_limits_cmp(&goal, candidate) == 0) {
			qvi_limits = candidate;
			break;
		}
	}

	list_for_each(pos, mock_sv_limits) {
		struct mock_variable_limits *candidate;

		candidate = list_entry(pos, struct mock_variable_limits, list);
		if (variable_limits_cmp(&goal, candidate) == 0) {
			sv_limits = candidate;
			break;
		}
	}
	if (!sv_limits) {
		return EFI_UNSUPPORTED;
	}

	if (sv_limits->status != EFI_SUCCESS)
		return sv_limits->status;

	if (*sv_limits->max_var_size < size) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():*sv_limits->max_var_size:%zu size:%zu\n",
		       __FILE__, __LINE__, __func__,
		       *sv_limits->max_var_size, size);
#endif
		return EFI_OUT_OF_RESOURCES;
	}

	if (change > 0 && (UINT64)change > *sv_limits->remaining_var_storage) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():*sv_limits->remaining_var_storage:%zu change:%zd\n",
		       __FILE__, __LINE__, __func__,
		       *sv_limits->remaining_var_storage, change);
#endif
		return EFI_OUT_OF_RESOURCES;
	}

	*sv_limits->remaining_var_storage += change;

	if (qvi_limits) {
		/*
		 * If the adjustment here is wrong, we don't want to not do
		 * the set variable, we also don't want to not account
		 * for it, and of course we can't have any integer UB.  So
		 * just limit it safely and move on, even though that may
		 * result in wrong checks against QueryVariableInfo() later.
		 *
		 * As if there are correct checks against QueryVariableInfo()...
		 */
		if (qvi_limits->remaining_var_storage == sv_limits->remaining_var_storage)
			;
		else if (change < 0 && (UINT64)-change > *qvi_limits->remaining_var_storage)
			*qvi_limits->remaining_var_storage = 0;
		else if (change > 0 && UINT64_MAX - *qvi_limits->remaining_var_storage < (UINT64)change)
			*qvi_limits->remaining_var_storage = UINT64_MAX;
		else
			*qvi_limits->remaining_var_storage += change;
	}
	return EFI_SUCCESS;
}

static EFI_STATUS
mock_delete_variable(struct mock_variable *var)
{
	EFI_STATUS status;

	status = mock_sv_adjust_usage_data(var->attrs, 0, - var->size);
	if (EFI_ERROR(status)) {
		printf("%s:%d:%s():  status:0x%lx\n",
		       __FILE__, __LINE__ - 1, __func__, status);
		mock_sv_post_hook(var->name, &var->guid, var->attrs, var->size,
		                  var->data, &status, DELETE);
		return status;
	}

	status = EFI_SUCCESS;
	mock_sv_post_hook(var->name, &var->guid, var->attrs, 0, 0, &status,
	                  DELETE);
	free_var(var);
	return status;
}

static EFI_STATUS
mock_replace_variable(struct mock_variable *var, VOID *data, UINTN size)
{
	EFI_STATUS status;
	VOID *new;

	status = mock_sv_adjust_usage_data(var->attrs, size,
					   - var->size + size);
	if (EFI_ERROR(status)) {
		mock_sv_post_hook(var->name, &var->guid, var->attrs, var->size,
		                  var->data, &status, REPLACE);
		return status;
	}

	new = calloc(1, size);
	if (!new) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():calloc(1, %zu) failed\n",
		       __FILE__, __LINE__, __func__,
		       size);
#endif
		status = EFI_OUT_OF_RESOURCES;
		mock_sv_post_hook(var->name, &var->guid, var->attrs, var->size,
		                  var->data, &status, REPLACE);
		return status;
	}
	memcpy(new, data, size);
	free(var->data);
	var->data = new;
	var->size = size;

	status = EFI_SUCCESS;
	mock_sv_post_hook(var->name, &var->guid, var->attrs, var->size,
	                  var->data, &status, REPLACE);
	return status;
}

static EFI_STATUS
mock_sv_extend(struct mock_variable *var, VOID *data, UINTN size)
{
	EFI_STATUS status;
	uint8_t *new;

	if (size == 0) {
		status = EFI_SUCCESS;
		mock_sv_post_hook(var->name, &var->guid, var->attrs, var->size,
		                  var->data, &status, APPEND);
		return status;
	}

	status = mock_sv_adjust_usage_data(var->attrs, var->size + size, size);
	if (EFI_ERROR(status)) {
		mock_sv_post_hook(var->name, &var->guid, var->attrs, var->size,
		                  var->data, &status, APPEND);
		return status;
	}

	new = realloc(var->data, var->size + size);
	if (!new) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():realloc(%zu) failed\n",
		       __FILE__, __LINE__, __func__,
		       var->size + size);
#endif
		status = EFI_OUT_OF_RESOURCES;
		mock_sv_post_hook(var->name, &var->guid, var->attrs, var->size,
		                  var->data, &status, APPEND);
		return status;
	}

	memcpy(&new[var->size], data, size);
	var->data = (void *)new;
	var->size += size;

	status = EFI_SUCCESS;
	mock_sv_post_hook(var->name, &var->guid, var->attrs, var->size,
	                  var->data, &status, APPEND);
	return status;
}

void
mock_print_var_list(list_t *head)
{
	list_t *pos = NULL;
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():variables so far:\n", __FILE__, __LINE__, __func__);
#endif
	list_for_each(pos, head) {
		struct mock_variable *var = NULL;

		var = list_entry(pos, struct mock_variable, list);
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():  "GUID_FMT"-%s (%lu bytes)\n",
		       __FILE__, __LINE__ - 1, __func__,
		       GUID_ARGS(var->guid), Str2str(var->name), var->size);
#endif
	}
}

static EFI_STATUS
mock_new_variable(CHAR16 *name, EFI_GUID *guid, UINT32 attrs, UINTN size,
		  VOID *data, struct mock_variable **out)
{
	EFI_STATUS status;
	struct mock_variable *var;
	uint8_t *buf;

	if (size == 0) {
		status = EFI_INVALID_PARAMETER;
		return status;
	}

	status = EFI_OUT_OF_RESOURCES;
	buf = calloc(1, sizeof(struct mock_variable) + StrSize(name));
	if (!buf) {
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():  calloc(1, %zu) failed\n",
		       __FILE__, __LINE__, __func__,
		       sizeof(struct mock_variable) + StrSize(name));
#endif
		goto err;
	}
	var = (struct mock_variable *)buf;

#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():  var:%p &var->guid:%p &var->list:%p\n",
	       __FILE__, __LINE__-1, __func__, var, &var->guid, &var->list);
#endif

	var->data = malloc(size);
	if (!var->data)
		goto err_free;

	var->name = (CHAR16 *)&buf[sizeof(*var)];
	StrCpy(var->name, name);
	memcpy(&var->guid, guid, sizeof(EFI_GUID));
	memcpy(var->data, data, size);
	var->size = size;
	var->attrs = attrs;
	INIT_LIST_HEAD(&var->list);

#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():  var: "GUID_FMT"-%s\n",
	       __FILE__, __LINE__-1, __func__,
	       GUID_ARGS(var->guid), Str2str(var->name));
#endif

	*out = var;
	status = EFI_SUCCESS;
err_free:
	if (EFI_ERROR(status))
		free_var(var);
err:
	return status;
}

EFI_STATUS EFIAPI
mock_set_variable(CHAR16 *name, EFI_GUID *guid, UINT32 attrs, UINTN size,
                  VOID *data)
{
	list_t *pos = NULL, *tmp = NULL, *var_list = NULL;
	struct mock_variable goal = {
		.name = name,
		.guid = *guid,
	};
	struct mock_variable *var = NULL;
	bool found = false;
	bool add_tail = true;
	EFI_STATUS status;
	long cmp = -1;

	status = mock_sv_pre_hook(name, guid, attrs, size, data);
	if (EFI_ERROR(status))
		return status;

	if (!name || name[0] == 0 || !guid) {
		status = EFI_INVALID_PARAMETER;
		mock_sv_post_hook(name, guid, attrs, size, data, &status,
		                  CREATE);
		return status;
	}

	if ((attrs & EFI_VARIABLE_RUNTIME_ACCESS) &&
	    !(attrs & EFI_VARIABLE_BOOTSERVICE_ACCESS)) {
		status = EFI_INVALID_PARAMETER;
		mock_sv_post_hook(name, guid, attrs, size, data, &status,
		                  CREATE);
		return status;
	}

#if 0
	/*
	 * We don't ever operate after ExitBootServices(), so I'm not
	 * checking for the missing EFI_VARIABLE_RUNTIME_ACCESS case
	 */
	if (has_exited_boot_services() && !(attrs & EFI_VARIABLE_RUNTIME_ACCESS)) {
		status = EFI_INVALID_PARAMETER;
		mock_sv_post_hook(name, guid, attrs, size, data, &status,
				  CREATE);
		return status;
	}
#endif

#if 0
	/*
	 * For now, we're ignoring that we don't support these.
	 */
	if (attrs & (EFI_VARIABLE_HARDWARE_ERROR_RECORD |
		     EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS |
		     EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS |
		     EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS)) {
		status = EFI_UNSUPPORTED;
		mock_sv_post_hook(name, guid, attrs, size, data, &status,
				  CREATE);
		return status;
	}
#endif

#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():Setting "GUID_FMT"-%s\n",
	       __FILE__, __LINE__ - 1, __func__,
	       GUID_ARGS(*guid), Str2str(name));
#endif
	switch (mock_variable_sort_policy) {
	case MOCK_SORT_PREPEND:
		var_list = &mock_variables;
		add_tail = false;
		break;
	case MOCK_SORT_APPEND:
		var_list = &mock_variables;
		add_tail = true;
		break;
	case MOCK_SORT_DESCENDING:
		add_tail = true;
		break;
	case MOCK_SORT_ASCENDING:
		add_tail = true;
		break;
	default:
		break;
	}

	pos = &mock_variables;
	list_for_each_safe(pos, tmp, &mock_variables) {
		found = false;
		var = list_entry(pos, struct mock_variable, list);
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s(): varcmp("GUID_FMT"-%s, "GUID_FMT"-%s)\n",
		       __FILE__, __LINE__-1, __func__,
		       GUID_ARGS(goal.guid), Str2str(goal.name),
		       GUID_ARGS(var->guid), Str2str(var->name));
#endif
		cmp = variable_cmp(&goal, var);
		cmp = cmp < 0 ? -1 : (cmp > 0 ? 1 : 0);

		switch (mock_variable_sort_policy) {
		case MOCK_SORT_DESCENDING:
			if (cmp >= 0) {
				var_list = pos;
				found = true;
			}
			break;
		case MOCK_SORT_ASCENDING:
			if (cmp <= 0) {
				var_list = pos;
				found = true;
			}
			break;
		default:
			if (cmp == 0) {
				var_list = pos;
				found = true;
			}
			break;
		}
		if (found)
			break;
	}
#if defined(SHIM_DEBUG) && SHIM_DEBUG != 0
	printf("%s:%d:%s():var_list:%p &mock_variables:%p cmp:%ld\n",
	       __FILE__, __LINE__ - 1, __func__,
	       var_list, &mock_variables, cmp);
#endif
	if (cmp != 0 || (cmp == 0 && var_list == &mock_variables)) {
		size_t totalsz = size + StrSize(name);
#if defined(SHIM_DEBUG) && SHIM_DEBUG != 0
		printf("%s:%d:%s():var:%p attrs:0x%lx\n",
		       __FILE__, __LINE__ - 1, __func__, var, attrs);
#endif
		status = mock_new_variable(name, guid, attrs, size, data, &var);
		if (EFI_ERROR(status)) {
			mock_sv_post_hook(name, guid, attrs, size, data,
					  &status, CREATE);
			return status;
		}
		mock_sv_adjust_usage_data(attrs, size, totalsz);
		mock_sv_post_hook(name, guid, attrs, size, data,
				  &status, CREATE);
		if (EFI_ERROR(status)) {
			mock_sv_adjust_usage_data(attrs, 0, -totalsz);
			return status;
		}

#if defined(SHIM_DEBUG) && SHIM_DEBUG != 0
		printf("%s:%d:%s(): Adding "GUID_FMT"-%s %s %s\n",
		       __FILE__, __LINE__ - 1, __func__,
		       GUID_ARGS(var->guid), Str2str(var->name),
		       add_tail ? "after" : "before",
		       list2var(pos));
#endif
		if (add_tail)
			list_add_tail(&var->list, pos);
		else
			list_add(&var->list, pos);
		return status;
	}

	var = list_entry(var_list, struct mock_variable, list);
#if defined(SHIM_DEBUG) && SHIM_DEBUG != 0
	printf("%s:%d:%s():var:%p attrs:%s cmp:%ld size:%ld\n",
	       __FILE__, __LINE__ - 1, __func__,
	       var, format_var_attrs(var->attrs), cmp, size);
#endif
	if (!mock_sv_attrs_match(var->attrs, attrs)) {
		status = EFI_INVALID_PARAMETER;
		if (size == 0 && !(attrs & EFI_VARIABLE_APPEND_WRITE)) {
			if ((mock_variable_delete_attr_policy & MOCK_VAR_DELETE_ATTR_ALLOW_ZERO)
			    && attrs == 0) {
				status = EFI_SUCCESS;
			} else if (mock_variable_delete_attr_policy & MOCK_VAR_DELETE_ATTR_ALOW_MISMATCH) {
				status = EFI_SUCCESS;
			}
		}
		if (EFI_ERROR(status)) {
			printf("%s:%d:%s(): var->attrs:%s attrs:%s\n",
			       __FILE__, __LINE__ - 1, __func__,
			       format_var_attrs(var->attrs),
			       format_var_attrs(attrs));
			mock_sv_post_hook(name, guid, attrs, size, data,
					  &status, REPLACE);
			return status;
		}
	}

	if (attrs & EFI_VARIABLE_APPEND_WRITE)
		return mock_sv_extend(var, data, size);

	if (size == 0) {
		UINT32 mask = EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS
			      | EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS;
		/*
		 * We can't process deletes on these correctly unless we
		 * parse the header.
		 */
		if (attrs & mask) {
			return EFI_INVALID_PARAMETER;
		}

		return mock_delete_variable(var);
	}

	return mock_replace_variable(var, data, size);
}

EFI_STATUS EFIAPI
mock_query_variable_info(UINT32 attrs, UINT64 *max_var_storage,
                         UINT64 *remaining_var_storage, UINT64 *max_var_size)
{
	list_t mvl, *pos = NULL;
	struct mock_variable_limits goal = {
		.attrs = attrs,
	};
	struct mock_variable_limits *limits = NULL;
	EFI_STATUS status;

	status = mock_qvi_pre_hook(attrs, max_var_storage,
				   remaining_var_storage, max_var_size);
	if (EFI_ERROR(status))
		return status;

	if (max_var_storage == NULL ||
	    remaining_var_storage == NULL ||
	    max_var_size == NULL) {
		status = EFI_INVALID_PARAMETER;
		mock_qvi_post_hook(attrs, max_var_storage,
		                   remaining_var_storage, max_var_size,
		                   &status);
		return status;
	}

	list_for_each(pos, mock_qvi_limits) {
		limits = list_entry(pos, struct mock_variable_limits, list);
		if (variable_limits_cmp(&goal, limits) == 0) {
			*max_var_storage = *limits->max_var_storage;
			*remaining_var_storage = *limits->remaining_var_storage;
			*max_var_size = *limits->max_var_size;

			status = EFI_SUCCESS;
			mock_qvi_post_hook(attrs, max_var_storage,
			                   remaining_var_storage, max_var_size,
			                   &status);
			return status;
		}
	}

	status = EFI_UNSUPPORTED;
	mock_qvi_post_hook(attrs, max_var_storage, remaining_var_storage,
	                   max_var_size, &status);
	return status;
}

static UINT64 default_max_var_storage;
static UINT64 default_remaining_var_storage;
static UINT64 default_max_var_size;

static struct mock_variable_limits default_limits[] = {
	{.attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS,
	 .max_var_storage = &default_max_var_storage,
	 .remaining_var_storage = &default_remaining_var_storage,
	 .max_var_size = &default_max_var_size,
	 .status = EFI_SUCCESS,
	},
	{.attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		  EFI_VARIABLE_RUNTIME_ACCESS,
	 .max_var_storage = &default_max_var_storage,
	 .remaining_var_storage = &default_remaining_var_storage,
	 .max_var_size = &default_max_var_size,
	 .status = EFI_SUCCESS,
	},
	{.attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		  EFI_VARIABLE_NON_VOLATILE,
	 .max_var_storage = &default_max_var_storage,
	 .remaining_var_storage = &default_remaining_var_storage,
	 .max_var_size = &default_max_var_size,
	 .status = EFI_SUCCESS,
	},
	{.attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		  EFI_VARIABLE_RUNTIME_ACCESS |
		  EFI_VARIABLE_NON_VOLATILE,
	 .max_var_storage = &default_max_var_storage,
	 .remaining_var_storage = &default_remaining_var_storage,
	 .max_var_size = &default_max_var_size,
	 .status = EFI_SUCCESS,
	},
	{.attrs = 0, }
};

void
mock_set_default_usage_limits(void)
{
	default_max_var_storage = 65536;
	default_remaining_var_storage = 65536;
	default_max_var_size = 32768;

	INIT_LIST_HEAD(&mock_default_variable_limits);
	for (size_t i = 0; default_limits[i].attrs != 0; i++) {
		INIT_LIST_HEAD(&default_limits[i].list);
		list_add_tail(&default_limits[i].list,
			      &mock_default_variable_limits);
	}
}

void
mock_load_one_variable(int dfd, const char * const dirname, char * const name)
{
	int fd;
	FILE *f;
	int rc;
	struct stat statbuf;
	size_t guidlen, namelen;
	efi_guid_t guid;
	size_t sz;
	ssize_t offset = 0;
	EFI_STATUS status;
	UINT32 attrs;

	rc = fstatat(dfd, name, &statbuf, 0);
	if (rc < 0)
		err(2, "Could not stat \"%s/%s\"", dirname, name);

	if (!(S_ISREG(statbuf.st_mode)))
		return;

	if (statbuf.st_size < 5)
		errx(2, "Test data variable \"%s/%s\" is too small (%ld bytes)",
		     dirname, name, statbuf.st_size);

#if 0
	mock_print_var_list(&mock_variables);
#endif

	uint8_t buf[statbuf.st_size];

	fd = openat(dfd, name, O_RDONLY);
	if (fd < 0)
		err(2, "Could not open \"%s/%s\"", dirname, name);

	f = fdopen(fd, "r");
	if (!f)
		err(2, "Could not open \"%s/%s\"", dirname, name);

	while (offset != statbuf.st_size) {
		sz = fread(buf + offset, 1, statbuf.st_size - offset, f);
		if (sz == 0) {
			if (ferror(f))
				err(2, "Could not read from \"%s/%s\"",
				    dirname, name);
			if (feof(f))
				errx(2, "Unexpected end of file reading \"%s/%s\"",
				     dirname, name);
		}

		offset += sz;
	}

	guidlen = strlen("8be4df61-93ca-11d2-aa0d-00e098032b8c");
	namelen = strlen(name) - guidlen;

	if (namelen < 2)
		errx(2, "namelen for \"%s\" is %zu!?!", name, namelen);

	CHAR16 namebuf[namelen];

	name[namelen-1] = 0;
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("loading %s-%s\n", &name[namelen], name);
#endif
	for (size_t i = 0; i < namelen; i++)
		namebuf[i] = name[i];

	rc = efi_str_to_guid(&name[namelen], &guid);
	if (rc < 0)
		err(2, "Could not parse \"%s\" as EFI GUID", &name[namelen]);

	memcpy(&attrs, (UINT32 *)buf, sizeof(UINT32));

	status = RT->SetVariable(namebuf, (EFI_GUID *)&guid, attrs,
				 statbuf.st_size - sizeof(attrs),
				 &buf[sizeof(attrs)]);
	if (EFI_ERROR(status))
		errx(2, "%s:%d:%s(): Could not set variable: 0x%llx",
		     __FILE__, __LINE__ - 1, __func__,
		     (unsigned long long)status);

	fclose(f);
}

void
mock_load_variables(const char *const dirname, const char *filters[],
		    bool filter_out)
{
	int dfd;
	DIR *d;
	struct dirent *entry;

	d = opendir(dirname);
	if (!d)
		err(1, "Could not open directory \"%s\"", dirname);

	dfd = dirfd(d);
	if (dfd < 0)
		err(1, "Could not get directory file descriptor for \"%s\"",
		    dirname);

	while ((entry = readdir(d)) != NULL) {
		size_t len = strlen(entry->d_name);
		bool found = false;
		if (filters && len > guidstr_size + 1) {
			char spacebuf[len];

			len -= guidstr_size;
			SetMem(spacebuf, sizeof(spacebuf)-1, ' ');
			spacebuf[len] = '\0';
			for (size_t i = 0; filters[i]; i++) {
				if (strlen(filters[i]) > len)
					continue;
				if (!strncmp(entry->d_name, filters[i], len)) {
					found = true;
					break;
				}
			}
		}
		if ((found == false && filter_out == true) ||
		    (found == true && filter_out == false)) {
			mock_load_one_variable(dfd, dirname, entry->d_name);
		}
	}

	closedir(d);
#if 0
	mock_print_var_list(&mock_variables);
#endif
}

static bool qvi_installed = false;

void
mock_install_query_variable_info(void)
{
	qvi_installed = true;
	RT->Hdr.Revision = 2ul << 16ul;
	RT->QueryVariableInfo = mock_query_variable_info;
}

void
mock_uninstall_query_variable_info(void)
{
	qvi_installed = false;
	RT->Hdr.Revision = EFI_1_10_SYSTEM_TABLE_REVISION;
	RT->QueryVariableInfo = mock_efi_unsupported;
}

EFI_CONFIGURATION_TABLE mock_config_table[MOCK_CONFIG_TABLE_ENTRIES] = {
	{
		.VendorGuid = { 0, },
		.VendorTable = NULL
	},
};

int
mock_config_table_cmp(const void *p0, const void *p1)
{
	EFI_CONFIGURATION_TABLE *entry0, *entry1;
	long cmp;

	if (!p0 || !p1) {
		cmp = (int)((intptr_t)p0 - (intptr_t)p1);
	} else {
		entry0 = (EFI_CONFIGURATION_TABLE *)p0;
		entry1 = (EFI_CONFIGURATION_TABLE *)p1;
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("comparing %p to %p\n", p0, p1);
#endif
		cmp = CompareGuid(&entry0->VendorGuid, &entry1->VendorGuid);
	}

	if (mock_config_table_sort_policy == MOCK_SORT_DESCENDING) {
		cmp = -cmp;
	}

	return cmp;
}

EFI_STATUS EFIAPI
mock_install_configuration_table(EFI_GUID *guid, VOID *table)
{
	bool found = false;
	EFI_CONFIGURATION_TABLE *entry;
	int idx = 0;
	size_t sz;

	if (!guid)
		return EFI_INVALID_PARAMETER;

	for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
		EFI_CONFIGURATION_TABLE *entry = &ST->ConfigurationTable[i];

		if (CompareGuid(guid, &entry->VendorGuid) == 0) {
			found = true;
			if (table) {
				// replace it
				entry->VendorTable = table;
			} else {
				// delete it
				ST->NumberOfTableEntries -= 1;
				sz = ST->NumberOfTableEntries - i;
				sz *= sizeof(*entry);
				memmove(&entry[0], &entry[1], sz);
			}
			return EFI_SUCCESS;
		}
	}
	if (!found && table == NULL)
		return EFI_NOT_FOUND;
	if (ST->NumberOfTableEntries == MOCK_CONFIG_TABLE_ENTRIES - 1) {
		/*
		 * If necessary, we could allocate another table and copy
		 * the data, but I'm lazy and we probably don't need to.
		 */
		return EFI_OUT_OF_RESOURCES;
	}

	switch (mock_config_table_sort_policy) {
	case MOCK_SORT_DESCENDING:
	case MOCK_SORT_ASCENDING:
	case MOCK_SORT_APPEND:
		idx = ST->NumberOfTableEntries;
		break;
	case MOCK_SORT_PREPEND:
		sz = ST->NumberOfTableEntries ? ST->NumberOfTableEntries : 0;
		sz *= sizeof(ST->ConfigurationTable[0]);
		memmove(&ST->ConfigurationTable[1], &ST->ConfigurationTable[0], sz);
		idx = 0;
		break;
	default:
		break;
	}

	entry = &ST->ConfigurationTable[idx];
	memcpy(&entry->VendorGuid, guid, sizeof(EFI_GUID));
	entry->VendorTable = table;
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s(): installing entry %p={%p,%p} as entry %d\n",
	       __FILE__, __LINE__, __func__,
	       entry, &entry->VendorGuid, entry->VendorTable, idx);
#endif
	ST->NumberOfTableEntries += 1;

#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s():ST->ConfigurationTable:%p\n"
	       "\t[%d]:%p\n"
	       "\t[%d]:%p\n",
	       __FILE__, __LINE__, __func__, ST->ConfigurationTable,
	       0, &ST->ConfigurationTable[0],
	       1, &ST->ConfigurationTable[1]);
#endif
	switch (mock_config_table_sort_policy) {
	case MOCK_SORT_DESCENDING:
	case MOCK_SORT_ASCENDING:
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s(): entries before sorting:\n", __FILE__, __LINE__, __func__);
		for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
			printf("\t[%d] = %p = {", i, &ST->ConfigurationTable[i]);
			printf(".VendorGuid=" GUID_FMT, GUID_ARGS(ST->ConfigurationTable[i].VendorGuid));
			printf(".VendorTable=%p}\n", ST->ConfigurationTable[i].VendorTable);
		}
#endif
		qsort(&ST->ConfigurationTable[0], ST->NumberOfTableEntries,
		      sizeof(ST->ConfigurationTable[0]),
		      mock_config_table_cmp);
		break;
	default:
		break;
	}
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
	printf("%s:%d:%s(): entries:\n", __FILE__, __LINE__, __func__);
	for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
		printf("\t[%d] = %p = {", i, &ST->ConfigurationTable[i]);
		printf(".VendorGuid=" GUID_FMT, GUID_ARGS(ST->ConfigurationTable[i].VendorGuid));
		printf(".VendorTable=%p}\n", ST->ConfigurationTable[i].VendorTable);
	}
#endif

	return EFI_SUCCESS;
}

void CONSTRUCTOR
mock_reset_variables(void)
{
	list_t *pos = NULL, *tmp = NULL;
	static bool once = true;

	init_efi_system_table();

	mock_set_variable_pre_hook = NULL;
	mock_set_variable_post_hook = NULL;
	mock_get_variable_pre_hook = NULL;
	mock_get_variable_post_hook = NULL;
	mock_get_next_variable_name_pre_hook = NULL;
	mock_get_next_variable_name_post_hook = NULL;
	mock_query_variable_info_pre_hook = NULL;
	mock_query_variable_info_post_hook = NULL;

	if (once) {
		INIT_LIST_HEAD(&mock_variables);
		once = false;
#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():mock_variables = {%p,%p};\n",
		       __FILE__, __LINE__-1, __func__,
		       mock_variables.next,
		       mock_variables.prev);
		printf("%s:%d:%s():list_empty(&mock_variables):%d\n",
		       __FILE__, __LINE__-1, __func__, list_empty(&mock_variables));
		printf("%s:%d:%s():list_size(&mock_variables):%d\n",
		       __FILE__, __LINE__-1, __func__, list_size(&mock_variables));
#endif
	}

	list_for_each_safe(pos, tmp, &mock_variables) {
		struct mock_variable *var = NULL;
		var = list_entry(pos, struct mock_variable, list);

#if (defined(SHIM_DEBUG) && SHIM_DEBUG != 0)
		printf("%s:%d:%s():var:"GUID_FMT"-%s\n",
		       __FILE__, __LINE__-1, __func__,
		       GUID_ARGS(var->guid), Str2str(var->name));
#endif
		mock_delete_variable(var);
	}
	INIT_LIST_HEAD(&mock_variables);
	mock_set_default_usage_limits();

	mock_variable_delete_attr_policy = MOCK_VAR_DELETE_ATTR_ALLOW_ZERO;

	RT->GetVariable = mock_get_variable;
	RT->GetNextVariableName = mock_get_next_variable_name;
	RT->SetVariable = mock_set_variable;
	if (qvi_installed)
		mock_install_query_variable_info();
	else
		mock_uninstall_query_variable_info();
}

void CONSTRUCTOR
mock_reset_config_table(void)
{
	init_efi_system_table();

	/*
	 * Note that BS->InstallConfigurationTable() is *not* defined as
	 * freeing these.  If a test case installs non-malloc()ed tables,
	 * it needs to call BS->InstallConfigurationTable(guid, NULL) to
	 * clear them.
	 */
	for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
		EFI_CONFIGURATION_TABLE *entry = &ST->ConfigurationTable[i];

		if (entry->VendorTable)
			free(entry->VendorTable);
	}

	SetMem(ST->ConfigurationTable,
	       ST->NumberOfTableEntries * sizeof(EFI_CONFIGURATION_TABLE),
	       0);

	ST->NumberOfTableEntries = 0;

	if (ST->ConfigurationTable != mock_config_table) {
		free(ST->ConfigurationTable);
		ST->ConfigurationTable = mock_config_table;
		SetMem(mock_config_table, sizeof(mock_config_table), 0);
	}

	BS->InstallConfigurationTable = mock_install_configuration_table;
}

void DESTRUCTOR
mock_finalize_vars_and_configs(void)
{
	mock_reset_variables();
	mock_reset_config_table();
}

// vim:fenc=utf-8:tw=75:noet
