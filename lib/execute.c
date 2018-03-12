/*
 * Copyright 2012 <James.Bottomley@HansenPartnership.com>
 *
 * see COPYING file
 *
 * --
 *
 * generate_path is a cut and paste from
 *  
 *   git://github.com/mjg59/shim.git
 *
 * Code Copyright 2012 Red Hat, Inc <mjg@redhat.com>
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
 *
 */

#include <efi.h>
#include <efilib.h>

#include "shim.h"

EFI_STATUS
generate_path(CHAR16* name, EFI_LOADED_IMAGE *li, EFI_DEVICE_PATH **path, CHAR16 **PathName)
{
	unsigned int pathlen;
	EFI_STATUS efi_status = EFI_SUCCESS;
	CHAR16 *devpathstr = DevicePathToStr(li->FilePath),
		*found = NULL;
	unsigned int i;

	for (i = 0; i < StrLen(devpathstr); i++) {
		if (devpathstr[i] == '/')
			devpathstr[i] = '\\';
		if (devpathstr[i] == '\\')
			found = &devpathstr[i];
	}
	if (!found) {
		pathlen = 0;
	} else {
		while (*(found - 1) == '\\')
			--found;
		*found = '\0';
		pathlen = StrLen(devpathstr);
	}

	if (name[0] != '\\')
		pathlen++;

	*PathName = AllocatePool((pathlen + 1 + StrLen(name))*sizeof(CHAR16));

	if (!*PathName) {
		console_print(L"Failed to allocate path buffer\n");
		efi_status = EFI_OUT_OF_RESOURCES;
		goto error;
	}

	StrCpy(*PathName, devpathstr);

	if (name[0] != '\\')
		StrCat(*PathName, L"\\");
	StrCat(*PathName, name);
	
	*path = FileDevicePath(li->DeviceHandle, *PathName);

error:
	FreePool(devpathstr);

	return efi_status;
}

EFI_STATUS
execute(EFI_HANDLE image, CHAR16 *name)
{
	EFI_STATUS efi_status;
	EFI_HANDLE h;
	EFI_LOADED_IMAGE *li;
	EFI_DEVICE_PATH *devpath;
	CHAR16 *PathName;

	efi_status = gBS->HandleProtocol(image, &IMAGE_PROTOCOL,
					 (void **) &li);
	if (EFI_ERROR(efi_status))
		return efi_status;

	efi_status = generate_path(name, li, &devpath, &PathName);
	if (EFI_ERROR(efi_status))
		return efi_status;

	efi_status = gBS->LoadImage(FALSE, image, devpath, NULL, 0, &h);
	if (EFI_ERROR(efi_status))
		goto out;

	efi_status = gBS->StartImage(h, NULL, NULL);
	gBS->UnloadImage(h);

 out:
	FreePool(PathName);
	FreePool(devpath);
	return efi_status;
}
