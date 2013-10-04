/*
 * shim - trivial UEFI first-stage bootloader
 *
 * Copyright 2012 Red Hat, Inc <mjg@redhat.com>
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
 * Significant portions of this code are derived from Tianocore
 * (http://tianocore.sf.net) and are Copyright 2009-2012 Intel
 * Corporation.
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

#include <efi.h>
#include <efiapi.h>
#include <efilib.h>
#include "shim.h"
#include "replacements.h"

/* oh for fuck's sakes.*/
#ifndef EFI_SECURITY_VIOLATION
#define EFI_SECURITY_VIOLATION 26
#endif

static EFI_SYSTEM_TABLE *systab;

static typeof(systab->BootServices->StartImage) system_start_image;
static typeof(systab->BootServices->Exit) system_exit;
static typeof(systab->BootServices->ExitBootServices) system_exit_boot_services;

extern UINT8 insecure_mode;

void
unhook_system_services(void)
{
	if (insecure_mode)
		return;
	systab->BootServices->Exit = system_exit;
	systab->BootServices->StartImage = system_start_image;
	systab->BootServices->ExitBootServices = system_exit_boot_services;
}

static EFI_STATUS EFIAPI
start_image(EFI_HANDLE image_handle, UINTN *exit_data_size, CHAR16 **exit_data)
{
	EFI_STATUS status;
	unhook_system_services();
	status = systab->BootServices->StartImage(image_handle, exit_data_size, exit_data);
	if (EFI_ERROR(status))
		hook_system_services(systab);
	return status;
}

static EFI_STATUS EFIAPI
exit_boot_services(EFI_HANDLE image_key, UINTN map_key)
{
	if (loader_is_participating || verification_method == VERIFIED_BY_HASH) {
		unhook_system_services();
		EFI_STATUS status;
		status = systab->BootServices->ExitBootServices(image_key, map_key);
		if (status != EFI_SUCCESS)
			hook_system_services(systab);
		return status;
	}

	Print(L"Bootloader has not verified loaded image.\n");
	Print(L"System is compromised.  halting.\n");
	systab->BootServices->Stall(5000000);
	systab->RuntimeServices->ResetSystem(EfiResetShutdown, EFI_SECURITY_VIOLATION, 0, NULL);
	return EFI_SECURITY_VIOLATION;
}

static EFI_STATUS EFIAPI
exit(EFI_HANDLE ImageHandle, EFI_STATUS ExitStatus,
     UINTN ExitDataSize, CHAR16 *ExitData)
{
	EFI_STATUS status;
	unhook_system_services();

	status = systab->BootServices->Exit(ImageHandle, ExitStatus, ExitDataSize, ExitData);
	if (EFI_ERROR(status))
		hook_system_services(systab);
	return status;
}


void
hook_system_services(EFI_SYSTEM_TABLE *local_systab)
{
	if (insecure_mode)
		return;
	systab = local_systab;

	/* We need to hook various calls to make this work... */

	/* we need StartImage() so that we can allow chain booting to an
	 * image trusted by the firmware */
	system_start_image = systab->BootServices->StartImage;
	systab->BootServices->StartImage = start_image;

	/* we need to hook ExitBootServices() so a) we can enforce the policy
	 * and b) we can unwrap when we're done. */
	system_exit_boot_services = systab->BootServices->ExitBootServices;
	systab->BootServices->ExitBootServices = exit_boot_services;

	/* we need to hook Exit() so that we can allow users to quit the
	 * bootloader and still e.g. start a new one or run an internal
	 * shell. */
	system_exit = systab->BootServices->Exit;
	systab->BootServices->Exit = exit;
}
