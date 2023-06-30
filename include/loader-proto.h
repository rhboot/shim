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

extern void hook_system_services(EFI_SYSTEM_TABLE *local_systab);
extern void unhook_system_services(void);

extern void hook_exit(EFI_SYSTEM_TABLE *local_systab);
extern void unhook_exit(void);

typedef struct _SHIM_IMAGE_LOADER {
	EFI_IMAGE_LOAD LoadImage;
	EFI_IMAGE_START StartImage;
	EFI_EXIT Exit;
	EFI_IMAGE_UNLOAD UnloadImage;
} SHIM_IMAGE_LOADER;

extern SHIM_IMAGE_LOADER shim_image_loader_interface;
extern void init_image_loader(void);

#endif /* SHIM_REPLACEMENTS_H */
