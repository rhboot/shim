EFI_STATUS
generate_path(CHAR16* name, EFI_LOADED_IMAGE *li,
	      EFI_DEVICE_PATH **path, CHAR16 **PathName);
EFI_STATUS
execute(EFI_HANDLE image, CHAR16 *name);
