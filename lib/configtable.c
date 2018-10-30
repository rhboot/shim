/*
 * Copyright 2013 <James.Bottomley@HansenPartnership.com>
 *
 * see COPYING file
 *
 * read some platform configuration tables
 */
#include <efi.h>
#include <efilib.h>
#include <dpath.h>

#include "shim.h"

void *
configtable_get_table(EFI_GUID *guid)
{
	unsigned int i;

	for (i = 0; i < gST->NumberOfTableEntries; i++) {
		EFI_CONFIGURATION_TABLE *CT = &gST->ConfigurationTable[i];

		if (CompareGuid(guid, &CT->VendorGuid) == 0) {
			return CT->VendorTable;
		}
	}
	return NULL;
}

EFI_IMAGE_EXECUTION_INFO_TABLE *
configtable_get_image_table(void)
{
	return configtable_get_table(&gEfiImageSecurityDatabaseGuid);
}

EFI_IMAGE_EXECUTION_INFO *
configtable_find_image(const EFI_DEVICE_PATH *DevicePath)
{
	EFI_IMAGE_EXECUTION_INFO_TABLE *t = configtable_get_image_table();

	if (!t)
		return NULL;

	UINTN entries = t->NumberOfImages;
	EFI_IMAGE_EXECUTION_INFO *e = (EFI_IMAGE_EXECUTION_INFO *)(t + 1);

	UINTN i;
	for (i = 0; i < entries; i++) {
#ifdef DEBUG_CONFIG
		console_print(L"InfoSize = %d  Action = %d\n", e->InfoSize, e->Action);

		/* print what we have for debugging */
		UINT8 *d = (UINT8 *)e; // + sizeof(UINT32)*2;
		console_print(L"Data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		      d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]); 
		d += 16;
		console_print(L"Data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		      d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]); 
		d += 16;
		console_print(L"Data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		      d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]); 
		d += 16;
		console_print(L"Data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		      d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]); 
		d += 16;
		console_print(L"Data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		      d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]); 
		d += 16;
		console_print(L"Data: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		      d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]); 
#endif
		UINT8 *data = (UINT8 *)(e + 1);
		CHAR16 *name = (CHAR16 *)data;
		UINTN skip = 0;

		/* There's a bug in a lot of EFI platforms and they forget to
		 * put the name here.  The only real way of detecting it is to
		 * look for either a UC16 NULL or ASCII as UC16 */
		if (name[0] == '\0' || (data[1] == 0 && data[3] == 0)) {
			skip = StrSize(name);
#ifdef DEBUG_CONFIG
			console_print(L"FOUND NAME %s (%zu)\n", name, skip);
#endif
		}
		EFI_DEVICE_PATH *dp = (EFI_DEVICE_PATH *)(data + skip), *dpn = dp;
		if (dp->Type == 0 || dp->Type > 6 || dp->SubType == 0
		    || ((unsigned)((dp->Length[1] << 8) + dp->Length[0]) > e->InfoSize)) {
			/* Parse error, table corrupt, bail */
			console_print(L"Image Execution Information table corrupt\n");
			break;
		}

		UINTN Size;
		DevicePathInstance(&dpn, &Size);
#ifdef DEBUG_CONFIG
		console_print(L"Path: %s\n", DevicePathToStr(dp));
		console_print(L"Device Path Size %d\n", Size);
#endif
		if (Size > e->InfoSize) {
			/* parse error; the platform obviously has a 
			 * corrupted image table; bail */
			console_print(L"Image Execution Information table corrupt\n");
			break;
		}
		
		if (CompareMem(dp, (void *)DevicePath, Size) == 0) {
#ifdef DEBUG_CONFIG
			console_print(L"***FOUND\n");
			console_get_keystroke();
#endif
			return e;
		}
		e = (EFI_IMAGE_EXECUTION_INFO *)((UINT8 *)e + e->InfoSize);
	}

#ifdef DEBUG_CONFIG
	console_print(L"***NOT FOUND\n");
	console_get_keystroke();
#endif

	return NULL;
}

int
configtable_image_is_forbidden(const EFI_DEVICE_PATH *DevicePath)
{
	EFI_IMAGE_EXECUTION_INFO *e = configtable_find_image(DevicePath);

	/* Image may not be in DB if it gets executed successfully If it is,
	 * and EFI_IMAGE_EXECUTION_INITIALIZED is not set, then the image
	 * isn't authenticated.  If there's no signature, usually
	 * EFI_IMAGE_EXECUTION_AUTH_UNTESTED is set, if the hash is in dbx,
	 * EFI_IMAGE_EXECUTION_AUTH_SIG_FOUND is returned, and if the key is
	 * in dbx, EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED is returned*/

	if (e && (e->Action == EFI_IMAGE_EXECUTION_AUTH_SIG_FOUND
		  || e->Action == EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED)) {
		/* this means the images signing key is in dbx */
#ifdef DEBUG_CONFIG
		console_print(L"SIGNATURE IS IN DBX, FORBIDDING EXECUTION\n");
#endif
		return 1;
	}

	return 0;
}
