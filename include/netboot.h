#ifndef SHIM_NETBOOT_H
#define SHIM_NETBOOT_H

extern BOOLEAN findNetboot(EFI_HANDLE image_handle);

extern EFI_STATUS parseNetbootinfo(EFI_HANDLE image_handle);

extern EFI_STATUS FetchNetbootimage(EFI_HANDLE image_handle, VOID **buffer, UINT64 *bufsiz);

#endif /* SHIM_NETBOOT_H */
