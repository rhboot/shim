// SPDX-License-Identifier: BSD-2-Clause-Patent

/*
 * Copyright Red Hat, Inc
 * Copyright Peter Jones <pjones@redhat.com>
 */
#ifndef SHIM_REPLACEMENTS_H
#define SHIM_REPLACEMENTS_H

extern EFI_SYSTEM_TABLE *get_active_systab(void);

typedef enum {
	VERIFIED_BY_NOTHING,
	VERIFIED_BY_CERT,
	VERIFIED_BY_HASH
} verification_method_t;

extern verification_method_t verification_method;
extern int loader_is_participating;

extern void hook_system_services(EFI_SYSTEM_TABLE *local_systab);
extern void unhook_system_services(void);

extern void hook_exit(EFI_SYSTEM_TABLE *local_systab);
extern void unhook_exit(void);

extern EFI_STATUS install_shim_protocols(void);
extern void uninstall_shim_protocols(void);

#endif /* SHIM_REPLACEMENTS_H */
