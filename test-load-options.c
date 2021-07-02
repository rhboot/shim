// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-argv.c - test our loader_opts parsing
 */
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wunused-function"

#pragma GCC diagnostic warning "-Wcpp"

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif

#include "shim.h"

#include <execinfo.h>
#include <stdio.h>
#include <efivar/efivar.h>

EFI_DEVICE_PATH *
make_file_dp(char *filename)
{
	void *filedp = NULL;
	size_t filedpsz = 0, filedpneeded = 0;

	filedpneeded = efidp_make_file(filedp, filedpneeded, filename);
	assert_positive_return(filedpneeded, NULL, "\n");

	filedp = calloc(1, filedpneeded + 4);
	assert_nonzero_return(filedp, NULL, "\n");

	filedpsz = efidp_make_file(filedp, filedpneeded, filename);
	assert_equal_goto(filedpsz, filedpneeded, err, "got %zu expected %zu\n");

	efidp_make_end_entire((uint8_t *)filedp + filedpneeded, 4);

	return filedp;
err:
	free(filedp);
	return NULL;
}

int
test_parse_load_options(char *load_option_data,
			size_t load_option_data_size,
			char *file_path,
			CHAR16 *target_second_stage,
			char *target_remaining,
			size_t target_remaining_size)
{
	EFI_STATUS status = EFI_SUCCESS;
	EFI_LOADED_IMAGE li = {
		.LoadOptions = load_option_data,
		.LoadOptionsSize = load_option_data_size,
		.FilePath = make_file_dp(file_path),
	};
	CHAR16 *dummy_second_stage = calloc(1, 8);
	int rc = -1;

	assert_nonzero_goto(li.FilePath, err, "\n");
	assert_nonzero_goto(dummy_second_stage, err, "\n");

	StrCat(dummy_second_stage, L"foo");
	second_stage = dummy_second_stage;

	status = parse_load_options(&li);
	assert_false_goto(EFI_ERROR(status), err, "\n");

	assert_nonzero_goto(second_stage, err, "\n");
	assert_not_equal_goto(second_stage, dummy_second_stage, err, "%p == %p\n");
	assert_zero_goto(StrnCmp(second_stage, target_second_stage, 90),
			 err_print_second_stage, "%d != 0\n");

	assert_equal_goto(load_options_size, target_remaining_size, err_remaining, "%zu != %zu\n");
	assert_equal_goto(load_options, target_remaining, err_remaining, "%p != %p\n");
	assert_zero_goto(memcmp(load_options, target_remaining, load_options_size), err_remaining, "\n");

	rc = 0;
err_remaining:
	if (rc != 0) {
		printf("expected remaining:%p\n", target_remaining);
		for (size_t i = 0; i < target_remaining_size; i++)
			printf("0x%02hhx ", target_remaining[i]);
		printf("\n");
		printf("actual remaining:%p\n", load_options);
		for (size_t i = 0; i < load_options_size; i++)
			printf("0x%02hhx ", ((char *)load_options)[i]);
		printf("\n");
	}
err_print_second_stage:
	if (rc != 0) {
		printf("second stage:\"");
		for(int i = 0; second_stage[i] != 0; i++)
			printf("%c", second_stage[i]);
		printf("\"\nexpected:\"");
		for(int i = 0; target_second_stage[i] != 0; i++)
			printf("%c", target_second_stage[i]);
		printf("\"\n");
	}
err:
	if (rc != 0) {
		print_traceback(0);
	}
	if (li.FilePath) {
		free(li.FilePath);
	}
	if (dummy_second_stage && dummy_second_stage != second_stage) {
		free(dummy_second_stage);
	}
	second_stage = NULL;

	return rc;
}

