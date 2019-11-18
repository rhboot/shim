/*
 * debug.c
 * Copyright 2019 Peter Jones <pjones@redhat.com>
 *
 */

#include "shim.h"

CONSTRUCTOR OPTIMIZE("0")
static void
debug_hook(void)
{
	UINT8 *data = NULL;
	UINTN dataSize = 0;
	EFI_STATUS efi_status;
	volatile register UINTN x = 0;
	extern char _text, _data;

	if (x)
		return;

	efi_status = get_variable(L"SHIM_DEBUG", &data, &dataSize,
				  SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
		return;
	}

	FreePool(data);

	console_print(L"add-symbol-file "DEBUGDIR
		      L"shim" EFI_ARCH L".efi.debug 0x%08x -s .data 0x%08x\n",
		      &_text, &_data);

	console_print(L"Pausing for debugger attachment.\n");
	console_print(L"To disable this, remove the EFI variable SHIM_DEBUG-%g .\n",
		      &SHIM_LOCK_GUID);
	x = 1;
	while (x++) {
		/* Make this so it can't /totally/ DoS us. */
#if defined(__x86_64__) || defined(__i386__) || defined(__i686__)
		if (x > 4294967294ULL)
			break;
#elif defined(__aarch64__)
		if (x > 1000)
			break;
#else
		if (x > 12000)
			break;
#endif
		pause();
	}
	x = 1;
}

// vim:fenc=utf-8:tw=75:noet
