// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * loader-proto.c - shim's loader protocol
 *
 * Copyright Red Hat, Inc
 * Copyright Canonical, Ltd
 */

#include "shim.h"

static EFI_SYSTEM_TABLE *systab;

EFI_SYSTEM_TABLE *
get_active_systab(void)
{
	if (systab)
		return systab;
	return ST;
}

static typeof(systab->BootServices->LoadImage) system_load_image;
static typeof(systab->BootServices->StartImage) system_start_image;
static typeof(systab->BootServices->UnloadImage) system_unload_image;
static typeof(systab->BootServices->Exit) system_exit;

void
unhook_system_services(void)
{
	if (!systab)
		return;

	systab->BootServices->LoadImage = system_load_image;
	systab->BootServices->StartImage = system_start_image;
	systab->BootServices->Exit = system_exit;
	systab->BootServices->UnloadImage = system_unload_image;
	BS = systab->BootServices;
}

typedef struct {
	EFI_HANDLE hnd;
	EFI_DEVICE_PATH *dp;
	void *buffer;
	size_t size;
} buffer_properties_t;

static EFI_STATUS
try_load_from_sfs(EFI_DEVICE_PATH *dp, buffer_properties_t *bprop)
{
	EFI_STATUS status = EFI_SUCCESS;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfs = NULL;
	EFI_FILE_HANDLE root = NULL;
	EFI_FILE_HANDLE file = NULL;
	UINT64 tmpsz = 0;

	bprop->buffer = NULL;

	/* look for a handle with SFS support from the input DP */
	bprop->dp = dp;
	status = BS->LocateDevicePath(&EFI_SIMPLE_FILE_SYSTEM_GUID, &bprop->dp, &bprop->hnd);
	if (EFI_ERROR(status)) {
		goto out;
	}

	/* make sure the remaining DP portion is really a file path */
	if (DevicePathType(bprop->dp) != MEDIA_DEVICE_PATH ||
	    DevicePathSubType(bprop->dp) != MEDIA_FILEPATH_DP) {
		status = EFI_LOAD_ERROR;
		goto out;
	}

	/* find protocol, open the root directory, then open file */
	status = BS->HandleProtocol(bprop->hnd, &EFI_SIMPLE_FILE_SYSTEM_GUID, (void **)&sfs);
	if (EFI_ERROR(status))
		goto out;
	status = sfs->OpenVolume(sfs, &root);
	if (EFI_ERROR(status))
		goto out;
	status = root->Open(root, &file, ((FILEPATH_DEVICE_PATH *) bprop->dp)->PathName, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status))
		goto out;

	/* get file size */
	status = file->SetPosition(file, -1ULL);
	if (EFI_ERROR(status))
		goto out;
	status = file->GetPosition(file, &tmpsz);
	if (EFI_ERROR(status))
		goto out;
	bprop->size = (size_t)tmpsz;
	status = file->SetPosition(file, 0);
	if (EFI_ERROR(status))
		goto out;

	/* allocate buffer */
	bprop->buffer = AllocatePool(bprop->size);
	if (bprop->buffer == NULL) {
		status = EFI_OUT_OF_RESOURCES;
		goto out;
	}

	/* read file */
	status = file->Read(file, &bprop->size, bprop->buffer);

out:
	if (EFI_ERROR(status) && bprop->buffer)
		FreePool(bprop->buffer);
	if (file)
		file->Close(file);
	if (root)
		root->Close(root);
	return status;
}


static EFI_STATUS
try_load_from_lf2(EFI_DEVICE_PATH *dp, buffer_properties_t *bprop)
{
	EFI_STATUS status = EFI_SUCCESS;
	EFI_LOAD_FILE2_PROTOCOL *lf2 = NULL;

	bprop->buffer = NULL;

	/* look for a handle with LF2 support from the input DP */
	bprop->dp = dp;
	status = BS->LocateDevicePath(&gEfiLoadFile2ProtocolGuid, &bprop->dp, &bprop->hnd);
	if (EFI_ERROR(status))
		goto out;

	/* find protocol */
	status = BS->HandleProtocol(bprop->hnd, &gEfiLoadFile2ProtocolGuid, (void **) &lf2);
	if (EFI_ERROR(status))
		goto out;

	/* get file size */
	bprop->size = 0; /* this shouldn't be read when Buffer=NULL but better be safe */
	status = lf2->LoadFile(lf2, bprop->dp, /*BootPolicy=*/false, &bprop->size, NULL);
	/*
	 * NOTE: the spec is somewhat ambiguous what is the correct return
	 * status code when asking for the buffer size with Buffer=NULL. I am
	 * assuming EFI_SUCCESS and EFI_BUFFER_TOO_SMALL are the only
	 * reasonable interpretations.
	 */
	if (EFI_ERROR(status) && status != EFI_BUFFER_TOO_SMALL) {
		status = EFI_LOAD_ERROR;
		goto out;
	}

	/* allocate buffer */
	bprop->buffer = AllocatePool(bprop->size);
	if (!bprop->buffer) {
		status = EFI_OUT_OF_RESOURCES;
		goto out;
	}

	/* read file */
	status = lf2->LoadFile(lf2, bprop->dp, /*BootPolicy=*/false, &bprop->size, bprop->buffer);
	if (EFI_ERROR(status))
		goto out;

out:
	if (EFI_ERROR(status) && bprop->buffer)
		FreePool(bprop->buffer);
	return status;
}

