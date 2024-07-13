// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SHIM_NETBOOT_H
#define SHIM_NETBOOT_H

#define SUPPRESS_NETBOOT_OPEN_FAILURE_NOISE 1

extern BOOLEAN findNetboot(EFI_HANDLE image_handle);

extern EFI_STATUS parseNetbootinfo(EFI_HANDLE image_handle, CHAR8 *name);

extern EFI_STATUS FetchNetbootimage(EFI_HANDLE image_handle, VOID **buffer,
	UINT64 *bufsiz, int flags);

#endif /* SHIM_NETBOOT_H */
