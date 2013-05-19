EFI_STATUS
simple_file_open (EFI_HANDLE image, CHAR16 *name, EFI_FILE **file, UINT64 mode);
EFI_STATUS
simple_file_open_by_handle(EFI_HANDLE device, CHAR16 *name, EFI_FILE **file, UINT64 mode);
EFI_STATUS
simple_file_read_all(EFI_FILE *file, UINTN *size, void **buffer);
EFI_STATUS
simple_file_write_all(EFI_FILE *file, UINTN size, void *buffer);
void
simple_file_close(EFI_FILE *file);
EFI_STATUS
simple_dir_read_all(EFI_HANDLE image, CHAR16 *name, EFI_FILE_INFO **Entries,
		    int *count);
EFI_STATUS
simple_dir_filter(EFI_HANDLE image, CHAR16 *name, CHAR16 *filter,
		  CHAR16 ***result, int *count, EFI_FILE_INFO **entries);
void
simple_file_selector(EFI_HANDLE *im, CHAR16 **title, CHAR16 *name,
		     CHAR16 *filter, CHAR16 **result);
EFI_STATUS
simple_volume_selector(CHAR16 **title, CHAR16 **selected, EFI_HANDLE *h);
