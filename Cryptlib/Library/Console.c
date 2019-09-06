/*
 * Console.c
 * Copyright 2019 Peter Jones <pjones@redhat.com>
 *
 */

#include <efi.h>
#include <efilib.h>

#include <Library/BaseCryptLib.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

extern UINT32 verbose;
extern UINTN console_print(const CHAR16 *fmt, ...);

static int
print_errors_cb(const char *str, size_t len, void *u)
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



// vim:fenc=utf-8:tw=75:noet
