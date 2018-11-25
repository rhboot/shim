/*
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

EFI_STATUS proxyload(BOOLEAN boot_policy, EFI_HANDLE parent_image,
		     EFI_DEVICE_PATH *path, VOID *buffer, UINTN size,
		     EFI_HANDLE *handle) {
	PROXY_LOADER_PROTOCOL *proxy;
	EFI_LOADED_IMAGE *li;
	EFI_PHYSICAL_ADDRESS alloc_address;
	UINTN alloc_pages;
	EFI_STATUS efi_status;

	/* Sanity check */
	if ((parent_image == NULL) || (path == NULL) || (buffer == NULL) ||
	    (handle == NULL)) {
		efi_status = EFI_INVALID_PARAMETER;
		goto err_invalid;
	}

	/* Allocate proxy loader protocol */
	proxy = AllocatePool(sizeof(*proxy));
	if (!proxy) {
		efi_status = EFI_OUT_OF_RESOURCES;
		goto err_alloc;
	}
	memset(proxy, 0, sizeof(*proxy));

	/* Install proxy loader image */
	efi_status = gBS->LoadImage(boot_policy, parent_image, NULL,
				    proxy_loader, proxy_loader_size, handle);
	if (EFI_ERROR(efi_status))
		goto err_load_image;

	/* Locate loaded image protocol */
	efi_status = gBS->HandleProtocol(*handle, &EFI_LOADED_IMAGE_GUID,
					 (void **)&li);
	if (EFI_ERROR(efi_status))
		goto err_loaded_image;

	/* Verify and relocate image */
	efi_status = handle_image(buffer, size, li, &proxy->Entry,
				  &alloc_address, &alloc_pages);
	if (EFI_ERROR(efi_status))
		goto err_handle_image;

	/* Install proxy loader protocol onto image handle */
	efi_status = gBS->InstallMultipleProtocolInterfaces(handle,
							    &PROXY_LOADER_GUID,
							    proxy, NULL);
	if (EFI_ERROR(efi_status))
		goto err_install;

	return EFI_SUCCESS;

 err_install:
	gBS->FreePages(alloc_address, alloc_pages);
 err_handle_image:
 err_loaded_image:
	gBS->UnloadImage(*handle);
 err_load_image:
	FreePool(proxy);
 err_alloc:
 err_invalid:
	return efi_status;
}