int __attribute__((__flatten__))
test_efi_shell_0(void)
{
/*
00000000  5c 00 45 00 46 00 49 00  5c 00 66 00 65 00 64 00  |\.E.F.I.\.f.e.d.|
00000010  6f 00 72 00 61 00 5c 00  73 00 68 00 69 00 6d 00  |o.r.a.\.s.h.i.m.|
00000020  78 00 36 00 34 00 2e 00  65 00 66 00 69 00 20 00  |x.6.4...e.f.i. .|
00000030  5c 00 45 00 46 00 49 00  5c 00 66 00 65 00 64 00  |\.E.F.I.\.f.e.d.|
00000040  6f 00 72 00 61 00 5c 00  66 00 77 00 75 00 70 00  |o.r.a.\.f.w.u.p.|
00000050  64 00 61 00 74 00 65 00  2e 00 65 00 66 00 69 00  |d.a.t.e...e.f.i.|
00000060  20 00 00 00 66 00 73 00  30 00 3a 00 5c 00 00 00  | ...f.s.0.:.\...|
*/

	char load_option_data[] = {
		0x5c, 0x00, 0x45, 0x00, 0x46, 0x00, 0x49, 0x00,
		0x5c, 0x00, 0x66, 0x00, 0x65, 0x00, 0x64, 0x00,
		0x6f, 0x00, 0x72, 0x00, 0x61, 0x00, 0x5c, 0x00,
		0x73, 0x00, 0x68, 0x00, 0x69, 0x00, 0x6d, 0x00,
		0x78, 0x00, 0x36, 0x00, 0x34, 0x00, 0x2e, 0x00,
		0x65, 0x00, 0x66, 0x00, 0x69, 0x00, 0x20, 0x00,
		0x5c, 0x00, 0x45, 0x00, 0x46, 0x00, 0x49, 0x00,
		0x5c, 0x00, 0x66, 0x00, 0x65, 0x00, 0x64, 0x00,
		0x6f, 0x00, 0x72, 0x00, 0x61, 0x00, 0x5c, 0x00,
		0x66, 0x00, 0x77, 0x00, 0x75, 0x00, 0x70, 0x00,
		0x64, 0x00, 0x61, 0x00, 0x74, 0x00, 0x65, 0x00,
		0x2e, 0x00, 0x65, 0x00, 0x66, 0x00, 0x69, 0x00,
		0x20, 0x00, 0x00, 0x00, 0x66, 0x00, 0x73, 0x00,
		0x30, 0x00, 0x3a, 0x00, 0x5c, 0x00, 0x00, 0x00,
	};
	size_t load_option_data_size = sizeof(load_option_data);
	char *remaining = &load_option_data[sizeof(load_option_data)-14];
	size_t remaining_size = 14;

	return test_parse_load_options(load_option_data,
				       load_option_data_size,
				       "\\EFI\\fedora\\shimx64.efi",
				       L"\\EFI\\fedora\\fwupdate.efi",
				       remaining, remaining_size);
}

int __attribute__((__flatten__))
test_bds_0(void)
{
/*
00000000  01 00 00 00 62 00 4c 00  69 00 6e 00 75 00 78 00  |....b.L.i.n.u.x.|
00000010  20 00 46 00 69 00 72 00  6d 00 77 00 61 00 72 00  | .F.i.r.m.w.a.r.|
00000020  65 00 20 00 55 00 70 00  64 00 61 00 74 00 65 00  |e. .U.p.d.a.t.e.|
00000030  72 00 00 00 40 01 2a 00  01 00 00 00 00 08 00 00  |r.....*.........|
00000040  00 00 00 00 00 40 06 00  00 00 00 00 1a 9e 55 bf  |.....@........U.|
00000050  04 57 f2 4f b4 4a ed 26  4a 40 6a 94 02 02 04 04  |.W.O.:.&J@j.....|
00000060  34 00 5c 00 45 00 46 00  49 00 5c 00 66 00 65 00  |4.\.E.F.I.\.f.e.|
00000070  64 00 6f 00 72 00 61 00  5c 00 73 00 68 00 69 00  |d.o.r.a.\.s.h.i.|
00000080  6d 00 78 00 36 00 34 00  2e 00 65 00 66 00 69 00  |m.x.6.4...e.f.i.|
00000090  00 00 7f ff 04 00 20 00  5c 00 66 00 77 00 75 00  |...... .\.f.w.u.|
000000a0  70 00 78 00 36 00 34 00  2e 00 65 00 66 00 69 00  |p.x.6.4...e.f.i.|
000000b0  00 00                                             |..|
*/
	char load_option_data [] = {
		0x01, 0x00, 0x00, 0x00, 0x62, 0x00, 0x4c, 0x00,
		0x69, 0x00, 0x6e, 0x00, 0x75, 0x00, 0x78, 0x00,
		0x20, 0x00, 0x46, 0x00, 0x69, 0x00, 0x72, 0x00,
		0x6d, 0x00, 0x77, 0x00, 0x61, 0x00, 0x72, 0x00,
		0x65, 0x00, 0x20, 0x00, 0x55, 0x00, 0x70, 0x00,
		0x64, 0x00, 0x61, 0x00, 0x74, 0x00, 0x65, 0x00,
		0x72, 0x00, 0x00, 0x00, 0x40, 0x01, 0x2a, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x06, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x1a, 0x9e, 0x55, 0xbf,
		0x04, 0x57, 0xf2, 0x4f, 0xb4, 0x4a, 0xed, 0x26,
		0x4a, 0x40, 0x6a, 0x94, 0x02, 0x02, 0x04, 0x04,
		0x34, 0x00, 0x5c, 0x00, 0x45, 0x00, 0x46, 0x00,
		0x49, 0x00, 0x5c, 0x00, 0x66, 0x00, 0x65, 0x00,
		0x64, 0x00, 0x6f, 0x00, 0x72, 0x00, 0x61, 0x00,
		0x5c, 0x00, 0x73, 0x00, 0x68, 0x00, 0x69, 0x00,
		0x6d, 0x00, 0x78, 0x00, 0x36, 0x00, 0x34, 0x00,
		0x2e, 0x00, 0x65, 0x00, 0x66, 0x00, 0x69, 0x00,
		0x00, 0x00, 0x7f, 0xff, 0x04, 0x00, 0x20, 0x00,
		0x5c, 0x00, 0x66, 0x00, 0x77, 0x00, 0x75, 0x00,
		0x70, 0x00, 0x78, 0x00, 0x36, 0x00, 0x34, 0x00,
		0x2e, 0x00, 0x65, 0x00, 0x66, 0x00, 0x69, 0x00,
		0x00, 0x00
	};
	size_t load_option_data_size = sizeof(load_option_data);

	return test_parse_load_options(load_option_data,
				       load_option_data_size,
				       "\\EFI\\fedora\\shimx64.efi",
				       L"\\fwupx64.efi",
				       NULL, 0);
}

