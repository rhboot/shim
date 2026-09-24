// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * stack-protector.c - support for gcc -fstack-protector
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

/*
 * Before the first call to stack_protector_init(), it's important for the
 * canary it to have /something nonzero/ and to have some variability.
 * urandom would be nice, but it's not that important that it's truly
 * random, just fairly hard to predict and not stable across a bunch of
 * different machines.
 *
 * So we're just assigning it to some address that'll get relocated during
 * startup.  It'll get reset during initialization soon after.
 */
uintptr_t __stack_chk_guard = (uintptr_t)stack_protector_init;

void NORETURN
__stack_chk_fail(void)
{
	do {
		console_print(L"Stack corruption detected.  Shutting down in 5 seconds.\n");
		backtrace(0);
		usleep(5000000);
		RT->ResetSystem(EfiResetShutdown, EFI_SECURITY_VIOLATION, 0, NULL);

		/*
		 * We should never get here, but if we do, we have to do
		 * /something/.
		 */
		wait_for_debug();
	} while (1);
}

/*
 * For reasons unknown, when we build for ia32 using:
 *
 * x86_64-linux-gnu-gcc -fstack-protector -mstack-protector-guard=global -m32 ...
 *
 * a few objects wind up referencing __stack_chk_fail_local() instead of
 * __stack_chk_fail().  I really don't understand why, but... eh.
 */
void NORETURN HIDDEN
__stack_chk_fail_local (void)
{
  __stack_chk_fail ();
}

uintptr_t
stack_protector_init(void)
{
	EFI_RNG_PROTOCOL *rng = NULL;
	EFI_STATUS efi_status;
	uintptr_t guard = __stack_chk_guard;

	efi_status = LibLocateProtocol(&EFI_RNG_PROTOCOL_GUID, (VOID *)&rng);
	if (EFI_ERROR(efi_status)) {
		perror(L"Could not find EFI RNG protocol: %r\n", efi_status);
		return guard;
	}

	efi_status = rng->GetRNG(rng, NULL, sizeof(guard)-1, (uint8_t *)&guard);
	if (EFI_ERROR(efi_status)) {
		perror(L"Could not get random data: %r\n", efi_status);
		return guard;
	}
	return guard;
}

// vim:fenc=utf-8:tw=75:noet
