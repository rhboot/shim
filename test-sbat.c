// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-sbat.c - test our sbat functions.
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

#include <stdio.h>

#define MAX_SIZE 512

list_t sbat_var;

BOOLEAN
secure_mode() {
	return 1;
}

#if 0
/*
 * Mock test helpers
 */
static struct sbat_entry *
create_mock_sbat_entry(const char* comp_name, const char* comp_gen,
		       const char* vend_name, const char* vend_pkg_name,
		       const char* vend_ver, const char* vend_url)
{
	struct sbat_entry *new_entry = AllocatePool(sizeof(*new_entry));
	if (!new_entry)
		return NULL;
	new_entry->component_name = comp_name;
	new_entry->component_generation = comp_gen;
	new_entry->vendor_name = vend_name;
	new_entry->vendor_package_name = vend_pkg_name;
	new_entry->vendor_version = vend_ver;
	new_entry->vendor_url = vend_url;
	return new_entry;
}

void
free_mock_sbat_entry(struct sbat_entry *entry)
{
	if (entry)
		FreePool(entry);
}

static struct sbat *
create_mock_sbat_one_entry(char* comp_name, char* comp_gen, char* vend_name,
			   char* vend_pkg_name, char* vend_ver, char* vend_url)
{
	struct sbat *new_entry = AllocatePool(sizeof(*new_entry));
	if (!new_entry)
		return NULL;
	struct sbat_entry *test_entry;
	struct sbat_entry **entries = AllocatePool(sizeof(*entries));
	if (!entries)
		return NULL;
	test_entry = create_mock_sbat_entry(comp_name, comp_gen, vend_name,
					     vend_pkg_name, vend_ver, vend_url);
	if (!test_entry)
		return NULL;
	entries[0] = test_entry;
	new_entry->size = 1;
	new_entry->entries = entries;
	return new_entry;
}

static struct sbat *
create_mock_sbat_multiple_entries(struct sbat_entry *entry_array,
				  size_t num_elem)
{
	unsigned int i;
	struct sbat *new_entry = AllocatePool(sizeof(*new_entry));
	if (!new_entry)
		return NULL;
	struct sbat_entry *test_entry;
	struct sbat_entry **entries = AllocatePool(num_elem * sizeof(*entries));
	if (!entries)
		return NULL;
	for (i = 0; i < num_elem; i++) {
		test_entry = create_mock_sbat_entry(entry_array[i].component_name,
						    entry_array[i].component_generation,
						    entry_array[i].vendor_name,
						    entry_array[i].vendor_package_name,
						    entry_array[i].vendor_version,
						    entry_array[i].vendor_url);
		if (!test_entry)
			return NULL;
		entries[i] = test_entry;
	}
	new_entry->size = num_elem;
	new_entry->entries = entries;

	return new_entry;
}

void
free_mock_sbat(struct sbat *sbat)
{
	unsigned int i;
	if (sbat) {
		for (i = 0; i < sbat->size; i++) {
			if (sbat->entries[i]) {
				FreePool(sbat->entries[i]);
			}
		}
		FreePool(sbat);
	}
}

static struct sbat_var *
create_mock_sbat_var_entry(const char* comp_name, const char* comp_gen)
{
	struct sbat_var *new_entry = AllocatePool(sizeof(*new_entry));
	if (!new_entry)
		return NULL;
	INIT_LIST_HEAD(&new_entry->list);
	int comp_name_size = strlen(comp_name) + 1;
	CHAR8 *alloc_comp_name = AllocatePool(comp_name_size * sizeof(*alloc_comp_name));
	if (!alloc_comp_name)
		return NULL;
	int comp_gen_size = strlen(comp_gen) + 1;
	CHAR8 *alloc_comp_gen = AllocatePool(comp_gen_size * sizeof(*alloc_comp_gen));
	if (!alloc_comp_gen)
		return NULL;
	CopyMem(alloc_comp_name, comp_name, comp_name_size);
	CopyMem(alloc_comp_gen, comp_gen, comp_gen_size);
	new_entry->component_name = alloc_comp_name;
	new_entry->component_generation = alloc_comp_gen;
	return new_entry;
}

