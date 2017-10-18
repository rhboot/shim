/*
 * mok.c
 * Copyright 2017 Peter Jones <pjones@redhat.com>
 *
 * Distributed under terms of the GPLv3 license.
 */

#include "shim.h"

/*
 * Check if a variable exists
 */
static BOOLEAN check_var(CHAR16 *varname)
{
	EFI_STATUS efi_status;
	UINTN size = sizeof(UINT32);
	UINT32 MokVar;
	UINT32 attributes;

	efi_status = gRT->GetVariable(varname, &SHIM_LOCK_GUID, &attributes,
				      &size, (void *)&MokVar);
	if (!EFI_ERROR(efi_status) || efi_status == EFI_BUFFER_TOO_SMALL)
		return TRUE;

	return FALSE;
}

/*
 * If the OS has set any of these variables we need to drop into MOK and
 * handle them appropriately
 */
static EFI_STATUS check_mok_request(EFI_HANDLE image_handle)
{
	EFI_STATUS efi_status;

	if (check_var(L"MokNew") || check_var(L"MokSB") ||
	    check_var(L"MokPW") || check_var(L"MokAuth") ||
	    check_var(L"MokDel") || check_var(L"MokDB") ||
	    check_var(L"MokXNew") || check_var(L"MokXDel") ||
	    check_var(L"MokXAuth")) {
		efi_status = start_image(image_handle, MOK_MANAGER);

		if (EFI_ERROR(efi_status)) {
			perror(L"Failed to start MokManager: %r\n", efi_status);
			return efi_status;
		}
	}

	return EFI_SUCCESS;
}

/*
 * MoK variables that need to have their storage validated.
 *
 * The order here is important, since this is where we measure for the
 * tpm as well.
 */
struct mok_state_variable {
	CHAR16 *name;
	char *name8;
	CHAR16 *rtname;
	EFI_GUID *guid;
	UINT8 *data;
	UINTN data_size;
	/*
	 * These two are indirect pointers just to make initialization
	 * saner...
	 */
	UINT8 **addend_source;
	UINT32 *addend_size;
	UINT32 yes_attr;
	UINT32 no_attr;
	UINT32 flags;
	UINTN pcr;
	UINT8 *state;
};

#define MOK_MIRROR_KEYDB	0x01
#define MOK_MIRROR_DELETE_FIRST	0x02
#define MOK_VARIABLE_MEASURE	0x04
#define MOK_VARIABLE_LOG	0x08

struct mok_state_variable mok_state_variables[] = {
	{.name = L"MokList",
	 .name8 = "MokList",
	 .rtname = L"MokListRT",
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
	 .addend_source = &vendor_cert,
	 .addend_size = &vendor_cert_size,
	 .flags = MOK_MIRROR_KEYDB |
		  MOK_VARIABLE_LOG,
	 .pcr = 14,
	},
	{.name = L"MokListX",
	 .name8 = "MokListX",
	 .rtname = L"MokListXRT",
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
	 .flags = MOK_MIRROR_KEYDB |
		  MOK_VARIABLE_LOG,
	 .pcr = 14,
	},
	{.name = L"MokSBState",
	 .name8 = "MokSBState",
	 .rtname = L"MokSBStateRT",
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
	 .flags = MOK_MIRROR_DELETE_FIRST |
		  MOK_VARIABLE_MEASURE |
		  MOK_VARIABLE_LOG,
	 .pcr = 14,
	 .state = &user_insecure_mode,
	},
	{.name = L"MokDBState",
	 .name8 = "MokDBState",
	 .rtname = L"MokIgnoreDB",
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
	 .state = &ignore_db,
	},
	{ NULL, }
};

static EFI_STATUS mirror_one_mok_variable(struct mok_state_variable *v)
{
	EFI_STATUS efi_status = EFI_SUCCESS;
	void *FullData = NULL;
	UINTN FullDataSize = 0;
	uint8_t *p = NULL;

	if ((v->flags & MOK_MIRROR_KEYDB) &&
	    v->addend_source && *v->addend_source &&
	    v->addend_size && *v->addend_size) {
		EFI_SIGNATURE_LIST *CertList = NULL;
		EFI_SIGNATURE_DATA *CertData = NULL;
		FullDataSize = v->data_size
			     + sizeof (*CertList)
			     + sizeof (EFI_GUID)
			     + *v->addend_size;
		FullData = AllocatePool(FullDataSize);
		if (!FullData) {
			perror(L"Failed to allocate space for MokListRT\n");
			return EFI_OUT_OF_RESOURCES;
		}
		p = FullData;

		if (!EFI_ERROR(efi_status) && v->data_size > 0) {
			CopyMem(p, v->data, v->data_size);
			p += v->data_size;
		}
		CertList = (EFI_SIGNATURE_LIST *)p;
		p += sizeof (*CertList);
		CertData = (EFI_SIGNATURE_DATA *)p;
		p += sizeof (EFI_GUID);

		CertList->SignatureType = EFI_CERT_TYPE_X509_GUID;
		CertList->SignatureListSize = *v->addend_size
					      + sizeof (*CertList)
					      + sizeof (*CertData)
					      -1;
		CertList->SignatureHeaderSize = 0;
		CertList->SignatureSize = *v->addend_size + sizeof (EFI_GUID);

		CertData->SignatureOwner = SHIM_LOCK_GUID;
		CopyMem(p, *v->addend_source, *v->addend_size);

		if (v->data && v->data_size)
			FreePool(v->data);
		v->data = FullData;
		v->data_size = FullDataSize;
	} else {
		FullDataSize = v->data_size;
		FullData = v->data;
	}

	if (FullDataSize) {
		efi_status = gRT->SetVariable(v->rtname, v->guid,
					      EFI_VARIABLE_BOOTSERVICE_ACCESS |
					      EFI_VARIABLE_RUNTIME_ACCESS,
					      FullDataSize, FullData);
		if (EFI_ERROR(efi_status)) {
			perror(L"Failed to set %s: %r\n",
			       v->rtname, efi_status);
		}
	}

	return efi_status;
}