int __attribute__((__flatten__))
test_bds_1(void)
{
/*
00000000  5c 00 66 00 77 00 75 00  70 00 78 00 36 00 34 00  |\.f.w.u.p.x.6.4.|
00000010  2e 00 65 00 66 00 69 00  00 00                    |..e.f.i...|
0000001a
*/
	char load_option_data [] = {
		0x5c, 0x00, 0x66, 0x00, 0x77, 0x00, 0x75, 0x00,
		0x70, 0x00, 0x78, 0x00, 0x36, 0x00, 0x34, 0x00,
		0x2e, 0x00, 0x65, 0x00, 0x66, 0x00, 0x69, 0x00,
		0x00, 0x00
	};
	size_t load_option_data_size = sizeof(load_option_data);

	return test_parse_load_options(load_option_data,
				       load_option_data_size,
				       "\\EFI\\fedora\\shimx64.efi",
				       L"\\fwupx64.efi",
				       NULL, 0);
}

int
test_bds_2(void)
{
/*
00000000  74 00 65 00 73 00 74 00  2E 00 65 00 66 00 69 00  |t.e.s.t...e.f.i.|
00000010  20 00 6F 00 6E 00 65 00  20 00 74 00 77 00 6F 00  |..o.n.e...t.w.o.|
00000020  20 00 74 00 68 00 72 00  65 00 65 00 00 00        |..t.h.r.e.e...|
*/
	char load_option_data [] = {
		0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
		0x2E, 0x00, 0x65, 0x00, 0x66, 0x00, 0x69, 0x00,
		0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x65, 0x00,
		0x20, 0x00, 0x74, 0x00, 0x77, 0x00, 0x6F, 0x00,
		0x20, 0x00, 0x74, 0x00, 0x68, 0x00, 0x72, 0x00,
		0x65, 0x00, 0x65, 0x00, 0x00, 0x00
	};
	size_t load_option_data_size = sizeof(load_option_data);
	char *target_remaining = &load_option_data[26];
	size_t target_remaining_size = 20;

	return test_parse_load_options(load_option_data,
				       load_option_data_size,
				       "test.efi",
				       L"one",
				       target_remaining,
				       target_remaining_size);
}

int
main(void)
{
	int status = 0;
	test(test_efi_shell_0);
	test(test_bds_0);
	test(test_bds_1);
	test(test_bds_2);
	return status;
}

// vim:fenc=utf-8:tw=75:noet