static list_t *
create_mock_sbat_entries_one_entry(char* name, char* gen)
{
	list_t *test_sbat_entries = AllocatePool(sizeof(*test_sbat_entries));
	if (!test_sbat_entries)
		return NULL;
	INIT_LIST_HEAD(test_sbat_entries);
	struct sbat_var *test_entry;
	test_entry = create_mock_sbat_var_entry(name, gen);
	if (!test_entry)
		return NULL;
	list_add(&test_entry->list, test_sbat_entries);
	return test_sbat_entries;
}

static list_t *
create_mock_sbat_entries_multiple_entries(struct sbat_var *var_array,
					  size_t num_elem)
{
	unsigned int i;
	list_t *test_sbat_entries = AllocatePool(sizeof(*test_sbat_entries));
	if (!test_sbat_entries)
		return NULL;
	INIT_LIST_HEAD(test_sbat_entries);
	struct sbat_var *test_entry;
	for (i = 0; i < num_elem; i++) {
		test_entry = create_mock_sbat_var_entry(var_array[i].component_name,
							var_array[i].component_generation);
		if (!test_entry)
			return NULL;
		list_add(&test_entry->list, test_sbat_entries);
	}
	return test_sbat_entries;
}

void
free_mock_sbat_entries(list_t *entries)
{
	list_t *pos = NULL;
	list_t *n = NULL;
	struct sbat_var *entry;

	if (entries)
	{
		list_for_each_safe(pos, n, entries)
		{
			entry = list_entry(pos, struct sbat_var, list);
			list_del(&entry->list);
			if (entry->component_name)
				FreePool((CHAR8 *)entry->component_name);
			if (entry->component_generation)
				FreePool((CHAR8 *)entry->component_generation);
			FreePool(entry);
		}
		FreePool(entries);
	}
}
#endif

/*
 * parse_sbat_section() tests
 */
