/*
 * loader - UEFI shim transparent LoadImage()/StartImage() support
 *
 * Copyright (C) 2018 Michael Brown <mbrown@fensystems.co.uk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "shim.h"
#include <string.h>
#include <efisetjmp.h>

struct image {
	LOADER_PROTOCOL loader; /* Must be first */
	EFI_LOADED_IMAGE_PROTOCOL li;

	EFI_HANDLE handle;
	EFI_IMAGE_ENTRY_POINT entry;
	EFI_PHYSICAL_ADDRESS memory;
	UINTN pages;

	BOOLEAN started;
	BOOLEAN exited;
	EFI_STATUS exit_status;
	UINTN exit_data_size;
	CHAR16 *exit_data;

	jmp_buf jmp_buf;
};

static EFI_DEVICE_PATH * devpath_end(EFI_DEVICE_PATH *path) {

	while (path->Type != END_DEVICE_PATH_TYPE) {
		path = (((void *) path) +
			((path->Length[1] << 8) | path->Length[0]));
	}
	return path;
}

LOADER_PROTOCOL *loader_protocol(EFI_HANDLE image) {
	void *interface;
	EFI_STATUS efi_status;

	efi_status = BS->OpenProtocol(image, &LOADER_GUID, &interface, image,
				      NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efi_status))
		return NULL;
	return interface;
}

static EFI_STATUS EFIAPI loader_start_image(LOADER_PROTOCOL *This,
					    UINTN *ExitDataSize,
					    CHAR16 **ExitData) {
	struct image *image = (struct image *) This;
	EFI_STATUS efi_status;

	if (image->started)
		return EFI_INVALID_PARAMETER;

	image->started = TRUE;
	image->exit_data_size = 0;
	image->exit_data = NULL;
	if (setjmp(&image->jmp_buf) == 0 ) {
		efi_status = image->entry(image->handle, image->li.SystemTable);
	} else {
		efi_status = image->exit_status;
	}
	image->exited = TRUE;
	if (ExitDataSize)
		*ExitDataSize = image->exit_data_size;
	if (ExitData)
		*ExitData = image->exit_data;

	return efi_status;
}

static EFI_STATUS EFIAPI loader_unload_image(LOADER_PROTOCOL *This) {
	struct image *image = (struct image *) This;
	EFI_STATUS efi_status;

	if (image->started) {
		if (!image->li.Unload)
			return EFI_UNSUPPORTED;
		efi_status = image->li.Unload(image->handle);
		if (EFI_ERROR(efi_status))
			return efi_status;
	}

	BS->UninstallMultipleProtocolInterfaces(
			  image->handle, &LOADER_GUID, &image->loader,
			  &EFI_LOADED_IMAGE_PATH_GUID, &image->li.FilePath,
			  &EFI_LOADED_IMAGE_GUID, &image->li,
			  NULL);
	BS->FreePages(image->memory, image->pages);
	FreePool(image);
	return 0;
}

static EFI_STATUS EFIAPI loader_exit(LOADER_PROTOCOL *This,
				     EFI_STATUS ExitStatus, UINTN ExitDataSize,
				     CHAR16 *ExitData) {
	struct image *image = (struct image *) This;

	if (!image->started)
		return loader_unload_image(This);
	if (image->exited)
		return EFI_INVALID_PARAMETER;
	image->exit_status = ExitStatus;
	image->exit_data_size = ExitDataSize;
	image->exit_data = ExitData;
	longjmp(&image->jmp_buf, 1);
}

EFI_STATUS loader_install(EFI_SYSTEM_TABLE *systab, EFI_HANDLE parent_image,
			  EFI_DEVICE_PATH *path, VOID *buffer, UINTN size,
			  EFI_HANDLE *handle) {
	struct image *image;
	EFI_DEVICE_PATH *end;
	UINTN path_len;
	EFI_STATUS efi_status;

	if ((parent_image == NULL) || (path == NULL) || (buffer == NULL) ||
	    (handle == NULL)) {
		efi_status = EFI_INVALID_PARAMETER;
		goto err_invalid;
	}

	end = devpath_end(path);
	path_len = (((void *)end - (void *)path) + sizeof(*end));
	image = AllocatePool(sizeof(*image) + path_len);
	if (!image) {
		efi_status = EFI_OUT_OF_RESOURCES;
		goto err_alloc;
	}
	memset(image, 0, sizeof(*image));
	image->loader.StartImage = loader_start_image;
	image->loader.UnloadImage = loader_unload_image;
	image->loader.Exit = loader_exit;
	image->li.Revision = EFI_LOADED_IMAGE_PROTOCOL_REVISION;
	image->li.ParentHandle = parent_image;
	image->li.SystemTable = systab;
	image->li.FilePath = (void *)image + sizeof(*image);
	memcpy(image->li.FilePath, path, path_len);

	efi_status = handle_image(buffer, size, &image->li, &image->entry,
				  &image->memory, &image->pages);
	if (EFI_ERROR(efi_status))
		goto err_handle_image;

	efi_status = BS->InstallMultipleProtocolInterfaces(
			       &image->handle, &LOADER_GUID, &image->loader,
			       &EFI_LOADED_IMAGE_PATH_GUID, image->li.FilePath,
			       &EFI_LOADED_IMAGE_GUID, &image->li,
			       NULL);
	if (EFI_ERROR(efi_status))
		goto err_install;

	*handle = image->handle;
	return 0;

 err_install:
	BS->FreePages(image->memory, image->pages);
 err_handle_image:
	FreePool(image);
 err_alloc:
 err_invalid:
	return efi_status;
}
