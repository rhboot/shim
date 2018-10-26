/*
 * Copyright 2012 <James.Bottomley@HansenPartnership.com>
 *
 * see COPYING file
 *
 * Install and remove a platform security2 override policy
 */

#include <efi.h>
#include <efilib.h>

#include "shim.h"

#include <Protocol/Security.h>
#include <Protocol/Security2.h>
#include <dpath.h>
#include <variables.h>
#include <simple_file.h>

#if defined(OVERRIDE_SECURITY_POLICY)
#include <security_policy.h>

static UINT8 *security_policy_esl = NULL;
static UINTN security_policy_esl_len;
static SecurityHook extra_check = NULL;

static EFI_SECURITY_FILE_AUTHENTICATION_STATE esfas = NULL;
static EFI_SECURITY2_FILE_AUTHENTICATION es2fa = NULL;

static EFI_STATUS EFIAPI
security2_policy_authentication (
	CONST EFI_SECURITY2_ARCH_PROTOCOL *This,
	CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
	VOID *FileBuffer,
	UINTN FileSize,
	BOOLEAN	BootPolicy
				 )
{
	EFI_STATUS efi_status, auth;

	/* Chain original security policy */

	efi_status = es2fa(This, DevicePath, FileBuffer, FileSize, BootPolicy);
	/* if OK, don't bother with MOK check */
	if (!EFI_ERROR(efi_status))
		return efi_status;

	if (extra_check)
		auth = extra_check(FileBuffer, FileSize);
	else
		return EFI_SECURITY_VIOLATION;

	if (auth == EFI_SECURITY_VIOLATION || auth == EFI_ACCESS_DENIED)
		/* return previous status, which is the correct one
		 * for the platform: may be either EFI_ACCESS_DENIED
		 * or EFI_SECURITY_VIOLATION */
		return efi_status;

	return auth;
}

static EFI_STATUS EFIAPI
security_policy_authentication (
	CONST EFI_SECURITY_ARCH_PROTOCOL *This,
	UINT32 AuthenticationStatus,
	CONST EFI_DEVICE_PATH_PROTOCOL *DevicePathConst
	)
{
	EFI_STATUS efi_status, fail_status;
	EFI_DEVICE_PATH *DevPath 
		= DuplicateDevicePath((EFI_DEVICE_PATH *)DevicePathConst),
		*OrigDevPath = DevPath;
	EFI_HANDLE h;
	EFI_FILE *f;
	VOID *FileBuffer;
	UINTN FileSize;
	CHAR16* DevPathStr;
	EFI_GUID SIMPLE_FS_PROTOCOL = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

	/* Chain original security policy */
	efi_status = esfas(This, AuthenticationStatus, DevicePathConst);
	/* if OK avoid checking MOK: It's a bit expensive to
	 * read the whole file in again (esfas already did this) */
	if (!EFI_ERROR(efi_status))
		goto out;

	/* capture failure status: may be either EFI_ACCESS_DENIED or
	 * EFI_SECURITY_VIOLATION */
	fail_status = efi_status;

	efi_status = gBS->LocateDevicePath(&SIMPLE_FS_PROTOCOL, &DevPath, &h);
	if (EFI_ERROR(efi_status))
		goto out;

	DevPathStr = DevicePathToStr(DevPath);

	efi_status = simple_file_open_by_handle(h, DevPathStr, &f,
						EFI_FILE_MODE_READ);
	FreePool(DevPathStr);
	if (EFI_ERROR(efi_status))
		goto out;

	efi_status = simple_file_read_all(f, &FileSize, &FileBuffer);
	f->Close(f);
	if (EFI_ERROR(efi_status))
		goto out;

	if (extra_check)
		efi_status = extra_check(FileBuffer, FileSize);
	else
		efi_status = EFI_SECURITY_VIOLATION;
	FreePool(FileBuffer);

	if (efi_status == EFI_ACCESS_DENIED ||
	    efi_status == EFI_SECURITY_VIOLATION)
		/* return what the platform originally said */
		efi_status = fail_status;
 out:
	FreePool(OrigDevPath);
	return efi_status;
}

EFI_STATUS
security_policy_install(SecurityHook hook)
{
	EFI_SECURITY_ARCH_PROTOCOL *security_protocol;
	EFI_SECURITY2_ARCH_PROTOCOL *security2_protocol = NULL;
	EFI_STATUS efi_status;

	if (esfas)
		/* Already Installed */
		return EFI_ALREADY_STARTED;

	/* Don't bother with status here.  The call is allowed
	 * to fail, since SECURITY2 was introduced in PI 1.2.1
	 * If it fails, use security2_protocol == NULL as indicator */
	LibLocateProtocol(&gEfiSecurity2ArchProtocolGuid,
			  (VOID **) &security2_protocol);

	efi_status = LibLocateProtocol(&gEfiSecurityArchProtocolGuid,
				       (VOID **) &security_protocol);
	if (EFI_ERROR(efi_status))
		/* This one is mandatory, so there's a serious problem */
		return efi_status;

	if (security2_protocol) {
		es2fa = security2_protocol->FileAuthentication;
		security2_protocol->FileAuthentication = security2_policy_authentication;
	}

	esfas = security_protocol->FileAuthenticationState;
	security_protocol->FileAuthenticationState = security_policy_authentication;

	if (hook)
		extra_check = hook;

	return EFI_SUCCESS;
}

EFI_STATUS
security_policy_uninstall(void)
{
	EFI_STATUS efi_status;

	if (esfas) {
		EFI_SECURITY_ARCH_PROTOCOL *security_protocol;

		efi_status = LibLocateProtocol(&gEfiSecurityArchProtocolGuid,
					       (VOID **) &security_protocol);
		if (EFI_ERROR(efi_status))
			return efi_status;

		security_protocol->FileAuthenticationState = esfas;
		esfas = NULL;
	} else {
		/* nothing installed */
		return EFI_NOT_STARTED;
	}

	if (es2fa) {
		EFI_SECURITY2_ARCH_PROTOCOL *security2_protocol;

		efi_status = LibLocateProtocol(&gEfiSecurity2ArchProtocolGuid,
					       (VOID **) &security2_protocol);
		if (EFI_ERROR(efi_status))
			return efi_status;

		security2_protocol->FileAuthentication = es2fa;
		es2fa = NULL;
	}

	if (extra_check)
		extra_check = NULL;

	return EFI_SUCCESS;
}

void
security_protocol_set_hashes(UINT8 *esl, UINTN len)
{
	security_policy_esl = esl;
	security_policy_esl_len = len;
}
#endif /* OVERRIDE_SECURITY_POLICY */