int
test_parse_sbat_section_null_sbat_base(void)
{
	char *section_base = NULL;
	size_t section_size = 20;
	struct sbat_section_entry **entries;
	size_t n = 0;
	EFI_STATUS status;

	status = parse_sbat_section(section_base, section_size, &n, &entries);
	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_section_zero_sbat_size(void)
{
	char section_base[] = "test1,1,SBAT test1,acme,1,testURL\n";
	size_t section_size = 0;
	struct sbat_section_entry **entries;
	size_t n = 0;
	EFI_STATUS status;

	status = parse_sbat_section(section_base, section_size, &n, &entries);
	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_section_null_entries(void)
{
	char section_base[] = "test1,1,SBAT test1,acme,1,testURL\n";
	/* intentionally not NUL terminated */
	size_t section_size = sizeof(section_base) - 1;
	size_t n = 0;
	EFI_STATUS status;

	status = parse_sbat_section(section_base, section_size, &n, NULL);
	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_section_null_count(void)
{
	char section_base[] = "test1,1,SBAT test1,acme,1,testURL\n";
	/* intentionally not NUL terminated */
	size_t section_size = sizeof(section_base) - 1;
	struct sbat_section_entry **entries;
	EFI_STATUS status;

	status = parse_sbat_section(section_base, section_size, NULL, &entries);
	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_section_no_newline(void)
{
	char section_base[] = "test1,1,SBAT test1,acme,1,testURL";
	/* intentionally not NUL terminated */
	size_t section_size = sizeof(section_base) - 1;
	struct sbat_section_entry **entries;
	size_t n = 0;
	EFI_STATUS status;

	status = parse_sbat_section(section_base, section_size, &n, &entries);
	cleanup_sbat_section_entries(n, entries);
	assert_equal_return(status, EFI_SUCCESS, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_section_no_commas(void)
{
	char section_base[] = "test1";
	/* intentionally not NUL terminated */
	size_t section_size = sizeof(section_base) - 1;
	struct sbat_section_entry **entries;
	size_t n = 0;
	EFI_STATUS status;

	status = parse_sbat_section(section_base, section_size, &n, &entries);
	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_section_too_few_elem(void)
{
	char section_base[] = "test1,1,acme";
	/* intentionally not NUL terminated */
	size_t section_size = sizeof(section_base) - 1;
	struct sbat_section_entry **entries;
	size_t n = 0;
	EFI_STATUS status;

	status = parse_sbat_section(section_base, section_size, &n, &entries);
	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_section_too_many_elem(void)
{
	char section_base[] = "test1,1,SBAT test1,acme1,1,testURL1,other1,stuff,is,here\n"
			      "test2,2,SBAT test2,acme2,2,testURL2,other2";
	/* intentionally not NUL terminated */
	size_t section_size = sizeof(section_base) - 1;
	struct sbat_section_entry **entries;
	size_t n = 0, i;
	list_t *pos = NULL;
	EFI_STATUS status;
	struct sbat_section_entry test_section_entry1 = {
		"test1", "1", "SBAT test1", "acme1", "1", "testURL1"
	};
	struct sbat_section_entry test_section_entry2 = {
		"test2", "2", "SBAT test2", "acme2", "2", "testURL2"
	};
	struct sbat_section_entry *test_entries[] = {
		&test_section_entry1, &test_section_entry2,
	};
	int rc = -1;

	status = parse_sbat_section(section_base, section_size, &n, &entries);
	assert_equal_return(status, EFI_SUCCESS, -1, "got %#hhx expected %#hhx\n");

	for (i = 0; i < n; i++) {
		struct sbat_section_entry *entry = entries[i];
		struct sbat_section_entry *test_entry = test_entries[i];

#define mkassert(a) \
	assert_equal_goto(strcmp(entry-> a, test_entry-> a), 0, fail, \
		"got %zu expected %d\n")

		mkassert(component_name);
		mkassert(component_generation);
		mkassert(vendor_name);
		mkassert(vendor_package_name);
		mkassert(vendor_version);
		mkassert(vendor_url);

#undef mkassert
	}
	assert_equal_goto(n, 2, fail, "got %zu expected %d\n");
	rc = 0;
fail:
	cleanup_sbat_section_entries(n, entries);
	return rc;
}

/*
 * parse_sbat_var() tests
 */
int
test_parse_sbat_var_null_list(void)
{
	EFI_STATUS status;

	INIT_LIST_HEAD(&sbat_var);
	status = parse_sbat_var(NULL);
	cleanup_sbat_var(&sbat_var);
	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_var_data_null_list(void)
{
	char sbat_var_data[] = "test1,1,2021022400";
	/*
	 * intentionally including the NUL termination, because
	 * get_variable() will always include it.
	 */
	size_t sbat_var_data_size = sizeof(sbat_var_data);
	EFI_STATUS status;

	INIT_LIST_HEAD(&sbat_var);
	status = parse_sbat_var_data(NULL, sbat_var_data, sbat_var_data_size);
	cleanup_sbat_var(&sbat_var);

	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_var_data_null_data(void)
{
	size_t sbat_var_data_size = 4;
	EFI_STATUS status;

	INIT_LIST_HEAD(&sbat_var);
	status = parse_sbat_var_data(&sbat_var, NULL, sbat_var_data_size);
	cleanup_sbat_var(&sbat_var);

	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_var_data_zero_size(void)
{
	char sbat_var_data[] = "test1,1,2021022400";
	EFI_STATUS status;

	INIT_LIST_HEAD(&sbat_var);
	status = parse_sbat_var_data(&sbat_var, sbat_var_data, 0);
	cleanup_sbat_var(&sbat_var);

	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

int
test_parse_sbat_var_data(void)
{
	char sbat_var_data[] = "test1,1,2021022400";
	EFI_STATUS status;

	INIT_LIST_HEAD(&sbat_var);
	status = parse_sbat_var_data(&sbat_var, sbat_var_data, 0);

	assert_equal_return(status, EFI_INVALID_PARAMETER, -1, "got %#hhx expected %#hhx\n");

	return 0;
}

/*
 * verify_sbat() tests
 * Note: verify_sbat also frees the underlying "sbat_entries" memory.
 */
int
test_verify_sbat_null_sbat_section(void)
{
	char sbat_var_data[] = "test1,1";
	EFI_STATUS status;
	list_t test_sbat_var;
	size_t n = 0;
	struct sbat_section_entry **entries = NULL;
	int rc = -1;

	INIT_LIST_HEAD(&test_sbat_var);
	status = parse_sbat_var_data(&test_sbat_var, sbat_var_data, sizeof(sbat_var_data));
	assert_equal_goto(status, EFI_SUCCESS, err, "got %#x expected %#x\n");

	status = verify_sbat_helper(&test_sbat_var, n, entries);
	assert_equal_goto(status, EFI_SUCCESS, err, "got %#x expected %#x\n");
	rc = 0;
err:
	cleanup_sbat_var(&test_sbat_var);

	return rc;
}

#if 0
int
test_verify_sbat_null_sbat_entries(void)
{
	struct sbat *test_sbat;
	test_sbat = create_mock_sbat_one_entry("test1","1","SBAT test1",
					       "acme","1","testURL");
	if (!test_sbat)
		return -1;

	list_t sbat_entries;
	INIT_LIST_HEAD(&sbat_entries);
	EFI_STATUS status;

	status = verify_sbat(test_sbat, &sbat_entries);

	assert(status == EFI_INVALID_PARAMETER);
	free_mock_sbat(test_sbat);
	return 0;
}

int
test_verify_sbat_match_one_exact(void)
{
	struct sbat *test_sbat;
	struct sbat_entry sbat_entry_array[2];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "1";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "2";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array, 2);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	test_sbat_entries = create_mock_sbat_entries_one_entry("test1", "1");
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SUCCESS);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}

int
test_verify_sbat_match_one_higher(void)
{
	struct sbat *test_sbat;
	struct sbat_entry sbat_entry_array[2];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "1";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "2";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array, 2);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	test_sbat_entries = create_mock_sbat_entries_one_entry("test2", "1");
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SUCCESS);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}

int
test_verify_sbat_reject_one(void)
{
	struct sbat *test_sbat;
	struct sbat_entry sbat_entry_array[2];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "1";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "2";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array, 2);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	test_sbat_entries = create_mock_sbat_entries_one_entry("test2", "3");
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SECURITY_VIOLATION);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}

int
test_verify_sbat_reject_many(void)
{
	struct sbat *test_sbat;
	unsigned int sbat_entry_array_size = 2;
	struct sbat_entry sbat_entry_array[sbat_entry_array_size];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "1";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "2";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array,
						      sbat_entry_array_size);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	unsigned int sbat_var_array_size = 2;
	struct sbat_var sbat_var_array[sbat_var_array_size];
	sbat_var_array[0].component_name = "test1";
	sbat_var_array[0].component_generation = "1";
	sbat_var_array[1].component_name = "test2";
	sbat_var_array[1].component_generation = "3";
	test_sbat_entries = create_mock_sbat_entries_multiple_entries(sbat_var_array,
								      sbat_var_array_size);
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SECURITY_VIOLATION);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}

int
test_verify_sbat_match_many_higher(void)
{
	struct sbat *test_sbat;
	unsigned int sbat_entry_array_size = 2;
	struct sbat_entry sbat_entry_array[sbat_entry_array_size];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "3";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "5";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array,
						      sbat_entry_array_size);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	unsigned int sbat_var_array_size = 2;
	struct sbat_var sbat_var_array[sbat_var_array_size];
	sbat_var_array[0].component_name = "test1";
	sbat_var_array[0].component_generation = "1";
	sbat_var_array[1].component_name = "test2";
	sbat_var_array[1].component_generation = "2";
	test_sbat_entries = create_mock_sbat_entries_multiple_entries(sbat_var_array,
								      sbat_var_array_size);
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SUCCESS);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}

int
test_verify_sbat_match_many_exact(void)
{
	struct sbat *test_sbat;
	unsigned int sbat_entry_array_size = 2;
	struct sbat_entry sbat_entry_array[sbat_entry_array_size];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "1";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "2";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array,
						      sbat_entry_array_size);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	unsigned int sbat_var_array_size = 2;
	struct sbat_var sbat_var_array[sbat_var_array_size];
	sbat_var_array[0].component_name = "test1";
	sbat_var_array[0].component_generation = "1";
	sbat_var_array[1].component_name = "test2";
	sbat_var_array[1].component_generation = "2";
	test_sbat_entries = create_mock_sbat_entries_multiple_entries(sbat_var_array,
								      sbat_var_array_size);
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SUCCESS);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}

