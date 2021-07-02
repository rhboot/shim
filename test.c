// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test.c - stuff we need for test harnesses
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif
#include "shim.h"

UINT8 in_protocol = 0;
int debug = DEFAULT_DEBUG_PRINT_STATE;

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"

static EFI_STATUS EFIAPI
mock_efi_allocate_pages(EFI_ALLOCATE_TYPE type,
			EFI_MEMORY_TYPE memory_type,
			UINTN nmemb,
			EFI_PHYSICAL_ADDRESS *memory)
{
	/*
	 * XXX so far this does not honor the type at all, and there's no
	 * tracking for memory_type either.
	 */
	*memory = (EFI_PHYSICAL_ADDRESS)(uintptr_t)calloc(nmemb, 4096);
	if ((void *)(uintptr_t)(*memory) == NULL)
		return EFI_OUT_OF_RESOURCES;

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI
mock_efi_free_pages(EFI_PHYSICAL_ADDRESS memory,
		    UINTN nmemb)
{
	free((void *)(uintptr_t)memory);

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI
mock_efi_allocate_pool(EFI_MEMORY_TYPE pool_type,
		       UINTN size,
		       VOID **buf)
{
	*buf = calloc(1, size);
	if (*buf == NULL)
		return EFI_OUT_OF_RESOURCES;

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI
mock_efi_free_pool(void *buf)
{
	free(buf);

	return EFI_SUCCESS;
}

EFI_BOOT_SERVICES mock_bs = {
	.RaiseTPL = NULL,
	.RestoreTPL = NULL,

	.AllocatePages = mock_efi_allocate_pages,
	.FreePages = mock_efi_free_pages,
	.AllocatePool = mock_efi_allocate_pool,
	.FreePool = mock_efi_free_pool,
};

EFI_RUNTIME_SERVICES mock_rt = {
	.Hdr = { 0, },
	.GetVariable = NULL,
};

EFI_SYSTEM_TABLE mock_st = {
	.Hdr = { 0, },
	.BootServices = &mock_bs,
	.RuntimeServices = &mock_rt,
};

void CONSTRUCTOR init_efi_system_table(void)
{
	ST = &mock_st;
	BS = &mock_bs;
	RT = &mock_rt;
}

EFI_STATUS EFIAPI
LogError_(const char *file, int line, const char *func, const CHAR16 *fmt, ...)
{
	assert(0);
	return EFI_SUCCESS;
}

#ifndef HAVE_GET_VARIABLE_ATTR
EFI_STATUS
get_variable_attr(const CHAR16 * const var, UINT8 **data, UINTN *len,
		  EFI_GUID owner, UINT32 *attributes)
{
	return EFI_UNSUPPORTED;
}
#endif

#ifndef HAVE_GET_VARIABLE
EFI_STATUS
get_variable(const CHAR16 * const var, UINT8 **data, UINTN *len, EFI_GUID owner)
{
	return get_variable_attr(var, data, len, owner, NULL);
}
#endif

#ifndef HAVE_SHIM_LOCK_GUID
EFI_GUID SHIM_LOCK_GUID = {0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23 } };
#endif

UINTN EFIAPI
console_print(const CHAR16 *fmt, ...)
{
	return 0;
}

// vim:fenc=utf-8:tw=75:noet
