/*
 * Copyright 2012 <James.Bottomley@HansenPartnership.com>
 *
 * see COPYING file
 *
 * misc shell helper functions
 */
#include <efi.h>
#include <efilib.h>

#include <shell.h>

EFI_STATUS
argsplit(EFI_HANDLE image, int *argc, CHAR16*** ARGV)
{
	unsigned int i, count = 1;
	EFI_STATUS status;
	EFI_LOADED_IMAGE *info;
	CHAR16 *start;

	*argc = 0;

	status = uefi_call_wrapper(BS->HandleProtocol, 3, image, &LoadedImageProtocol, (VOID **) &info);
	if (EFI_ERROR(status)) {
		Print(L"Failed to get arguments\n");
		return status;
	}

	for (i = 0; i < info->LoadOptionsSize; i += 2) {
		CHAR16 *c = (CHAR16 *)(info->LoadOptions + i);
		if (*c == L' ' && *(c+1) != '\0') {
			(*argc)++;
		}
	}

	(*argc)++;		/* we counted spaces, so add one for initial */

	*ARGV = AllocatePool(*argc * sizeof(**ARGV));
	if (!*ARGV) {
		return EFI_OUT_OF_RESOURCES;
	}
	(*ARGV)[0] = (CHAR16 *)info->LoadOptions;
	for (i = 0; i < info->LoadOptionsSize; i += 2) {
		CHAR16 *c = (CHAR16 *)(info->LoadOptions + i);
		if (*c == L' ') {
			*c = L'\0';
			if (*(c + 1) == '\0')
				/* strip trailing space */
				break;
			start = c + 1;
			(*ARGV)[count++] = start;
		}
	}

	return EFI_SUCCESS;
}

