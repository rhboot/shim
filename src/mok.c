/*
 * mok.c
 * Copyright 2017 Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

#include <stdint.h>

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

typedef enum {
	VENDOR_ADDEND_DB,
	VENDOR_ADDEND_X509,
	VENDOR_ADDEND_NONE,
} vendor_addend_category_t;

struct mok_state_variable;
typedef vendor_addend_category_t (vendor_addend_categorizer_t)(struct mok_state_variable *);

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
	 * These are indirect pointers just to make initialization saner...
	 */
	vendor_addend_categorizer_t *categorize_addend;
	UINT8 **addend;
	UINT32 *addend_size;

	UINT8 **build_cert;
	UINT32 *build_cert_size;

	UINT32 yes_attr;
	UINT32 no_attr;
	UINT32 flags;
	UINTN pcr;
	UINT8 *state;
};

static vendor_addend_category_t
categorize_authorized(struct mok_state_variable *v)
{
	if (!(v->addend && v->addend_size &&
	      *v->addend && *v->addend_size)) {
		return VENDOR_ADDEND_NONE;
	}

	return vendor_authorized_category;
}

static vendor_addend_category_t
categorize_deauthorized(struct mok_state_variable *v)
{
	if (!(v->addend && v->addend_size &&
	      *v->addend && *v->addend_size)) {
		return VENDOR_ADDEND_NONE;
	}

	return VENDOR_ADDEND_DB;
}

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
	 .categorize_addend = categorize_authorized,
	 .addend = &vendor_authorized,
	 .addend_size = &vendor_authorized_size,
#if defined(ENABLE_SHIM_CERT)
	 .build_cert = &build_cert,
	 .build_cert_size = &build_cert_size,
#endif /* defined(ENABLE_SHIM_CERT) */
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
	 .categorize_addend = categorize_deauthorized,
	 .addend = &vendor_deauthorized,
	 .addend_size = &vendor_deauthorized_size,
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

#define should_mirror_addend(v) (((v)->categorize_addend) && ((v)->categorize_addend(v) != VENDOR_ADDEND_NONE))

static inline BOOLEAN nonnull(1)
should_mirror_build_cert(struct mok_state_variable *v)
{
	return (v->build_cert && v->build_cert_size &&
		*v->build_cert && *v->build_cert_size) ? TRUE : FALSE;
}

static const uint8_t null_sha256[32] = { 0, };

