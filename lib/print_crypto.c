// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * Copyright 2019 SUSE LLC <glin@suse.com>
 */
#include "shim.h"

#include <Library/BaseCryptLib.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <console.h>

static int
print_errors_cb(const char *str, size_t len, void *u UNUSED)
{
	console_print(L"%a", str);

	return len;
}

EFI_STATUS
print_crypto_errors(EFI_STATUS efi_status,
		    char *file, const char *func, int line)
{
	if (!(verbose && EFI_ERROR(efi_status)))
		return efi_status;

	console_print(L"SSL Error: %a:%d %a(): %r\n", file, line, func,
		      efi_status);
	ERR_print_errors_cb(print_errors_cb, NULL);

	return efi_status;
}