int
test_verify_sbat_reject_many_all(void)
{
	struct sbat *test_sbat;
	unsigned int sbat_entry_array_size = 2;
	struct sbat_entry sbat_entry_array[sbat_entry_array_size];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "1";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "2";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array,
						      sbat_entry_array_size);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	unsigned int sbat_var_array_size = 2;
	struct sbat_var sbat_var_array[sbat_var_array_size];
	sbat_var_array[0].component_name = "test1";
	sbat_var_array[0].component_generation = "3";
	sbat_var_array[1].component_name = "test2";
	sbat_var_array[1].component_generation = "5";
	test_sbat_entries = create_mock_sbat_entries_multiple_entries(sbat_var_array,
								      sbat_var_array_size);
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SECURITY_VIOLATION);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}

int
test_verify_sbat_match_diff_name(void)
{
	struct sbat *test_sbat;
	unsigned int sbat_entry_array_size = 2;
	struct sbat_entry sbat_entry_array[sbat_entry_array_size];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "1";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "2";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array,
						      sbat_entry_array_size);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	unsigned int sbat_var_array_size = 2;
	struct sbat_var sbat_var_array[sbat_var_array_size];
	sbat_var_array[0].component_name = "foo";
	sbat_var_array[0].component_generation = "5";
	sbat_var_array[1].component_name = "bar";
	sbat_var_array[1].component_generation = "2";
	test_sbat_entries = create_mock_sbat_entries_multiple_entries(sbat_var_array,
								      sbat_var_array_size);
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SUCCESS);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}

