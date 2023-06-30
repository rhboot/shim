// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * loader-proto.c - shim's loader protocol
 *
 * Copyright Red Hat, Inc
 */

/*   Chemical agents lend themselves to covert use in sabotage against
 * which it is exceedingly difficult to visualize any really effective
 * defense... I will not dwell upon this use of CBW because, as one
 * pursues the possibilities of such covert uses, one discovers that the
 * scenarios resemble that in which the components of a nuclear weapon
 * are smuggled into New York City and assembled in the basement of the
 * Empire State Building.
 *   In other words, once the possibility is recognized to exist, about
 * all that one can do is worry about it.
 *   -- Dr. Ivan L Bennett, Jr., testifying before the Subcommittee on
 *      National Security Policy and Scientific Developments, November 20,
 *      1969.
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

static EFI_STATUS EFIAPI
shim_load_image(BOOLEAN BootPolicy, EFI_HANDLE ParentImageHandle,
                EFI_DEVICE_PATH *FilePath, VOID *SourceBuffer,
                UINTN SourceSize, EFI_HANDLE *ImageHandle)
{
	SHIM_LOADED_IMAGE *image;
	EFI_STATUS efi_status;

	(void)FilePath;

	if (BootPolicy || !SourceBuffer || !SourceSize)
		return EFI_UNSUPPORTED;

	image = AllocatePool(sizeof(*image));
	if (!image)
		return EFI_OUT_OF_RESOURCES;

	SetMem(image, sizeof(*image), 0);

	image->li.Revision = 0x1000;
	image->li.ParentHandle = ParentImageHandle;
	image->li.SystemTable = systab;

	efi_status = handle_image(SourceBuffer, SourceSize, &image->li,
	                          &image->entry_point, &image->alloc_address,
	                          &image->alloc_pages);
	if (EFI_ERROR(efi_status))
		goto free_image;

	*ImageHandle = NULL;
	efi_status = BS->InstallMultipleProtocolInterfaces(ImageHandle,
					&SHIM_LOADED_IMAGE_GUID, image,
					&EFI_LOADED_IMAGE_GUID, &image->li,
					NULL);
	if (EFI_ERROR(efi_status))
		goto free_alloc;

	return EFI_SUCCESS;

free_alloc:
	BS->FreePages(image->alloc_address, image->alloc_pages);
free_image:
	FreePool(image);
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
					NULL);

	BS->FreePages(image->alloc_address, image->alloc_pages);
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
