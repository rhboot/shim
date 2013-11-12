#ifndef _NETBOOT_H_
#define _NETBOOT_H_

extern BOOLEAN findNetboot(EFI_HANDLE image_handle);

extern EFI_STATUS parseNetbootinfo(EFI_HANDLE image_handle);

extern EFI_STATUS FetchNetbootimage(EFI_HANDLE image_handle, VOID **buffer, UINT64 *bufsiz);
#endif