int
test_verify_sbat_match_diff_name_mixed(void)
{
	struct sbat *test_sbat;
	unsigned int sbat_entry_array_size = 2;
	struct sbat_entry sbat_entry_array[sbat_entry_array_size];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "1";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "2";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array,
						      sbat_entry_array_size);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	unsigned int sbat_var_array_size = 2;
	struct sbat_var sbat_var_array[sbat_var_array_size];
	sbat_var_array[0].component_name = "test1";
	sbat_var_array[0].component_generation = "1";
	sbat_var_array[1].component_name = "bar";
	sbat_var_array[1].component_generation = "2";
	test_sbat_entries = create_mock_sbat_entries_multiple_entries(sbat_var_array,
								      sbat_var_array_size);
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SUCCESS);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}

int
test_verify_sbat_reject_diff_name_mixed(void)
{
	struct sbat *test_sbat;
	unsigned int sbat_entry_array_size = 2;
	struct sbat_entry sbat_entry_array[sbat_entry_array_size];
	sbat_entry_array[0].component_name = "test1";
	sbat_entry_array[0].component_generation = "1";
	sbat_entry_array[0].vendor_name = "SBAT test1";
	sbat_entry_array[0].vendor_package_name = "acme";
	sbat_entry_array[0].vendor_version = "1";
	sbat_entry_array[0].vendor_url = "testURL";
	sbat_entry_array[1].component_name = "test2";
	sbat_entry_array[1].component_generation = "2";
	sbat_entry_array[1].vendor_name = "SBAT test2";
	sbat_entry_array[1].vendor_package_name = "acme2";
	sbat_entry_array[1].vendor_version = "2";
	sbat_entry_array[1].vendor_url = "testURL2";
	test_sbat = create_mock_sbat_multiple_entries(sbat_entry_array,
						      sbat_entry_array_size);
	if (!test_sbat)
		return -1;

	list_t *test_sbat_entries;
	unsigned int sbat_var_array_size = 2;
	struct sbat_var sbat_var_array[sbat_var_array_size];
	sbat_var_array[0].component_name = "test1";
	sbat_var_array[0].component_generation = "5";
	sbat_var_array[1].component_name = "bar";
	sbat_var_array[1].component_generation = "2";
	test_sbat_entries = create_mock_sbat_entries_multiple_entries(sbat_var_array,
								      sbat_var_array_size);
	if (!test_sbat_entries)
		return -1;
	EFI_STATUS status;

	status = verify_sbat(test_sbat, test_sbat_entries);

	assert(status == EFI_SECURITY_VIOLATION);
	free_mock_sbat(test_sbat);
	free_mock_sbat_entries(test_sbat_entries);
	return 0;
}
#endif

