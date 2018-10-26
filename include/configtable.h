#ifndef SHIM_CONFIGTABLE_H
#define SHIM_CONFIGTABLE_H

void *
configtable_get_table(EFI_GUID *guid);
EFI_IMAGE_EXECUTION_INFO_TABLE *
configtable_get_image_table(void);
EFI_IMAGE_EXECUTION_INFO *
configtable_find_image(const EFI_DEVICE_PATH *DevicePath);
int
configtable_image_is_forbidden(const EFI_DEVICE_PATH *DevicePath);

#endif /* SHIM_CONFIGTABLE_H */