static EFI_STATUS nonnull(1)
mirror_one_mok_variable(struct mok_state_variable *v)
{
	EFI_STATUS efi_status = EFI_SUCCESS;
	uint8_t *FullData = NULL;
	size_t FullDataSize = 0;
	vendor_addend_category_t addend_category = VENDOR_ADDEND_NONE;
	uint8_t *p = NULL;

	size_t build_cert_esl_sz = 0, addend_esl_sz = 0;

	if (v->categorize_addend)
		addend_category = v->categorize_addend(v);

	/*
	 * we're always mirroring the original data, whether this is an efi
	 * security database or not
	 */
	dprint(L"v->data_size:%lu v->data:0x%08llx\n", v->data_size, v->data);
	dprint(L"FullDataSize:%lu FullData:0x%08llx\n", FullDataSize, FullData);
	if (v->data_size) {
		FullDataSize = v->data_size;
		dprint(L"FullDataSize:%lu FullData:0x%08llx\n",
		       FullDataSize, FullData);
	}

	/*
	 * if it is, there's more data
	 */
	if (v->flags & MOK_MIRROR_KEYDB) {

		/*
		 * We're mirroring (into) an efi security database, aka an
		 * array of efi_signature_list_t.  Its layout goes like:
		 *
		 *   existing_variable_data
		 *   existing_variable_data_size
		 *   if flags & MOK_MIRROR_KEYDB
		 *     if build_cert
		 *       build_cert_esl
		 *       build_cert_header (always sz=0)
		 *       build_cert_esd[0] { owner, data }
		 *     if addend==vendor_db
		 *       for n=[1..N]
		 *         vendor_db_esl_n
		 *           vendor_db_header_n (always sz=0)
		 *           vendor_db_esd_n[m] {{ owner, data }, ... }
		 *     elif addend==vendor_cert
		 *       vendor_cert_esl
		 *         vendor_cert_header (always sz=0)
		 *         vendor_cert_esd[1] { owner, data }
		 *
		 * first we determine the size of the variable, then alloc
		 * and add the data.
		 */

		/*
		 * first bit is existing data, but we added that above
		 */

		/*
		 * then the build cert if it's there
		 */
		if (should_mirror_build_cert(v)) {
			efi_status = fill_esl(*v->build_cert,
					      *v->build_cert_size,
					      &EFI_CERT_TYPE_X509_GUID,
					      &SHIM_LOCK_GUID,
					      NULL, &build_cert_esl_sz);
			if (efi_status != EFI_BUFFER_TOO_SMALL) {
				perror(L"Could not add built-in cert to %s: %r\n",
				       v->name, efi_status);
				return efi_status;
			}
			FullDataSize += build_cert_esl_sz;
			dprint(L"FullDataSize:%lu FullData:0x%08llx\n",
			       FullDataSize, FullData);
		}

		/*
		 * then the addend data
		 */
		switch (addend_category) {
		case VENDOR_ADDEND_DB:
			/*
			 * if it's an ESL already, we use it wholesale
			 */
			FullDataSize += *v->addend_size;
			dprint(L"FullDataSize:%lu FullData:0x%08llx\n",
			       FullDataSize, FullData);
			break;
		case VENDOR_ADDEND_X509:
			efi_status = fill_esl(*v->addend, *v->addend_size,
					      &EFI_CERT_TYPE_X509_GUID,
					      &SHIM_LOCK_GUID,
					      NULL, &addend_esl_sz);
			if (efi_status != EFI_BUFFER_TOO_SMALL) {
				perror(L"Could not add built-in cert to %s: %r\n",
				       v->name, efi_status);
				return efi_status;
			}
			FullDataSize += addend_esl_sz;
			dprint(L"FullDataSize:%lu FullData:0x%08llx\n",
				      FullDataSize, FullData);
			break;
		default:
		case VENDOR_ADDEND_NONE:
			dprint(L"FullDataSize:%lu FullData:0x%08llx\n",
				      FullDataSize, FullData);
			break;
		}
	}

	/*
	 * Now we have the full size
	 */
	if (FullDataSize) {
		/*
		 * allocate the buffer, or use the old one if it's just the
		 * existing data.
		 */
		if (FullDataSize != v->data_size) {
			dprint(L"FullDataSize:%lu FullData:0x%08llx allocating FullData\n",
			       FullDataSize, FullData);
			FullData = AllocatePool(FullDataSize);
			if (!FullData) {
				FreePool(v->data);
				v->data = NULL;
				v->data_size = 0;
				perror(L"Failed to allocate %lu bytes for %s\n",
				       FullDataSize, v->name);
				return EFI_OUT_OF_RESOURCES;
			}
			p = FullData;
			dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
			if (v->data && v->data_size) {
				CopyMem(p, v->data, v->data_size);
				p += v->data_size;
			}
			dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
		} else {
			FullData = v->data;
			FullDataSize = v->data_size;
			p = FullData + FullDataSize;
			dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
			v->data = NULL;
			v->data_size = 0;
		}
	}
	dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
	       FullDataSize, FullData, p, p-(uintptr_t)FullData);

	/*
	 * Now fill it.
	 */
	if (v->flags & MOK_MIRROR_KEYDB) {
		/*
		 * first bit is existing data, but again, we added that above
		 */

		/*
		 * second is the build cert
		 */
		dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
		       FullDataSize, FullData, p, p-(uintptr_t)FullData);
		if (should_mirror_build_cert(v)) {
			efi_status = fill_esl(*v->build_cert,
					      *v->build_cert_size,
					      &EFI_CERT_TYPE_X509_GUID,
					      &SHIM_LOCK_GUID,
					      p, &build_cert_esl_sz);
			if (EFI_ERROR(efi_status)) {
				perror(L"Could not add built-in cert to %s: %r\n",
				       v->name, efi_status);
				return efi_status;
			}
			p += build_cert_esl_sz;
			dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
		}

		switch (addend_category) {
		case VENDOR_ADDEND_DB:
			CopyMem(p, *v->addend, *v->addend_size);
			p += *v->addend_size;
			dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
			break;
		case VENDOR_ADDEND_X509:
			efi_status = fill_esl(*v->addend, *v->addend_size,
					      &EFI_CERT_TYPE_X509_GUID,
					      &SHIM_LOCK_GUID,
					      p, &addend_esl_sz);
			if (EFI_ERROR(efi_status)) {
				perror(L"Could not add built-in cert to %s: %r\n",
				       v->name, efi_status);
				return efi_status;
			}
			p += addend_esl_sz;
			dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
			break;
		default:
		case VENDOR_ADDEND_NONE:
			dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
			break;
		}
	}
	/*
	 * We always want to create our key databases, so in this case we
	 * need a dummy entry
	 */
	if ((v->flags & MOK_MIRROR_KEYDB) && FullDataSize == 0) {
		efi_status = variable_create_esl(
				null_sha256, sizeof(null_sha256),
				&EFI_CERT_SHA256_GUID, &SHIM_LOCK_GUID,
				&FullData, &FullDataSize);
		if (EFI_ERROR(efi_status)) {
			perror(L"Failed to allocate %lu bytes for %s\n",
			       FullDataSize, v->name);
			return efi_status;
		}
		p = FullData + FullDataSize;
		dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
		       FullDataSize, FullData, p, p-(uintptr_t)FullData);
	}

	dprint(L"FullDataSize:%lu FullData:0x%08llx p:0x%08llx pos:%lld\n",
	       FullDataSize, FullData, p, p-(uintptr_t)FullData);
	if (FullDataSize) {
		dprint(L"Setting %s with %lu bytes of data\n",
		       v->rtname, FullDataSize);
		efi_status = gRT->SetVariable(v->rtname, v->guid,
					      EFI_VARIABLE_BOOTSERVICE_ACCESS |
					      EFI_VARIABLE_RUNTIME_ACCESS,
					      FullDataSize, FullData);
		if (EFI_ERROR(efi_status)) {
			perror(L"Failed to set %s: %r\n",
			       v->rtname, efi_status);
		}
	}
	if (v->data && v->data_size) {
		FreePool(v->data);
		v->data = NULL;
		v->data_size = 0;
	}
	if (FullData && FullDataSize) {
		FreePool(FullData);
	}
	dprint(L"returning %r\n", efi_status);
	return efi_status;
}