int
test_parse_and_verify(void)
{
	EFI_STATUS status;
	char sbat_section[] =
		"test1,1,SBAT test1,acme1,1,testURL1\n"
		"test2,2,SBAT test2,acme2,2,testURL2\n";
	struct sbat_section_entry **section_entries = NULL;
	size_t n_section_entries = 0, i;
	struct sbat_section_entry test_section_entry1 = {
		"test1", "1", "SBAT test1", "acme1", "1", "testURL1"
	};
	struct sbat_section_entry test_section_entry2 = {
		"test2", "2", "SBAT test2", "acme2", "2", "testURL2"
	};
	struct sbat_section_entry *test_entries[] = {
		&test_section_entry1, &test_section_entry2,
	};
	int rc = -1;

	status = parse_sbat_section(sbat_section, sizeof(sbat_section)-1,
	                            &n_section_entries, &section_entries);
	eassert(status == EFI_SUCCESS, "expected %d got %d\n",
		EFI_SUCCESS, status);
	eassert(section_entries != NULL, "expected non-NULL got NULL\n");

	for (i = 0; i < n_section_entries; i++) {
		struct sbat_section_entry *entry = section_entries[i];
		struct sbat_section_entry *test_entry = test_entries[i];

#define mkassert(a) \
	eassert(strcmp(entry-> a, test_entry-> a) == 0, \
		"expected \"%s\" got \"%s\"\n", \
		test_entry-> a, entry-> a )

		mkassert(component_name);
		mkassert(component_generation);
		mkassert(vendor_name);
		mkassert(vendor_package_name);
		mkassert(vendor_version);
		mkassert(vendor_url);

#undef mkassert
	}

	eassert(n_section_entries == 2, "expected %d got %d\n",
		2, n_section_entries);

	char sbat_var_data[] = "test1,5\nbar,2\n";
	size_t sbat_var_data_size = sizeof(sbat_var_data);
	char *sbat_var_alloced = calloc(1, sbat_var_data_size);
	if (!sbat_var_alloced)
		return -1;
	memcpy(sbat_var_alloced, sbat_var_data, sbat_var_data_size);

	INIT_LIST_HEAD(&sbat_var);
	status = parse_sbat_var_data(&sbat_var, sbat_var_alloced, sbat_var_data_size);
	free(sbat_var_alloced);
	if (status != EFI_SUCCESS || list_empty(&sbat_var))
		return -1;

	status = verify_sbat(n_section_entries, section_entries);
	assert_equal_goto(status, EFI_SECURITY_VIOLATION, err, "expected %#x got %#x\n");

	rc = 0;
err:
	cleanup_sbat_section_entries(n_section_entries, section_entries);
	cleanup_sbat_var(&sbat_var);

	return rc;
}

int
test_preserve_sbat_uefi_variable_good(void)
{
	char sbat[] =    "sbat,1,2021030218\ncomponent,2,\n";
	char sbatvar[] = "sbat,1,2021030218\ncomponent,2,\n";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = SBAT_VAR_ATTRS;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return 0;
	else
		return -1;
}

int
test_preserve_sbat_uefi_variable_version_newer(void)
{
	char sbat[] =    "sbat,2,2022030218\ncomponent,2,\n";
	char sbatvar[] = "sbat,1,2021030218\ncomponent,2,\n";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = SBAT_VAR_ATTRS;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return 0;
	else
		return -1;
}

int
test_preserve_sbat_uefi_variable_version_newerlonger(void)
{
	char sbat[] =    "sbat,10,2022030218\ncomponent,2,\n";
	char sbatvar[] = "sbat,2,2021030218\ncomponent,2,\n";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = SBAT_VAR_ATTRS;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return 0;
	else
		return -1;
}

int
test_preserve_sbat_uefi_variable_version_older(void)
{
	char sbat[] =    "sbat,1,2021030218\ncomponent,2,\n";
	char sbatvar[] = "sbat,2,2022030218\ncomponent,2,\n";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = SBAT_VAR_ATTRS;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return -1;
	else
		return 0;
}

