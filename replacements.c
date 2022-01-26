// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * shim - trivial UEFI first-stage bootloader
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
static typeof(systab->BootServices->Exit) system_exit;
#if !defined(DISABLE_EBS_PROTECTION)
static typeof(systab->BootServices->ExitBootServices) system_exit_boot_services;
#endif /* !defined(DISABLE_EBS_PROTECTION) */

static EFI_HANDLE last_loaded_image;

void
unhook_system_services(void)
{
	if (!systab)
		return;

	systab->BootServices->LoadImage = system_load_image;
	systab->BootServices->StartImage = system_start_image;
#if !defined(DISABLE_EBS_PROTECTION)
	systab->BootServices->ExitBootServices = system_exit_boot_services;
#endif /* !defined(DISABLE_EBS_PROTECTION) */
	BS = systab->BootServices;
}

static EFI_STATUS EFIAPI
load_image(BOOLEAN BootPolicy, EFI_HANDLE ParentImageHandle,
	EFI_DEVICE_PATH *DevicePath, VOID *SourceBuffer,
	UINTN SourceSize, EFI_HANDLE *ImageHandle)
{
	EFI_STATUS efi_status;

	unhook_system_services();
	efi_status = BS->LoadImage(BootPolicy, ParentImageHandle, DevicePath,
				   SourceBuffer, SourceSize, ImageHandle);
	hook_system_services(systab);
	if (EFI_ERROR(efi_status))
		last_loaded_image = NULL;
	else
		last_loaded_image = *ImageHandle;
	return efi_status;
}

static EFI_STATUS EFIAPI
replacement_start_image(EFI_HANDLE image_handle, UINTN *exit_data_size, CHAR16 **exit_data)
{
	EFI_STATUS efi_status;
	unhook_system_services();

	if (image_handle == last_loaded_image) {
		loader_is_participating = 1;
		uninstall_shim_protocols();
	}
	efi_status = BS->StartImage(image_handle, exit_data_size, exit_data);
	if (EFI_ERROR(efi_status)) {
		if (image_handle == last_loaded_image) {
			EFI_STATUS efi_status2 = install_shim_protocols();

			if (EFI_ERROR(efi_status2)) {
				console_print(L"Something has gone seriously wrong: %r\n",
					      efi_status2);
				console_print(L"shim cannot continue, sorry.\n");
				msleep(5000000);
				RT->ResetSystem(EfiResetShutdown,
						EFI_SECURITY_VIOLATION,
						0, NULL);
			}
		}
		hook_system_services(systab);
		loader_is_participating = 0;
	}
	return efi_status;
}

#if !defined(DISABLE_EBS_PROTECTION)
static EFI_STATUS EFIAPI
exit_boot_services(EFI_HANDLE image_key, UINTN map_key)
{
	if (loader_is_participating ||
	    verification_method == VERIFIED_BY_HASH) {
		unhook_system_services();
		EFI_STATUS efi_status;
		efi_status = BS->ExitBootServices(image_key, map_key);
		if (EFI_ERROR(efi_status))
			hook_system_services(systab);
		return efi_status;
	}

	console_print(L"Bootloader has not verified loaded image.\n");
	console_print(L"System is compromised.  halting.\n");
	msleep(5000000);
	RT->ResetSystem(EfiResetShutdown, EFI_SECURITY_VIOLATION, 0, NULL);
	return EFI_SECURITY_VIOLATION;
}
#endif /* !defined(DISABLE_EBS_PROTECTION) */

static EFI_STATUS EFIAPI
do_exit(EFI_HANDLE ImageHandle, EFI_STATUS ExitStatus,
	UINTN ExitDataSize, CHAR16 *ExitData)
{
	EFI_STATUS efi_status;

	shim_fini();

	restore_loaded_image();

	efi_status = BS->Exit(ImageHandle, ExitStatus,
			      ExitDataSize, ExitData);
	if (EFI_ERROR(efi_status)) {
		EFI_STATUS efi_status2 = shim_init();

		if (EFI_ERROR(efi_status2)) {
			console_print(L"Something has gone seriously wrong: %r\n",
				      efi_status2);
			console_print(L"shim cannot continue, sorry.\n");
			msleep(5000000);
			RT->ResetSystem(EfiResetShutdown,
					EFI_SECURITY_VIOLATION, 0, NULL);
		}
	}
	return efi_status;
}

void
hook_system_services(EFI_SYSTEM_TABLE *local_systab)
{
	systab = local_systab;
	BS = systab->BootServices;

	/* We need to hook various calls to make this work... */

	/* We need LoadImage() hooked so that fallback.c can load shim
	 * without having to fake LoadImage as well.  This allows it
	 * to call the system LoadImage(), and have us track the output
	 * and mark loader_is_participating in replacement_start_image.  This
	 * means anything added by fallback has to be verified by the system
	 * db, which we want to preserve anyway, since that's all launching
	 * through BDS gives us. */
	system_load_image = systab->BootServices->LoadImage;
	systab->BootServices->LoadImage = load_image;

	/* we need StartImage() so that we can allow chain booting to an
	 * image trusted by the firmware */
	system_start_image = systab->BootServices->StartImage;
	systab->BootServices->StartImage = replacement_start_image;

#if !defined(DISABLE_EBS_PROTECTION)
	/* we need to hook ExitBootServices() so a) we can enforce the policy
	 * and b) we can unwrap when we're done. */
	system_exit_boot_services = systab->BootServices->ExitBootServices;
	systab->BootServices->ExitBootServices = exit_boot_services;
#endif /* defined(DISABLE_EBS_PROTECTION) */
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

	/* we need to hook Exit() so that we can allow users to quit the
	 * bootloader and still e.g. start a new one or run an internal
	 * shell. */
	system_exit = systab->BootServices->Exit;
	systab->BootServices->Exit = do_exit;
}
