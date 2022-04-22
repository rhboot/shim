// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * test.c - stuff we need for test harnesses
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

#include <execinfo.h>
#include <stdio.h>
#include <string.h>

#define BT_BUF_SIZE (4096/sizeof(void *))

static void *frames[BT_BUF_SIZE] = { 0, };

UINT8 in_protocol = 0;
int debug = DEFAULT_DEBUG_PRINT_STATE;

void
print_traceback(int skip)
{
	int nptrs;
	char **strings;

	nptrs = backtrace(frames, BT_BUF_SIZE);
	if (nptrs < skip)
		return;

	strings = backtrace_symbols(frames, nptrs);
	for (int i = skip; strings != NULL && i < nptrs; i++) {
		printf("%p %s\n", (void *)frames[i], strings[i]);
	}
	if (strings)
		free(strings);
}

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

void EFIAPI
mock_efi_void()
{
	;
}

EFI_STATUS EFIAPI
mock_efi_success()
{
	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
mock_efi_unsupported()
{
	return EFI_UNSUPPORTED;
}

EFI_STATUS EFIAPI
mock_efi_not_found()
{
	return EFI_NOT_FOUND;
}

EFI_BOOT_SERVICES mock_bs, mock_default_bs = {
	.Hdr = {
		.Signature = EFI_BOOT_SERVICES_SIGNATURE,
		.Revision = EFI_1_10_BOOT_SERVICES_REVISION,
		.HeaderSize = offsetof(EFI_BOOT_SERVICES, SetMem)
			      + sizeof(mock_bs.SetMem),
	},

	.RaiseTPL = mock_efi_unsupported,
	.RestoreTPL = mock_efi_void,

	.AllocatePages = mock_efi_allocate_pages,
	.FreePages = mock_efi_free_pages,
	.GetMemoryMap = mock_efi_unsupported,
	.AllocatePool = mock_efi_allocate_pool,
	.FreePool = mock_efi_free_pool,

	.CreateEvent = mock_efi_unsupported,
	.SetTimer = mock_efi_unsupported,
	.WaitForEvent = mock_efi_unsupported,
	.SignalEvent = mock_efi_unsupported,
	.CloseEvent = mock_efi_unsupported,
	.CheckEvent = mock_efi_unsupported,

	.InstallProtocolInterface = mock_efi_unsupported,
	.ReinstallProtocolInterface = mock_efi_unsupported,
	.UninstallProtocolInterface = mock_efi_unsupported,
	.HandleProtocol = mock_efi_unsupported,
#if 0
	/*
	 * EFI 1.10 has a "Reserved" field here that's not in later
	 * revisions.
	 *
	 * I don't think it's in any actual *firmware* either.
	 */
	.Reserved = NULL,
#endif
	.RegisterProtocolNotify = mock_efi_unsupported,
	.LocateHandle = mock_efi_not_found,
	.LocateDevicePath = mock_efi_unsupported,
	.InstallConfigurationTable = mock_efi_unsupported,

	.LoadImage = (void *)mock_efi_unsupported,
	.StartImage = mock_efi_unsupported,
	.Exit = mock_efi_unsupported,
	.UnloadImage = mock_efi_unsupported,
	.ExitBootServices = mock_efi_unsupported,

	.GetNextMonotonicCount = mock_efi_unsupported,
	.Stall = mock_efi_unsupported,
	.SetWatchdogTimer = mock_efi_unsupported,

	.ConnectController = (void *)mock_efi_unsupported,
	.DisconnectController = mock_efi_unsupported,

	.OpenProtocol = mock_efi_unsupported,
	.CloseProtocol = mock_efi_unsupported,
	.OpenProtocolInformation = mock_efi_unsupported,

	.ProtocolsPerHandle = mock_efi_unsupported,
	.LocateHandleBuffer = mock_efi_unsupported,
	.LocateProtocol = mock_efi_unsupported,

	.InstallMultipleProtocolInterfaces = (void *)mock_efi_unsupported,
	.UninstallMultipleProtocolInterfaces = (void *)mock_efi_unsupported,

	.CalculateCrc32 = mock_efi_unsupported,

	.CopyMem = NULL,
	.SetMem = NULL,
	.CreateEventEx = mock_efi_unsupported,
};

EFI_RUNTIME_SERVICES mock_rt, mock_default_rt = {
	.Hdr = {
		.Signature = EFI_RUNTIME_SERVICES_SIGNATURE,
		.Revision = EFI_1_10_RUNTIME_SERVICES_REVISION,
		.HeaderSize = offsetof(EFI_RUNTIME_SERVICES, ResetSystem)
			      + sizeof(mock_rt.ResetSystem),
	},

	.GetTime = mock_efi_unsupported,
	.SetTime = mock_efi_unsupported,
	.GetWakeupTime = mock_efi_unsupported,
	.SetWakeupTime = (void *)mock_efi_unsupported,

	.SetVirtualAddressMap = mock_efi_unsupported,
	.ConvertPointer = mock_efi_unsupported,

	.GetVariable = mock_efi_unsupported,
	.SetVariable = mock_efi_unsupported,
	.GetNextVariableName = mock_efi_unsupported,

	.GetNextHighMonotonicCount = mock_efi_unsupported,
	.ResetSystem = mock_efi_unsupported,

	.UpdateCapsule = mock_efi_unsupported,
	.QueryCapsuleCapabilities = mock_efi_unsupported,

	.QueryVariableInfo = mock_efi_unsupported,
};

EFI_SYSTEM_TABLE mock_st, mock_default_st = {
	.Hdr = {
		.Signature = EFI_SYSTEM_TABLE_SIGNATURE,
		.Revision = EFI_1_10_SYSTEM_TABLE_REVISION,
		.HeaderSize = sizeof(EFI_SYSTEM_TABLE),
	},
	.BootServices = &mock_bs,
	.RuntimeServices = &mock_rt,
};

void CONSTRUCTOR
init_efi_system_table(void)
{
	static bool once = true;
	if (once) {
		once = false;
		reset_efi_system_table();
	}
}

void
reset_efi_system_table(void)
{
	ST = &mock_st;
	BS = &mock_bs;
	RT = &mock_rt;

	memcpy(&mock_bs, &mock_default_bs, sizeof(mock_bs));
	memcpy(&mock_rt, &mock_default_rt, sizeof(mock_rt));
	memcpy(&mock_st, &mock_default_st, sizeof(mock_st));
}

EFI_STATUS EFIAPI
LogError_(const char *file, int line, const char *func, const CHAR16 *fmt, ...)
{
	assert(0);
	return EFI_SUCCESS;
}

#ifndef HAVE_SHIM_LOCK_GUID
EFI_GUID SHIM_LOCK_GUID = {0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23 } };
#endif

UINTN EFIAPI
console_print(const CHAR16 *fmt, ...)
{
	return 0;
}

void
console_error(CHAR16 *err, EFI_STATUS efi_status)
{
	return;
}

#ifndef HAVE_START_IMAGE
EFI_STATUS
start_image(EFI_HANDLE image_handle, CHAR16 *ImagePath)
{
	return EFI_UNSUPPORTED;
}
#endif

// vim:fenc=utf-8:tw=75:noet