/*
 * Mirror a variable if it has an rtname, and preserve any
 * EFI_SECURITY_VIOLATION status at the same time.
 */
static EFI_STATUS nonnull(1)
maybe_mirror_one_mok_variable(struct mok_state_variable *v, EFI_STATUS ret)
{
	EFI_STATUS efi_status;
	BOOLEAN present = FALSE;

	if (v->rtname) {
		if (v->flags & MOK_MIRROR_DELETE_FIRST)
			LibDeleteVariable(v->rtname, v->guid);

		efi_status = mirror_one_mok_variable(v);
		if (EFI_ERROR(efi_status)) {
			if (ret != EFI_SECURITY_VIOLATION)
				ret = efi_status;
			perror(L"Could not create %s: %r\n", v->rtname,
			       efi_status);
		}
	}

	present = (v->data && v->data_size) ? TRUE : FALSE;
	if (!present)
		return ret;

	if (v->data_size == sizeof(UINT8) && v->state) {
		*v->state = v->data[0];
	}

	if (v->flags & MOK_VARIABLE_MEASURE) {
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

	if (v->flags & MOK_VARIABLE_LOG) {
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

	return ret;
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

	dprint(L"importing mok state\n");
	for (i = 0; mok_state_variables[i].name != NULL; i++) {
		struct mok_state_variable *v = &mok_state_variables[i];
		UINT32 attrs = 0;
		BOOLEAN delete = FALSE;

		efi_status = get_variable_attr(v->name,
					       &v->data, &v->data_size,
					       *v->guid, &attrs);
		dprint(L"maybe mirroring %s\n", v->name);
		if (efi_status == EFI_NOT_FOUND) {
			v->data = NULL;
			v->data_size = 0;
		} else if (EFI_ERROR(efi_status)) {
			perror(L"Could not verify %s: %r\n", v->name,
			       efi_status);
			/*
			 * don't clobber EFI_SECURITY_VIOLATION from some
			 * other variable in the list.
			 */
			if (ret != EFI_SECURITY_VIOLATION)
				ret = efi_status;
			delete = TRUE;
		} else {
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
		}

		ret = maybe_mirror_one_mok_variable(v, ret);
	}

	/*
	 * Enter MokManager if necessary.  Any actual *changes* here will
	 * cause MokManager to demand a machine reboot, so this is safe to
	 * have after the entire loop.
	 */
	dprint(L"checking mok request\n");
	efi_status = check_mok_request(image_handle);
	dprint(L"mok returned %r\n", efi_status);
	if (EFI_ERROR(efi_status)) {
		if (ret != EFI_SECURITY_VIOLATION)
			ret = efi_status;
		return ret;
	}

	dprint(L"returning %ld\n", ret);
	return ret;
}

// vim:fenc=utf-8:tw=75:noet
