#ifndef SHIM_NETFILE_H
#define SHIM_NETFILE_H

extern BOOLEAN findNetfile(EFI_HANDLE image_handle);

extern EFI_STATUS parseNetfileinfo(EFI_HANDLE image_handle, const char *file_name);

extern EFI_STATUS FetchNetfile(EFI_HANDLE image_handle, VOID **buffer, UINT64 *bufsiz);

#endif /* SHIM_NETFILE_H */