static EFI_STATUS EFIAPI
shim_load_image(BOOLEAN BootPolicy, EFI_HANDLE ParentImageHandle,
                EFI_DEVICE_PATH *DevicePath, VOID *SourceBuffer,
                UINTN SourceSize, EFI_HANDLE *ImageHandle)
{
	SHIM_LOADED_IMAGE *image;
	EFI_STATUS efi_status;
	buffer_properties_t bprop;

	if (BootPolicy)
		return EFI_UNSUPPORTED;

	if (!SourceBuffer || !SourceSize) {
		if (!DevicePath) /* Both SourceBuffer and DevicePath are NULL */
			return EFI_NOT_FOUND;

		if (try_load_from_sfs(DevicePath, &bprop) == EFI_SUCCESS)
			;
		else if (try_load_from_lf2(DevicePath, &bprop) == EFI_SUCCESS)
			;
		else
			/* no buffer given and we cannot load from this device */
			return EFI_LOAD_ERROR;

		SourceBuffer = bprop.buffer;
		SourceSize = bprop.size;
	} else {
		bprop.buffer = NULL;
		/*
		 * Even if we are using a buffer, try populating the
		 * device_handle and file_path fields the best we can
		 */

		bprop.dp = DevicePath;

		if (bprop.dp) {
			efi_status = BS->LocateDevicePath(&gEfiDevicePathProtocolGuid,
			                                  &bprop.dp,
			                                  &bprop.hnd);
			if (efi_status != EFI_SUCCESS) {
				/* can't seem to pull apart this DP */
				bprop.dp = DevicePath;
				bprop.hnd = NULL;
			}
		}
	}

	image = AllocatePool(sizeof(*image));
	if (!image) {
		efi_status = EFI_OUT_OF_RESOURCES;
		goto free_buffer;
	}

	SetMem(image, sizeof(*image), 0);

	image->li.Revision = 0x1000;
	image->li.ParentHandle = ParentImageHandle;
	image->li.SystemTable = systab;
	image->li.DeviceHandle = bprop.hnd;
	if (bprop.dp) {
		image->li.FilePath = DuplicateDevicePath(bprop.dp);
		if (!image->li.FilePath) {
			efi_status = EFI_OUT_OF_RESOURCES;
			goto free_image;
		}
	}
	if (DevicePath) {
		image->loaded_image_device_path = DuplicateDevicePath(DevicePath);
		if (!image->loaded_image_device_path) {
			efi_status = EFI_OUT_OF_RESOURCES;
			goto free_image;
		}
	}

	in_protocol = 1;
	efi_status = handle_image(SourceBuffer, SourceSize, &image->li,
	                          &image->entry_point, &image->alloc_address,
	                          &image->alloc_pages);
	in_protocol = 0;
	if (EFI_ERROR(efi_status))
		goto free_image;

	*ImageHandle = NULL;
	efi_status = BS->InstallMultipleProtocolInterfaces(ImageHandle,
					&SHIM_LOADED_IMAGE_GUID, image,
					&EFI_LOADED_IMAGE_GUID, &image->li,
					&gEfiLoadedImageDevicePathProtocolGuid,
					image->loaded_image_device_path,
					NULL);
	if (EFI_ERROR(efi_status))
		goto free_alloc;

	if (bprop.buffer)
		FreePool(bprop.buffer);

	return EFI_SUCCESS;

free_alloc:
	BS->FreePages(image->alloc_address, image->alloc_pages);
free_image:
	if (image->loaded_image_device_path)
		FreePool(image->loaded_image_device_path);
	if (image->li.FilePath)
		FreePool(image->li.FilePath);
	FreePool(image);
free_buffer:
	if (bprop.buffer)
		FreePool(bprop.buffer);
	return efi_status;
}

