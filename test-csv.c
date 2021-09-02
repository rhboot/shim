// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-csv.c - test our csv parser
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

#include <stdio.h>

struct test_entry {
	size_t n_columns;
	char *columns[7];
};

int
test_parse_csv_line_size_0(void)
{
	char *s0 = "";
	char *columns[] = { "a", "b", "c", "d" };
	char *test_columns[] = { NULL, NULL, NULL, NULL };
	size_t n_columns = 3;
	size_t i;

	test_columns[3] = columns[3];

	parse_csv_line(s0, 0, &n_columns, (const char **)columns);

	assert_equal_return(s0[0], '\0', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(n_columns, 0, -1, "got %#hhx expected %#hhx\n");
	for (i = 0; i < 4; i++) {
		assert_equal_return(test_columns[i], columns[i], -1,
		                    "expected %p got %p for column %d\n",
				    i);
	}
	return 0;
}

int
test_parse_csv_line_size_1(void)
{
	char *s0 = "";
	char *columns[] = { "a", "b", "c", "d" };
	char *test_columns[] = { "", NULL, NULL, NULL };
	size_t n_columns = 3;
	size_t max = 1;
	size_t i;

	test_columns[3] = columns[3];

	parse_csv_line(s0, max, &n_columns, (const char **)columns);

	assert_equal_return(s0[0], '\0', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(n_columns, 1, -1, "got %#hhx expected %#hhx\n");
	for (i = 0; i < 4; i++) {
		assert_equal_return(test_columns[i], columns[i], -1,
		                    "expected %p got %p for column %d\n",
				    i);
	}
	return 0;
}

int
test_parse_csv_line_comma_size_1(void)
{
	char *s0;
	char *columns[] = { "a", "b", "c", "d" };
	char *test_columns[] = { "", NULL, NULL, "d" };
	size_t n_columns = 3;
	size_t max = 1;
	size_t i;

	/*
	 * For reasons unknown, when I do this the normal way with:
	 * char *s0 = ",";
	 * gcc is putting it in .rodata,
	 * *** AND combining it with the "," from delims from parse_csv_line***.
	 */
	s0 = alloca(2);
	s0[0] = ',';
	s0[1] = '\0';

	parse_csv_line(s0, max, &n_columns, (const char **)columns);

	assert_equal_return(s0[0], '\0', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(n_columns, 1, -1, "got %#hhx expected %#hhx\n");
//	for (i = 0; i < 4; i++) {
//		printf("columns[%d]:%p:\"%s\"\n", i, columns[i], columns[i]);
//	}
	for (i = 0; i < 1; i++) {
		assert_equal_return(strcmp(test_columns[i], columns[i]), 0, -1,
				    "expected %d got %d for column %d\n", i);
	}
	for (i = 1; i < 3; i++) {
		assert_equal_return(test_columns[i], columns[i], -1,
		                    "expected %p got %p for column %d\n",
				    i);
	}
	for (i = 3; i < 4; i++) {
		assert_equal_return(strcmp(test_columns[i], columns[i]), 0, -1,
				    "expected %d got %d for column %d\n", i);
	}

	return 0;
}

int
test_parse_csv_line_comma_size_2(void)
{
	char *s0;
	char *columns[] = { "a", "b", "c", "d" };
	char *test_columns[] = { "", "", NULL, "d" };
	size_t n_columns = 3;
	size_t max = 2;
	size_t i;

	/*
	 * For reasons unknown, when I do this the normal way with:
	 * char *s0 = ",";
	 * gcc is putting it in .rodata,
	 * *** AND combining it with the "," from delims from parse_csv_line***.
	 */
	s0 = alloca(2);
	s0[0] = ',';
	s0[1] = '\0';

	parse_csv_line(s0, max, &n_columns, (const char **)columns);

	assert_equal_return(s0[0], '\0', -1, "got %#hhx expected %#hhx\n");
	assert_equal_return(n_columns, 2, -1, "got %#hhx expected %#hhx\n");
	for (i = 0; i < 2; i++) {
		assert_equal_return(strcmp(test_columns[i], columns[i]), 0, -1,
				    "expected %d got %d for column %d\n", i);
	}
	for (i = 2; i < 3; i++) {
		assert_equal_return(test_columns[i], columns[i], -1,
		                    "expected %p got %p for column %d\n",
				    i);
	}
	for (i = 3; i < 4; i++) {
		assert_equal_return(strcmp(test_columns[i], columns[i]), 0, -1,
				    "expected %d got %d for column %d\n", i);
	}

	return 0;
}

int
test_csv_0(void)
{
	char csv[] =
		"\000\000\000"
		"a,b,c,d,e,f,g,h\n"
		"a,b,c\n"
		"\n"
		"\n"
		"a,b,c,d,e,f,g,h\n"
		"a,b,c";
	struct test_entry test_entries[]= {
		{ 7, { "a", "b", "c", "d", "e", "f", "g" } },
		{ 3, { "a", "b", "c", NULL, NULL, NULL, NULL } },
		{ 7, { "a", "b", "c", "d", "e", "f", "g" } },
		{ 3, { "a", "b", "c", NULL, NULL, NULL, NULL } },
	};
	list_t entry_list;
	size_t i;
	char *current, *end;
	list_t *pos = NULL;
	EFI_STATUS efi_status;

	INIT_LIST_HEAD(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");

	memcpy(csv, (char [])UTF8_BOM, UTF8_BOM_SIZE);

	current = csv;
	end = csv + sizeof(csv) - 1;

	efi_status = parse_csv_data(current, end, 7, &entry_list);
	assert_equal_return(efi_status, EFI_SUCCESS, -1, "got %x expected %x\n");

	i = 0;
	list_for_each(pos, &entry_list) {
		struct csv_row *csv_row;
		struct test_entry *test_entry = &test_entries[i++];
		size_t j;

		assert_goto(i > 0 && i <= 4, fail, "got %d expected 0 to 4\n", i);

		csv_row = list_entry(pos, struct csv_row, list);

		assert_equal_goto(csv_row->n_columns, test_entry->n_columns,
				  fail, "got %d expected %d\n");
		for (j = 0; j < csv_row->n_columns; j++) {
			assert_equal_goto(strcmp(csv_row->columns[j],
					   test_entry->columns[j]), 0,
				    fail, "got %d expected %d\n");
		}
	}

	assert_equal_return(list_size(&entry_list), 4, -1,
			    "got %d expected %d\n");
	free_csv_list(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");
	return 0;
fail:
	free_csv_list(&entry_list);
	return -1;
}

int
test_csv_1(void)
{
	char csv[] =
		"a,b,c,d,e,f,g,h\n"
		"a,b,c\n"
		"\n"
		"\n"
		"a,b,c,d,e,f,g,h\n"
		"a,b,c";
	struct test_entry test_entries[]= {
		{ 7, { "a", "b", "c", "d", "e", "f", "g" } },
		{ 3, { "a", "b", "c", NULL, NULL, NULL, NULL } },
		{ 7, { "a", "b", "c", "d", "e", "f", "g" } },
		{ 3, { "a", "b", "c", NULL, NULL, NULL, NULL } },
	};
	list_t entry_list;
	size_t i;
	char *current, *end;
	list_t *pos = NULL;
	EFI_STATUS efi_status;

	INIT_LIST_HEAD(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");

	current = csv;
	end = csv + sizeof(csv) - 1;

	efi_status = parse_csv_data(current, end, 7, &entry_list);
	assert_equal_return(efi_status, EFI_SUCCESS, -1, "got %x expected %x\n");

	i = 0;
	list_for_each(pos, &entry_list) {
		struct csv_row *csv_row;
		struct test_entry *test_entry = &test_entries[i++];
		size_t j;

		assert_goto(i > 0 && i <= 4, fail, "got %d expected 0 to 4\n", i);

		csv_row = list_entry(pos, struct csv_row, list);

		assert_equal_goto(csv_row->n_columns, test_entry->n_columns,
				  fail, "got %d expected %d\n");
		for (j = 0; j < csv_row->n_columns; j++) {
			assert_equal_goto(strcmp(csv_row->columns[j],
					   test_entry->columns[j]), 0,
				    fail, "got %d expected %d\n");
		}
	}

	assert_equal_return(list_size(&entry_list), 4, -1,
			    "got %d expected %d\n");
	free_csv_list(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");
	return 0;
fail:
	free_csv_list(&entry_list);
	return -1;
}

int
test_csv_2(void)
{
	char csv[] =
		"\000\000\000"
		"a,b,c,d,e,f,g,h\n"
		",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,c\n"
		"\n"
		"\n"
		"a,b,c,d,e,f,g,h\n"
		"a,b,c";
	struct test_entry test_entries[]= {
		{ 7, { "a", "b", "c", "d", "e", "f", "g" } },
		{ 7, { "", "", "", "", "", "", "" } },
		{ 7, { "a", "b", "c", "d", "e", "f", "g" } },
		{ 3, { "a", "b", "c", NULL, NULL, NULL, NULL } },
	};
	list_t entry_list;
	size_t i;
	char *current, *end;
	list_t *pos = NULL;
	EFI_STATUS efi_status;

	INIT_LIST_HEAD(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");

	memcpy(csv, (char [])UTF8_BOM, UTF8_BOM_SIZE);

	current = csv;
	end = csv + sizeof(csv) - 1;

	efi_status = parse_csv_data(current, end, 7, &entry_list);
	assert_equal_return(efi_status, EFI_SUCCESS, -1, "got %x expected %x\n");

	i = 0;
	list_for_each(pos, &entry_list) {
		struct csv_row *csv_row;
		struct test_entry *test_entry = &test_entries[i++];
		size_t j;

		assert_goto(i > 0 && i <= 7, fail, "got %d expected 0 to 7\n", i);
		csv_row = list_entry(pos, struct csv_row, list);

		assert_equal_goto(csv_row->n_columns, test_entry->n_columns,
				  fail, "got %d expected %d\n");
		for (j = 0; j < csv_row->n_columns; j++) {
			assert_equal_goto(strcmp(csv_row->columns[j],
					   test_entry->columns[j]), 0,
				    fail, "got %d expected %d\n");
		}
	}

	free_csv_list(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");

	return 0;
fail:
	free_csv_list(&entry_list);
	return -1;
}

int
test_csv_3(void)
{
	char csv[] =
		"a,b,c,d,e,f,g,h\n"
		"a,b,c\n"
		"\n"
		"\n"
		"a,b,c,d,e,f,g,h\n"
		"a,b,c\0x,y\0z\0";
	struct test_entry test_entries[]= {
		{ 7, { "a", "b", "c", "d", "e", "f", "g" } },
		{ 3, { "a", "b", "c", NULL, NULL, NULL, NULL } },
		{ 7, { "a", "b", "c", "d", "e", "f", "g" } },
		{ 3, { "a", "b", "c", NULL, NULL, NULL, NULL } },
	};
	list_t entry_list;
	size_t i;
	char *current, *end;
	list_t *pos = NULL;
	EFI_STATUS efi_status;

	INIT_LIST_HEAD(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");

	current = csv;
	end = csv + sizeof(csv) - 1;

	efi_status = parse_csv_data(current, end, 7, &entry_list);
	assert_equal_return(efi_status, EFI_SUCCESS, -1, "got %x expected %x\n");

	i = 0;
	list_for_each(pos, &entry_list) {
		struct csv_row *csv_row;
		struct test_entry *test_entry = &test_entries[i++];
		size_t j;

		assert_goto(i > 0 && i <= 4, fail, "got %d expected 0 to 4\n", i);

		csv_row = list_entry(pos, struct csv_row, list);

		assert_equal_goto(csv_row->n_columns, test_entry->n_columns,
				  fail, "got %d expected %d\n");
		for (j = 0; j < csv_row->n_columns; j++) {
			assert_equal_goto(strcmp(csv_row->columns[j],
					   test_entry->columns[j]), 0,
				    fail, "got %d expected %d\n");
		}
	}

	assert_equal_return(list_size(&entry_list), 4, -1,
			    "got %d expected %d\n");
	free_csv_list(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");
	return 0;
fail:
	free_csv_list(&entry_list);
	return -1;
}

int
test_simple_sbat_csv(void)
{
	char csv[] =
		"test1,1,SBAT test1,acme1,1,testURL1\n"
		"test2,2,SBAT test2,acme2,2,testURL2\n";
	struct test_entry test_entries[]= {
		{ 6, { "test1", "1", "SBAT test1", "acme1", "1", "testURL1" } },
		{ 6, { "test2", "2", "SBAT test2", "acme2", "2", "testURL2" } },
	};
	list_t entry_list;
	size_t i;
	char *current, *end;
	list_t *pos = NULL;
	EFI_STATUS efi_status;

	INIT_LIST_HEAD(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");

	current = csv;
	end = csv + sizeof(csv) - 1;

	efi_status = parse_csv_data(current, end, 6, &entry_list);
	assert_equal_return(efi_status, EFI_SUCCESS, -1,
	                    "got %d expected %d\n");

	i = 0;
	list_for_each(pos, &entry_list) {
		struct csv_row *csv_row;
		struct test_entry *test_entry = &test_entries[i++];
		size_t j;

		csv_row = list_entry(pos, struct csv_row, list);

		assert_equal_goto(csv_row->n_columns, test_entry->n_columns,
		                  fail, "got %d expected %d");

		for (j = 0; j < csv_row->n_columns; j++) {
			assert_equal_goto(strcmp(csv_row->columns[j],
					   test_entry->columns[j]), 0,
				    fail, "got %d expected %d\n");
		}
	}

	assert_equal_return(list_size(&entry_list), 2, -1,
			    "got %d expected %d\n");
	free_csv_list(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");

	return 0;
fail:
	free_csv_list(&entry_list);
	return -1;

}

int
test_csv_simple_fuzz(char *random_bin, size_t random_bin_len,
		     bool assert_entries)
{
	list_t entry_list;
	size_t i;
	char *current, *end;
	list_t *pos = NULL;
	EFI_STATUS efi_status;

	INIT_LIST_HEAD(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");

	current = &random_bin[0];
	current = current + 1 - 1;
	end = current + random_bin_len - 1;
	*end = '\0';

	efi_status = parse_csv_data(current, end, 7, &entry_list);
	assert_equal_return(efi_status, EFI_SUCCESS, -1, "expected %#x got %#x\n");
	printf("parsed %zd entries\n", list_size(&entry_list));
	if (assert_entries)
		assert_goto(list_size(&entry_list) > 0, fail,
				    "expected >0 entries\n");

	i = 0;
	list_for_each(pos, &entry_list) {
		struct csv_row *csv_row;

		csv_row = list_entry(pos, struct csv_row, list);
		dprint("row[%zd]: %zd columns\n", i, csv_row->n_columns);
		i++;
	}

	free_csv_list(&entry_list);
	assert_equal_return(list_size(&entry_list), 0, -1,
			    "got %d expected %d\n");

	return 0;
fail:
	free_csv_list(&entry_list);
	return -1;
}

#include "test-random.h"

int
main(void)
{
	int status = 0;
	size_t i, j;

	setbuf(stdout, NULL);
	test(test_parse_csv_line_size_0);
	test(test_parse_csv_line_size_1);
	test(test_parse_csv_line_comma_size_1);
	test(test_parse_csv_line_comma_size_2);
	test(test_csv_0);
	test(test_csv_1);
	test(test_csv_2);
	test(test_csv_3);
	test(test_simple_sbat_csv);
	test(test_csv_simple_fuzz, random_bin, random_bin_len, false);
	for (i = 0; i < random_bin_len; i++) {
		j = i;
		while (random_bin[i] == '\0')
			random_bin[i] = j++;
	}
	test(test_csv_simple_fuzz, random_bin, random_bin_len, true);

	return status;
}

// vim:fenc=utf-8:tw=75:noet
