// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * errlog.c
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

static CHAR16 **errs = NULL;
static UINTN nerrs = 0;

EFI_STATUS EFIAPI
vdprint_(const CHAR16 *fmt, const char *file, int line, const char *func,
         ms_va_list args)
{
	ms_va_list args2;
	EFI_STATUS efi_status = EFI_SUCCESS;

	if (verbose) {
		ms_va_copy(args2, args);
		console_print(L"%a:%d:%a() ", file, line, func);
		efi_status = VPrint(fmt, args2);
		ms_va_end(args2);
	}
	return efi_status;
}

EFI_STATUS EFIAPI
VLogError(const char *file, int line, const char *func, const CHAR16 *fmt,
          ms_va_list args)
{
	ms_va_list args2;
	CHAR16 **newerrs;

	if (file == NULL || func == NULL || fmt == NULL)
		return EFI_INVALID_PARAMETER;

	newerrs = ReallocatePool(errs, (nerrs + 1) * sizeof(*errs),
				       (nerrs + 3) * sizeof(*errs));
	if (!newerrs)
		return EFI_OUT_OF_RESOURCES;

	newerrs[nerrs] = PoolPrint(L"%a:%d %a() ", file, line, func);
	if (!newerrs[nerrs])
		return EFI_OUT_OF_RESOURCES;
	ms_va_copy(args2, args);
	newerrs[nerrs+1] = VPoolPrint(fmt, args2);
	if (!newerrs[nerrs+1])
		return EFI_OUT_OF_RESOURCES;
	ms_va_end(args2);

	nerrs += 2;
	newerrs[nerrs] = NULL;
	errs = newerrs;

	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI
LogError_(const char *file, int line, const char *func, const CHAR16 *fmt, ...)
{
	ms_va_list args;
	EFI_STATUS efi_status;

	ms_va_start(args, fmt);
	efi_status = VLogError(file, line, func, fmt, args);
	ms_va_end(args);

	return efi_status;
}

VOID
LogHexdump_(const char *file, int line, const char *func, const void *data, size_t sz)
{
	hexdumpat(file, line, func, data, sz, 0);
}

VOID
PrintErrors(VOID)
{
	UINTN i;

	if (!verbose)
		return;

	for (i = 0; i < nerrs; i++)
		console_print(L"%s", errs[i]);
}

VOID
ClearErrors(VOID)
{
	UINTN i;

	for (i = 0; i < nerrs; i++)
		FreePool(errs[i]);
	FreePool(errs);
	nerrs = 0;
	errs = NULL;
}

static size_t
format_error_log(UINT8 *dest, size_t dest_sz)
{
	size_t err_log_sz = 0;
	size_t pos = 0;

	for (UINTN i = 0; i < nerrs; i++)
		err_log_sz += StrSize(errs[i]);

	if (!dest || dest_sz < err_log_sz)
		return err_log_sz;

	ZeroMem(dest, err_log_sz);
	for (UINTN i = 0; i < nerrs; i++) {
		UINTN sz = StrSize(errs[i]);
		CopyMem(&dest[pos], errs[i], sz);
		pos += sz;
	}

	return err_log_sz;
}

static UINT8 *debug_log = NULL;
static size_t debug_log_sz = 0;
static size_t debug_log_alloc = 0;

UINTN EFIAPI
log_debug_print(const CHAR16 *fmt, ...)
{
	ms_va_list args;
	CHAR16 *buf;
	size_t buf_sz;
	UINTN ret = 0;

	ms_va_start(args, fmt);
	buf = VPoolPrint(fmt, args);
	if (!buf)
		return 0;
	ms_va_end(args);

	ret = StrLen(buf);
	buf_sz = StrSize(buf);
	if (debug_log_sz + buf_sz > debug_log_alloc) {
		size_t new_alloc_sz = debug_log_alloc;
		CHAR16 *new_debug_log;

		new_alloc_sz += buf_sz;
		new_alloc_sz = ALIGN_UP(new_alloc_sz, EFI_PAGE_SIZE);

		new_debug_log = ReallocatePool(debug_log, debug_log_alloc, new_alloc_sz);
		if (!new_debug_log)
			return 0;
		debug_log = (UINT8 *)new_debug_log;
		debug_log_alloc = new_alloc_sz;
	}

	CopyMem(&debug_log[debug_log_sz], buf, buf_sz);
	debug_log_sz += buf_sz;
	FreePool(buf);
	return ret;
}

static size_t
format_debug_log(UINT8 *dest, size_t dest_sz)
{
	if (!dest || dest_sz < debug_log_sz)
		return debug_log_sz;

	ZeroMem(dest, debug_log_sz);
	CopyMem(dest, debug_log, debug_log_sz);
	return debug_log_sz;
}

void
replace_config_table(EFI_CONFIGURATION_TABLE *CT, EFI_PHYSICAL_ADDRESS new_table, UINTN new_table_pages)
{
	EFI_GUID bogus_guid = { 0x29f2f0db, 0xd025, 0x4aa6, { 0x99, 0x58, 0xa0, 0x21, 0x8b, 0x1d, 0xec, 0x0e }};
	EFI_STATUS efi_status;

	if (CT) {
		CopyMem(&CT->VendorGuid, &bogus_guid, sizeof(bogus_guid));
		if (CT->VendorTable &&
		    CT->VendorTable == (void *)(uintptr_t)mok_config_table) {
			BS->FreePages(mok_config_table, mok_config_table_pages);
			CT->VendorTable = NULL;
		}
	}

	efi_status = BS->InstallConfigurationTable(&MOK_VARIABLE_STORE,
						   (void *)(uintptr_t)new_table);
	if (EFI_ERROR(efi_status)) {
		console_print(L"Could not re-install MoK configuration table: %r\n", efi_status);
	} else {
		mok_config_table = new_table;
		mok_config_table_pages = new_table_pages;
	}
}

void
save_logs(void)
{
	struct mok_variable_config_entry *cfg_table = NULL;
	struct mok_variable_config_entry *new_table = NULL;
	struct mok_variable_config_entry *entry = NULL;
	EFI_PHYSICAL_ADDRESS physaddr = 0;
	UINTN new_table_pages = 0;
	size_t new_table_sz;
	UINTN pos = 0;
	EFI_STATUS efi_status;
	size_t errlog_sz, dbglog_sz;

	errlog_sz = format_error_log(NULL, 0);
	dbglog_sz = format_debug_log(NULL, 0);

	if (errlog_sz == 0 && dbglog_sz == 0) {
		console_print(L"No console or debug log?!?!?\n");
		return;
	}

	for (UINTN i = 0; i < ST->NumberOfTableEntries; i++) {
		EFI_CONFIGURATION_TABLE *CT;
		CT = &ST->ConfigurationTable[i];

		if (CompareGuid(&MOK_VARIABLE_STORE, &CT->VendorGuid) == 0) {
			cfg_table = CT->VendorTable;
			break;
		}
		CT = NULL;
	}

	entry = cfg_table;
	while (entry && entry->name[0] != 0) {
		size_t entry_sz;
		entry = (struct mok_variable_config_entry *)((uintptr_t)cfg_table + pos);

		if (entry->name[0] != 0) {
			entry_sz = sizeof(*entry);
			entry_sz += entry->data_size;
			pos += entry_sz;
		}
	}

	new_table_sz = pos +
		(errlog_sz ? sizeof(*entry) + errlog_sz : 0) +
		(dbglog_sz ? sizeof(*entry) + dbglog_sz : 0) +
		sizeof(*entry);
	new_table = NULL;
	new_table_pages = ALIGN_UP(new_table_sz + 4*EFI_PAGE_SIZE, EFI_PAGE_SIZE) / EFI_PAGE_SIZE;
	efi_status = BS->AllocatePages(AllocateAnyPages, EfiRuntimeServicesData, new_table_pages, &physaddr);
	if (EFI_ERROR(efi_status)) {
		perror(L"Couldn't allocate %llu pages\n", new_table_pages);
		return;
	}
	new_table = (void *)(uintptr_t)physaddr;
	if (!new_table)
		return;
	ZeroMem(new_table, new_table_pages * EFI_PAGE_SIZE);
	CopyMem(new_table, cfg_table, pos);

	entry = (struct mok_variable_config_entry *)((uintptr_t)new_table + pos);
	if (errlog_sz) {
		strcpy(entry->name, "shim-err.txt");
		entry->data_size = errlog_sz;
		format_error_log(&entry->data[0], errlog_sz);

		pos += sizeof(*entry) + errlog_sz;
		entry = (struct mok_variable_config_entry *)((uintptr_t)new_table + pos);
	}
	if (dbglog_sz) {
		strcpy(entry->name, "shim-dbg.txt");
		entry->data_size = dbglog_sz;
		format_debug_log(&entry->data[0], dbglog_sz);

		pos += sizeof(*entry) + dbglog_sz;

		entry = (struct mok_variable_config_entry *)((uintptr_t)new_table + pos);
	}

	replace_config_table((EFI_CONFIGURATION_TABLE *)cfg_table, physaddr, new_table_pages);
}

// vim:fenc=utf-8:tw=75