static EFI_STATUS EFIAPI
shim_start_image(IN EFI_HANDLE ImageHandle, OUT UINTN *ExitDataSize,
                 OUT CHAR16 **ExitData OPTIONAL)
{
	SHIM_LOADED_IMAGE *image;
	EFI_STATUS efi_status;

	efi_status = BS->HandleProtocol(ImageHandle, &SHIM_LOADED_IMAGE_GUID,
					(void **)&image);

	/*
	 * This image didn't come from shim_load_image(), so it must have come
	 * from something before shim was involved.
	 */
	if (efi_status == EFI_UNSUPPORTED)
		return system_start_image(ImageHandle, ExitDataSize, ExitData);

	if (EFI_ERROR(efi_status) || image->started)
		return EFI_INVALID_PARAMETER;

	if (!setjmp(image->longjmp_buf)) {
		image->started = true;
		efi_status =
			image->entry_point(ImageHandle, image->li.SystemTable);
	} else {
		if (ExitData) {
			*ExitDataSize = image->exit_data_size;
			*ExitData = (CHAR16 *)image->exit_data;
		}
		efi_status = image->exit_status;
	}

	//
	// We only support EFI applications, so we can unload and free the
	// image unconditionally.
	//
	BS->UninstallMultipleProtocolInterfaces(ImageHandle,
	                                &EFI_LOADED_IMAGE_GUID, image,
	                                &SHIM_LOADED_IMAGE_GUID, &image->li,
	                                &gEfiLoadedImageDevicePathProtocolGuid,
					image->loaded_image_device_path,
					NULL);

	BS->FreePages(image->alloc_address, image->alloc_pages);
	if (image->li.FilePath)
		BS->FreePool(image->li.FilePath);
	if (image->loaded_image_device_path)
		BS->FreePool(image->loaded_image_device_path);
	FreePool(image);

	return efi_status;
}

static EFI_STATUS EFIAPI
shim_unload_image(EFI_HANDLE ImageHandle)
{
	SHIM_LOADED_IMAGE *image;
	EFI_STATUS efi_status;

	efi_status = BS->HandleProtocol(ImageHandle, &SHIM_LOADED_IMAGE_GUID,
					(void **)&image);

	if (efi_status == EFI_UNSUPPORTED)
		return system_unload_image(ImageHandle);

	BS->FreePages(image->alloc_address, image->alloc_pages);
	FreePool(image);

	return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI
shim_exit(EFI_HANDLE ImageHandle,
	  EFI_STATUS ExitStatus,
	  UINTN ExitDataSize,
	  CHAR16 *ExitData)
{
	EFI_STATUS efi_status;
	SHIM_LOADED_IMAGE *image;

	efi_status = BS->HandleProtocol(ImageHandle, &SHIM_LOADED_IMAGE_GUID,
					(void **)&image);

	/*
	 * If this happens, something above us on the stack of running
	 * applications called Exit(), and we're getting aborted along with
	 * it.
	 */
	if (efi_status == EFI_UNSUPPORTED) {
		shim_fini();
		return system_exit(ImageHandle, ExitStatus, ExitDataSize,
				   ExitData);
	}

	if (EFI_ERROR(efi_status))
		return efi_status;

	image->exit_status = ExitStatus;
	image->exit_data_size = ExitDataSize;
	image->exit_data = ExitData;

	longjmp(image->longjmp_buf, 1);
}

void
init_image_loader(void)
{
	shim_image_loader_interface.LoadImage = shim_load_image;
	shim_image_loader_interface.StartImage = shim_start_image;
	shim_image_loader_interface.Exit = shim_exit;
	shim_image_loader_interface.UnloadImage = shim_unload_image;
}

void
hook_system_services(EFI_SYSTEM_TABLE *local_systab)
{
	systab = local_systab;
	BS = systab->BootServices;

	/* We need to hook various calls to make this work... */

	/*
	 * We need LoadImage() hooked so that we can guarantee everything is
	 * verified.
	 */
	system_load_image = systab->BootServices->LoadImage;
	systab->BootServices->LoadImage = shim_load_image;

	/*
	 * We need StartImage() hooked because the system's StartImage()
	 * doesn't know about our structure layout.
	 */
	system_start_image = systab->BootServices->StartImage;
	systab->BootServices->StartImage = shim_start_image;

	/*
	 * We need Exit() hooked so that we make sure to use the right jmp_buf
	 * when an application calls Exit(), but that happens in a separate
	 * function.
	 */

	/*
	 * We need UnloadImage() to match our LoadImage()
	 */
	system_unload_image = systab->BootServices->UnloadImage;
	systab->BootServices->UnloadImage = shim_unload_image;
}

void
unhook_exit(void)
{
	systab->BootServices->Exit = system_exit;
	BS = systab->BootServices;
}

void
hook_exit(EFI_SYSTEM_TABLE *local_systab)
{
	systab = local_systab;
	BS = local_systab->BootServices;

	/*
	 * We need to hook Exit() so that we can allow users to quit the
	 * bootloader and still e.g. start a new one or run an internal
	 * shell.
	 */
	system_exit = systab->BootServices->Exit;
	systab->BootServices->Exit = shim_exit;
}