/*
 * Verify our non-volatile MoK state.  This checks the variables above
 * accessable and have valid attributes.  If they don't, it removes
 * them.  If any of them can't be removed, our ability to do this is
 * comprimized, so return EFI_SECURITY_VIOLATION.
 *
 * Any variable that isn't deleted and has ->measure == TRUE is then
 * measured into the tpm.
 *
 * Any variable with a ->rtname element is then mirrored to a
 * runtime-accessable version.  The new ones won't be marked NV, so the OS
 * can't modify them.
 */
EFI_STATUS import_mok_state(EFI_HANDLE image_handle)
{
	UINTN i;
	EFI_STATUS ret = EFI_SUCCESS;
	EFI_STATUS efi_status;

	user_insecure_mode = 0;
	ignore_db = 0;

	for (i = 0; mok_state_variables[i].name != NULL; i++) {
		struct mok_state_variable *v = &mok_state_variables[i];
		UINT32 attrs = 0;
		BOOLEAN delete = FALSE, present, addend;

		efi_status = get_variable_attr(v->name,
					       &v->data, &v->data_size,
					       *v->guid, &attrs);
		if (efi_status == EFI_NOT_FOUND)
			continue;
		if (EFI_ERROR(efi_status)) {
			perror(L"Could not verify %s: %r\n", v->name,
			       efi_status);
			/*
			 * don't clobber EFI_SECURITY_VIOLATION from some
			 * other variable in the list.
			 */
			if (ret != EFI_SECURITY_VIOLATION)
				ret = efi_status;
			continue;
		}

		if (!(attrs & v->yes_attr)) {
			perror(L"Variable %s is missing attributes:\n",
			       v->name);
			perror(L"  0x%08x should have 0x%08x set.\n",
			       attrs, v->yes_attr);
			delete = TRUE;
		}
		if (attrs & v->no_attr) {
			perror(L"Variable %s has incorrect attribute:\n",
			       v->name);
			perror(L"  0x%08x should not have 0x%08x set.\n",
			       attrs, v->no_attr);
			delete = TRUE;
		}
		if (delete == TRUE) {
			perror(L"Deleting bad variable %s\n", v->name);
			efi_status = LibDeleteVariable(v->name, v->guid);
			if (EFI_ERROR(efi_status)) {
				perror(L"Failed to erase %s\n", v->name);
				ret = EFI_SECURITY_VIOLATION;
			}
			FreePool(v->data);
			v->data = NULL;
			v->data_size = 0;
			continue;
		}

		if (v->data && v->data_size == sizeof(UINT8) && v->state) {
			*v->state = v->data[0];
		}

		present = (v->data && v->data_size) ? TRUE : FALSE;
		addend = (v->addend_source && v->addend_size &&
			  *v->addend_source && *v->addend_size)
			? TRUE : FALSE;

		if (v->flags & MOK_VARIABLE_MEASURE && present) {
			/*
			 * Measure this into PCR 7 in the Microsoft format
			 */
			efi_status = tpm_measure_variable(v->name, *v->guid,
							  v->data_size,
							  v->data);
			if (EFI_ERROR(efi_status)) {
				if (ret != EFI_SECURITY_VIOLATION)
					ret = efi_status;
			}
		}

		if (v->flags & MOK_VARIABLE_LOG && present) {
			/*
			 * Log this variable into whichever PCR the table
			 * says.
			 */
			EFI_PHYSICAL_ADDRESS datap =
					(EFI_PHYSICAL_ADDRESS)(UINTN)v->data,
			efi_status = tpm_log_event(datap, v->data_size,
						   v->pcr, (CHAR8 *)v->name8);
			if (EFI_ERROR(efi_status)) {
				if (ret != EFI_SECURITY_VIOLATION)
					ret = efi_status;
			}
		}

		if (v->rtname && present && addend) {
			if (v->flags & MOK_MIRROR_DELETE_FIRST)
				LibDeleteVariable(v->rtname, v->guid);

			efi_status = mirror_one_mok_variable(v);
			if (EFI_ERROR(efi_status) &&
			    ret != EFI_SECURITY_VIOLATION)
				ret = efi_status;
		}
	}

	/*
	 * Enter MokManager if necessary.  Any actual *changes* here will
	 * cause MokManager to demand a machine reboot, so this is safe to
	 * have after the entire loop.
	 */
	efi_status = check_mok_request(image_handle);
	if (EFI_ERROR(efi_status)) {
		if (ret != EFI_SECURITY_VIOLATION)
			ret = efi_status;
		return ret;
	}


	return ret;
}

// vim:fenc=utf-8:tw=75