int
test_preserve_sbat_uefi_variable_version_olderlonger(void)
{
	char sbat[] =    "sbat,2,2021030218\ncomponent,2,\n";
	char sbatvar[] = "sbat,10,2022030218\ncomponent,2,\n";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = SBAT_VAR_ATTRS;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return -1;
	else
		return 0;
}


int
test_preserve_sbat_uefi_variable_newer(void)
{
	char sbat[] =    "sbat,1,2021030218\ncomponent,2,\n";
	char sbatvar[] = "sbat,1,2025030218\ncomponent,5,\ncomponent,3";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = SBAT_VAR_ATTRS;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return -1;
	else
		return 0;
}
int
test_preserve_sbat_uefi_variable_older(void)
{
	char sbat[] =    "sbat,1,2025030218\ncomponent,2,\ncomponent,3";
	char sbatvar[] = "sbat,1,2020030218\ncomponent,1,\n";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = SBAT_VAR_ATTRS;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return 0;
	else
		return -1;
}

int
test_preserve_sbat_uefi_variable_bad_sig(void)
{
	char sbat[] = "bad_sig,1,2021030218\ncomponent,2,\n";
	char sbatvar[] = "sbat,1,2021030218\n";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = SBAT_VAR_ATTRS;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return -1;
	else
		return 0;
}

int
test_preserve_sbat_uefi_variable_bad_attr(void)
{
	char sbat[] = "sbat,1,2021030218\ncomponent,2,\n";
	char sbatvar[] = "sbat,1,2021030218\n";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = 0;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return -1;
	else
		return 0;
}

int
test_preserve_sbat_uefi_variable_bad_short(void)
{
	char sbat[] = "sba";
	char sbatvar[] = "sbat,1,2021030218\n";
	size_t sbat_size = sizeof(sbat);
	UINT32 attributes = SBAT_VAR_ATTRS;

	if (preserve_sbat_uefi_variable(sbat, sbat_size, attributes, sbatvar))
		return -1;
	else
		return 0;
}

int
main(void)
{
	int status = 0;
	// parse_sbat section tests
	test(test_parse_sbat_section_null_sbat_base);
	test(test_parse_sbat_section_zero_sbat_size);
	test(test_parse_sbat_section_null_entries);
	test(test_parse_sbat_section_null_count);
	test(test_parse_sbat_section_no_newline);
	test(test_parse_sbat_section_no_commas);
	test(test_parse_sbat_section_too_few_elem);
	test(test_parse_sbat_section_too_many_elem);

	// parse_sbat_var tests
	test(test_parse_sbat_var_null_list);
	test(test_parse_sbat_var_data_null_list);
	test(test_parse_sbat_var_data_null_data);
	test(test_parse_sbat_var_data_zero_size);

	// verify_sbat tests
	test(test_verify_sbat_null_sbat_section);
#if 0
	test(test_verify_sbat_null_sbat_entries);
	test(test_verify_sbat_match_one_exact);
	test(test_verify_sbat_match_one_higher);
	test(test_verify_sbat_reject_one);
	test(test_verify_sbat_reject_many);
	test(test_verify_sbat_match_many_higher);
	test(test_verify_sbat_match_many_exact);
	test(test_verify_sbat_reject_many_all);
	test(test_verify_sbat_match_diff_name);
	test(test_verify_sbat_match_diff_name_mixed);
	test(test_verify_sbat_reject_diff_name_mixed);
#endif
	test(test_parse_and_verify);

	test(test_preserve_sbat_uefi_variable_good);
	test(test_preserve_sbat_uefi_variable_newer);
	test(test_preserve_sbat_uefi_variable_older);
	test(test_preserve_sbat_uefi_variable_bad_sig);
	test(test_preserve_sbat_uefi_variable_bad_attr);
	test(test_preserve_sbat_uefi_variable_bad_short);
	test(test_preserve_sbat_uefi_variable_version_newer);
	test(test_preserve_sbat_uefi_variable_version_newerlonger);
	test(test_preserve_sbat_uefi_variable_version_older);
	test(test_preserve_sbat_uefi_variable_version_olderlonger);

	return 0;
}

// vim:fenc=utf-8:tw=75:noet
