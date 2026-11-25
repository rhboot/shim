// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test-pe-reloc.c - attempt to test relocate_coff()
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

static int
test_image_address(void)
{
	char image[4];
	void *ret;

	assert_equal_return(ImageAddress(image, sizeof(image), 0), &image[0], -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress(image, sizeof(image), 4), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)1, 2, 3), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)-1ull, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)0, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)1, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)2, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");
	assert_equal_return(ImageAddress((void *)3, UINT64_MAX, UINT64_MAX), NULL, -1, "got %p expected %p\n");

	return 0;
}

static int
test_nx_header_policy(void)
{
	struct {
		EFI_IMAGE_DOS_HEADER dos;
		EFI_IMAGE_OPTIONAL_HEADER_UNION header;
		EFI_IMAGE_SECTION_HEADER section;
		char padding[512];
	} image = {0};
	PE_COFF_LOADER_IMAGE_CONTEXT context;
	EFI_STATUS status, expected;
	UINT16 *characteristics;

	image.dos.e_magic = EFI_IMAGE_DOS_SIGNATURE;
	image.dos.e_lfanew = offsetof(typeof(image), header);
	image.header.Pe32.Signature = EFI_IMAGE_NT_SIGNATURE;
	image.header.Pe32.FileHeader.NumberOfSections = 1;
#if defined(__x86_64__)
	image.header.Pe32.FileHeader.Machine = IMAGE_FILE_MACHINE_X64;
#elif defined(__i386__)
	image.header.Pe32.FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
#elif defined(__aarch64__)
	image.header.Pe32.FileHeader.Machine = IMAGE_FILE_MACHINE_ARM64;
#elif defined(__arm__)
	image.header.Pe32.FileHeader.Machine = IMAGE_FILE_MACHINE_ARMTHUMB_MIXED;
#elif defined(__riscv) && __riscv_xlen == 64
	image.header.Pe32.FileHeader.Machine = IMAGE_FILE_MACHINE_RISCV64;
#elif defined(__ia64__)
	image.header.Pe32.FileHeader.Machine = IMAGE_FILE_MACHINE_IA64;
#endif
	if (sizeof(void *) == 8) {
		image.header.Pe32.FileHeader.SizeOfOptionalHeader = sizeof(EFI_IMAGE_OPTIONAL_HEADER64);
		image.header.Pe32Plus.OptionalHeader.Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
		image.header.Pe32Plus.OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;
		image.header.Pe32Plus.OptionalHeader.SizeOfHeaders = sizeof(image);
		image.header.Pe32Plus.OptionalHeader.SizeOfImage = 4096;
		characteristics = &image.header.Pe32Plus.OptionalHeader.DllCharacteristics;
	} else {
		image.header.Pe32.FileHeader.SizeOfOptionalHeader = sizeof(EFI_IMAGE_OPTIONAL_HEADER32);
		image.header.Pe32.OptionalHeader.Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
		image.header.Pe32.OptionalHeader.NumberOfRvaAndSizes = EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES;
		image.header.Pe32.OptionalHeader.SizeOfHeaders = sizeof(image);
		image.header.Pe32.OptionalHeader.SizeOfImage = 4096;
		characteristics = &image.header.Pe32.OptionalHeader.DllCharacteristics;
	}

	for (unsigned int policy = 0; policy <= 1; policy++) {
		mok_policy = policy;
		for (unsigned int nx = 0; nx <= 1; nx++) {
			*characteristics = nx ? EFI_IMAGE_DLLCHARACTERISTICS_NX_COMPAT : 0;
			memset(&context, 0, sizeof(context));
			expected = SHIM_NX_COMPAT && !nx ? EFI_UNSUPPORTED : EFI_SUCCESS;
			status = read_header(&image, sizeof(image), &context, true);
			assert_equal_return(status, expected, -1, "got %lx expected %lx\n");
		}
	}
	mok_policy = 0;
	return 0;
}

int
main(void)
{
	int status = 0;
	test(test_image_address);
	test(test_nx_header_policy);

	return status;
}

// vim:fenc=utf-8:tw=75:noet
