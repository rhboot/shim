// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * mok.c - MoK variable processing
 * Copyright 2017 Peter Jones <pjones@redhat.com>
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

	efi_status = RT->GetVariable(varname, &SHIM_LOCK_GUID, &attributes,
				     &size, (void *)&MokVar);
	if (!EFI_ERROR(efi_status) || efi_status == EFI_BUFFER_TOO_SMALL)
		return TRUE;

	return FALSE;
}

#define SetVariable(name, guid, attrs, varsz, var)                                  \
	({                                                                          \
		EFI_STATUS efi_status_;                                             \
		efi_status_ = RT->SetVariable(name, guid, attrs, varsz, var);       \
		dprint_(L"%a:%d:%a() SetVariable(\"%s\", ... varsz=0x%llx) = %r\n", \
		        __FILE__, __LINE__ - 5, __func__, name, varsz,              \
		        efi_status_);                                               \
		efi_status_;                                                        \
	})

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
	    check_var(L"MokXAuth") || check_var(L"MokListTrustedNew")) {
		efi_status = start_image(image_handle, MOK_MANAGER);

		if (EFI_ERROR(efi_status)) {
			perror(L"Failed to start MokManager: %r\n", efi_status);
			return efi_status;
		}
	}

	return EFI_SUCCESS;
}

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
#define MOK_VARIABLE_INVERSE	0x10

struct mok_state_variable mok_state_variable_data[] = {
	{.name = L"MokList",
	 .name8 = "MokList",
	 .rtname = L"MokListRT",
	 .rtname8 = "MokListRT",
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
	 .categorize_addend = categorize_authorized,
	 .addend = &vendor_authorized,
	 .addend_size = &vendor_authorized_size,
	 .user_cert = &user_cert,
	 .user_cert_size = &user_cert_size,
#if defined(ENABLE_SHIM_CERT)
	 .build_cert = &build_cert,
	 .build_cert_size = &build_cert_size,
#endif /* defined(ENABLE_SHIM_CERT) */
	 .flags = MOK_MIRROR_KEYDB |
		  MOK_MIRROR_DELETE_FIRST |
		  MOK_VARIABLE_LOG,
	 .pcr = 14,
	},
	{.name = L"MokListX",
	 .name8 = "MokListX",
	 .rtname = L"MokListXRT",
	 .rtname8 = "MokListXRT",
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
	 .categorize_addend = categorize_deauthorized,
	 .addend = &vendor_deauthorized,
	 .addend_size = &vendor_deauthorized_size,
	 .flags = MOK_MIRROR_KEYDB |
		  MOK_MIRROR_DELETE_FIRST |
		  MOK_VARIABLE_LOG,
	 .pcr = 14,
	},
	{.name = L"MokSBState",
	 .name8 = "MokSBState",
	 .rtname = L"MokSBStateRT",
	 .rtname8 = "MokSBStateRT",
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
	 .rtname8 = "MokIgnoreDB",
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
	 .state = &ignore_db,
	},
	{.name = SBAT_VAR_NAME,
	 .name8 = SBAT_VAR_NAME8,
	 .rtname = SBAT_RT_VAR_NAME,
	 .rtname8 = SBAT_RT_VAR_NAME8,
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 /*
	  * we're enforcing that SBAT can't have an RT flag here because
	  * there's no way to tell whether it's an authenticated variable.
	  */
#if !defined(ENABLE_SHIM_DEVEL)
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
#else
	 .no_attr = 0,
#endif
	 .flags = MOK_MIRROR_DELETE_FIRST |
		  MOK_VARIABLE_MEASURE,
	 .pcr = 7,
	},
	{.name = L"MokListTrusted",
	 .name8 = "MokListTrusted",
	 .rtname = L"MokListTrustedRT",
	 .rtname8 = "MokListTrustedRT",
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
	 .flags = MOK_MIRROR_DELETE_FIRST |
		  MOK_VARIABLE_MEASURE |
		  MOK_VARIABLE_INVERSE |
		  MOK_VARIABLE_LOG,
	 .pcr = 14,
	 .state = &trust_mok_list,
	},
	{.name = L"MokPolicy",
	 .name8 = "MokPolicy",
	 .rtname = L"MokPolicyRT",
	 .rtname8 = "MokPolicyRT",
	 .guid = &SHIM_LOCK_GUID,
	 .yes_attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
		     EFI_VARIABLE_NON_VOLATILE,
	 .no_attr = EFI_VARIABLE_RUNTIME_ACCESS,
	 .flags = MOK_MIRROR_DELETE_FIRST |
		  MOK_VARIABLE_LOG,
	 .pcr = 14,
	 .state = &mok_policy,
	},
	{ NULL, }
};
size_t n_mok_state_variables = sizeof(mok_state_variable_data) / sizeof(mok_state_variable_data[0]);
struct mok_state_variable *mok_state_variables = &mok_state_variable_data[0];

#define should_mirror_addend(v) (((v)->categorize_addend) && ((v)->categorize_addend(v) != VENDOR_ADDEND_NONE))

static inline BOOLEAN NONNULL(1)
should_mirror_build_cert(struct mok_state_variable *v)
{
	return (v->build_cert && v->build_cert_size &&
		*v->build_cert && *v->build_cert_size) ? TRUE : FALSE;
}

static const uint8_t null_sha256[32] = { 0, };

typedef UINTN SIZE_T;

#define EFI_MAJOR_VERSION(tablep) ((UINT16)((((tablep)->Hdr.Revision) >> 16) & 0xfffful))
#define EFI_MINOR_VERSION(tablep) ((UINT16)(((tablep)->Hdr.Revision) & 0xfffful))

static EFI_STATUS
get_max_var_sz(UINT32 attrs, SIZE_T *max_var_szp)
{
	EFI_STATUS efi_status;
	uint64_t max_storage_sz = 0;
	uint64_t remaining_sz = 0;
	uint64_t max_var_sz = 0;

	*max_var_szp = 0;
	if (EFI_MAJOR_VERSION(RT) < 2) {
		dprint(L"EFI %d.%d; no RT->QueryVariableInfo().  Using 1024!\n",
		       EFI_MAJOR_VERSION(RT), EFI_MINOR_VERSION(RT));
		max_var_sz = remaining_sz = max_storage_sz = 1024;
		efi_status = EFI_SUCCESS;
	} else {
		dprint(L"calling RT->QueryVariableInfo() at 0x%lx\n",
		       RT->QueryVariableInfo);
		efi_status = RT->QueryVariableInfo(attrs, &max_storage_sz,
						   &remaining_sz, &max_var_sz);
		if (EFI_ERROR(efi_status)) {
			perror(L"Could not get variable storage info: %r\n",
			       efi_status);
			return efi_status;
		}
	}

	/*
	 * I just don't trust implementations to not be showing static data
	 * for max_var_sz
	 */
	*max_var_szp = (max_var_sz < remaining_sz) ? max_var_sz : remaining_sz;
	dprint("max_var_sz:%lx remaining_sz:%lx max_storage_sz:%lx\n",
		max_var_sz, remaining_sz, max_storage_sz);
	return efi_status;
}

/*
 * If any entries fit in < maxsz, and nothing goes wrong, create a variable
 * of the given name and guid with as many esd entries as possible in it,
 * and updates *esdp with what would be the next entry (even if makes *esdp
 * > esl+esl->SignatureListSize), and returns whatever SetVariable()
 * returns
 *
 * If no entries fit (i.e. sizeof(esl) + esl->SignatureSize > maxsz),
 * returns EFI_BUFFER_TOO_SMALL;
 */
static EFI_STATUS
mirror_one_esl(CHAR16 *name, EFI_GUID *guid, UINT32 attrs,
	       EFI_SIGNATURE_LIST *esl, EFI_SIGNATURE_DATA *esd,
	       SIZE_T howmany)
{
	EFI_STATUS efi_status;
	SIZE_T varsz = 0;
	UINT8 *var;

	/*
	 * We always assume esl->SignatureHeaderSize is 0 (and so far,
	 * that's true as per UEFI 2.8)
	 */
	dprint(L"Trying to add %lx signatures to \"%s\" of size %lx\n",
	       howmany, name, esl->SignatureSize);

	/*
	 * Because of the semantics of variable_create_esl(), the first
	 * owner guid from the data is not part of esdsz, or the data.
	 *
	 * Compensate here.
	 */
	efi_status = variable_create_esl(esd, howmany,
					 &esl->SignatureType,
					 esl->SignatureSize,
					 &var, &varsz);
	if (EFI_ERROR(efi_status) || !var || !varsz) {
		LogError(L"Couldn't allocate %lu bytes for mok variable \"%s\": %r\n",
			 varsz, var, efi_status);
		return efi_status;
	}

	dprint(L"new esl:\n");
	dhexdumpat(var, varsz, 0);

	efi_status = SetVariable(name, guid, attrs, varsz, var);
	FreePool(var);
	if (EFI_ERROR(efi_status)) {
		LogError(L"Couldn't create mok variable \"%s\": %r\n",
			 varsz, var, efi_status);
		return efi_status;
	}

	return efi_status;
}

static EFI_STATUS
mirror_mok_db(CHAR16 *name, CHAR8 *name8, EFI_GUID *guid, UINT32 attrs,
	      UINT8 *FullData, SIZE_T FullDataSize, BOOLEAN only_first)
{
	EFI_STATUS efi_status = EFI_SUCCESS;
	SIZE_T max_var_sz;

	efi_status = get_max_var_sz(attrs, &max_var_sz);
	if (EFI_ERROR(efi_status) && efi_status != EFI_UNSUPPORTED) {
		LogError(L"Could not get maximum variable size: %r",
			 efi_status);
		return efi_status;
	}

	/* Some UEFI environment such as u-boot doesn't implement
	 * QueryVariableInfo() and we will only get EFI_UNSUPPORTED when
	 * querying the available space. In this case, we just mirror
	 * the variable directly. */
	if (FullDataSize <= max_var_sz || efi_status == EFI_UNSUPPORTED) {
		efi_status = EFI_SUCCESS;
		if (only_first)
			efi_status = SetVariable(name, guid, attrs,
						 FullDataSize, FullData);

		return efi_status;
	}

	CHAR16 *namen;
	CHAR8 *namen8;
	UINTN namelen, namesz;

	namelen = StrLen(name);
	namesz = namelen * 2;
	if (only_first) {
		namen = name;
		namen8 = name8;
	} else {
		namelen += 18;
		namesz += 34;
		namen = AllocateZeroPool(namesz);
		if (!namen) {
			LogError(L"Could not allocate %lu bytes", namesz);
			return EFI_OUT_OF_RESOURCES;
		}
		namen8 = AllocateZeroPool(namelen);
		if (!namen8) {
			FreePool(namen);
			LogError(L"Could not allocate %lu bytes", namelen);
			return EFI_OUT_OF_RESOURCES;
		}
	}

	UINTN pos, i;
	const SIZE_T minsz = sizeof(EFI_SIGNATURE_LIST)
			     + sizeof(EFI_SIGNATURE_DATA)
			     + SHA1_DIGEST_SIZE;
	BOOLEAN did_one = FALSE;

	/*
	 * Create any entries that can fit.
	 */
	if (!only_first) {
		dprint(L"full data for \"%s\":\n", name);
		dhexdumpat(FullData, FullDataSize, 0);
	}
	EFI_SIGNATURE_LIST *esl = NULL;
	UINTN esl_end_pos = 0;
	for (i = 0, pos = 0; FullDataSize - pos >= minsz && FullData; ) {
		EFI_SIGNATURE_DATA *esd = NULL;

		dprint(L"pos:0x%llx FullDataSize:0x%llx\n", pos, FullDataSize);
		if (esl == NULL || pos >= esl_end_pos) {
			UINT8 *nesl = FullData + pos;
			dprint(L"esl:0x%llx->0x%llx\n", esl, nesl);
			esl = (EFI_SIGNATURE_LIST *)nesl;
			esl_end_pos = pos + esl->SignatureListSize;
			dprint(L"pos:0x%llx->0x%llx\n", pos, pos + sizeof(*esl));
			pos += sizeof(*esl);
		}
		esd = (EFI_SIGNATURE_DATA *)(FullData + pos);
		if (pos >= FullDataSize)
			break;
		if (esl->SignatureListSize == 0 || esl->SignatureSize == 0)
			break;

		dprint(L"esl[%lu] 0x%llx = {sls=0x%lx, ss=0x%lx} esd:0x%llx\n",
		       i, esl, esl->SignatureListSize, esl->SignatureSize, esd);

		if (!only_first) {
			SPrint(namen, namelen, L"%s%lu", name, i);
			namen[namelen-1] = 0;
			/* uggggh */
			UINTN j;
			for (j = 0; j < namelen; j++)
				namen8[j] = (CHAR8)(namen[j] & 0xff);
			namen8[namelen - 1] = 0;
		}

		/*
		 * In case max_var_sz is computed dynamically, refresh the
		 * value here.
		 */
		efi_status = get_max_var_sz(attrs, &max_var_sz);
		if (EFI_ERROR(efi_status)) {
			LogError(L"Could not get maximum variable size: %r",
				 efi_status);
			if (!only_first) {
				FreePool(namen);
				FreePool(namen8);
			}
			return efi_status;
		}

		/* The name counts towards the size of the variable */
		max_var_sz -= (StrLen(namen) + 1) * 2;
		dprint(L"max_var_sz - name: %lx\n", max_var_sz);

		SIZE_T howmany;
		howmany = MIN((max_var_sz - sizeof(*esl)) / esl->SignatureSize,
			      (esl_end_pos - pos) / esl->SignatureSize);
		if (howmany == 0) {
			/* No signatures from this ESL can be mirrored in to a
			 * single variable, so skip it.
			 */
			dprint(L"skipping esl, pos:0x%llx->0x%llx\n", pos, esl_end_pos);
			pos = esl_end_pos;
			continue;
		}

		UINTN adj = howmany * esl->SignatureSize;

		if (!only_first && i == 0) {
			dprint(L"pos:0x%llx->0x%llx\n", pos, pos + adj);
			pos += adj;
			i++;
			continue;

		}

		efi_status = mirror_one_esl(namen, guid, attrs,
					    esl, esd, howmany);
		dprint(L"esd:0x%llx adj:0x%llx\n", esd, adj);
		if (EFI_ERROR(efi_status)) {
			LogError(L"Could not mirror mok variable \"%s\": %r\n",
				 namen, efi_status);
			break;
		}

		dprint(L"pos:0x%llx->0x%llx\n", pos, pos + adj);
		pos += adj;
		did_one = TRUE;
		if (only_first)
			break;
		i++;
	}

	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to set %s: %r\n", name, efi_status);
	} else if (only_first && !did_one) {
		/*
		 * In this case we're going to try to create a
		 * dummy variable so that there's one there.  It
		 * may or may not work, because on some firmware
		 * builds when the SetVariable call above fails it
		 * does actually set the variable(!), so aside from
		 * not using the allocation if it doesn't work, we
		 * don't care about failures here.
		 */
		UINT8 *var;
		UINTN varsz;

		efi_status = variable_create_esl_with_one_signature(
				null_sha256, sizeof(null_sha256),
				&EFI_CERT_SHA256_GUID, &SHIM_LOCK_GUID,
				&var, &varsz);
		/*
		 * from here we don't really care if it works or
		 * doesn't.
		 */
		if (!EFI_ERROR(efi_status) && var && varsz) {
			efi_status = SetVariable(name, guid,
				    EFI_VARIABLE_BOOTSERVICE_ACCESS
				    | EFI_VARIABLE_RUNTIME_ACCESS,
				    varsz, var);
			FreePool(var);
		}
	}
	return efi_status;
}


static EFI_STATUS NONNULL(1)
mirror_one_mok_variable(struct mok_state_variable *v,
			BOOLEAN only_first)
{
	EFI_STATUS efi_status = EFI_SUCCESS;
	uint8_t *FullData = NULL;
	size_t FullDataSize = 0;
	vendor_addend_category_t addend_category = VENDOR_ADDEND_NONE;
	uint8_t *p = NULL;
	uint32_t attrs = EFI_VARIABLE_BOOTSERVICE_ACCESS |
			 EFI_VARIABLE_RUNTIME_ACCESS;
	BOOLEAN measure = v->flags & MOK_VARIABLE_MEASURE;
	BOOLEAN log = v->flags & MOK_VARIABLE_LOG;
	size_t build_cert_esl_sz = 0, addend_esl_sz = 0;
	bool reuse = FALSE;

	if (v->categorize_addend)
		addend_category = v->categorize_addend(v);

	/*
	 * if it is, there's more data
	 */
	if (v->flags & MOK_MIRROR_KEYDB) {

		/*
		 * We're mirroring (into) an efi security database, aka an
		 * array of EFI_SIGNATURE_LIST.  Its layout goes like:
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
		 * *first* vendor_db or vendor_cert
		 */
		switch (addend_category) {
		case VENDOR_ADDEND_DB:
			/*
			 * if it's an ESL already, we use it wholesale
			 */
			FullDataSize += *v->addend_size;
			dprint(L"FullDataSize:%lu FullData:0x%llx\n",
			       FullDataSize, FullData);
			break;
		case VENDOR_ADDEND_X509:
			efi_status = fill_esl_with_one_signature(*v->addend,
								 *v->addend_size,
								 &EFI_CERT_TYPE_X509_GUID,
								 &SHIM_LOCK_GUID,
								 NULL,
								 &addend_esl_sz);
			if (efi_status != EFI_BUFFER_TOO_SMALL) {
				perror(L"Could not add built-in cert to %s: %r\n",
				       v->name, efi_status);
				return efi_status;
			}
			FullDataSize += addend_esl_sz;
			dprint(L"FullDataSize:%lu FullData:0x%llx\n",
				      FullDataSize, FullData);
			break;
		default:
		case VENDOR_ADDEND_NONE:
			dprint(L"FullDataSize:%lu FullData:0x%llx\n",
				      FullDataSize, FullData);
			break;
		}

		/*
		 * then the build cert if it's there
		 */
		if (should_mirror_build_cert(v)) {
			efi_status = fill_esl_with_one_signature(*v->build_cert,
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
			dprint(L"FullDataSize:0x%lx FullData:0x%llx\n",
			       FullDataSize, FullData);
		}
		if (v->user_cert_size)
			FullDataSize += *v->user_cert_size;
	}

	/*
	 * we're always mirroring the original data, whether this is an efi
	 * security database or not
	 */
	dprint(L"v->name:\"%s\" v->rtname:\"%s\"\n", v->name, v->rtname);
	dprint(L"v->data_size:%lu v->data:0x%llx\n", v->data_size, v->data);
	dprint(L"FullDataSize:%lu FullData:0x%llx\n", FullDataSize, FullData);
	if (v->data_size) {
		FullDataSize += v->data_size;
		dprint(L"FullDataSize:%lu FullData:0x%llx\n",
		       FullDataSize, FullData);
	}
	if (v->data_size == FullDataSize)
		reuse = TRUE;

	/*
	 * Now we have the full size
	 */
	if (FullDataSize) {
		/*
		 * allocate the buffer, or use the old one if it's just the
		 * existing data.
		 */
		if (FullDataSize == v->data_size) {
			FullData = v->data;
			FullDataSize = v->data_size;
			p = FullData + FullDataSize;
			dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
			v->data = NULL;
			v->data_size = 0;
		} else {
			dprint(L"FullDataSize:%lu FullData:0x%llx allocating FullData\n",
			       FullDataSize, FullData);
			/*
			 * make sure we've got some zeroes at the end, just
			 * in case.
			 */
			UINTN new, allocsz;

			allocsz = FullDataSize + sizeof(EFI_SIGNATURE_LIST);
			new = ALIGN_VALUE(allocsz, 4096);
			allocsz = new == allocsz ? new + 4096 : new;
			FullData = AllocateZeroPool(allocsz);
			if (!FullData) {
				perror(L"Failed to allocate %lu bytes for %s\n",
				       FullDataSize, v->name);
				return EFI_OUT_OF_RESOURCES;
			}
			p = FullData;
		}
	}
	dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
	       FullDataSize, FullData, p, p-(uintptr_t)FullData);

	/*
	 * Now fill it.
	 */
	if (v->flags & MOK_MIRROR_KEYDB) {
		/*
		 * first vendor_cert or vendor_db
		 */
		switch (addend_category) {
		case VENDOR_ADDEND_DB:
			CopyMem(p, *v->addend, *v->addend_size);
			p += *v->addend_size;
			dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
			break;
		case VENDOR_ADDEND_X509:
			efi_status = fill_esl_with_one_signature(*v->addend,
								 *v->addend_size,
								 &EFI_CERT_TYPE_X509_GUID,
								 &SHIM_LOCK_GUID,
								 p, &addend_esl_sz);
			if (EFI_ERROR(efi_status)) {
				perror(L"Could not add built-in cert to %s: %r\n",
				       v->name, efi_status);
				return efi_status;
			}
			p += addend_esl_sz;
			dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
			break;
		default:
		case VENDOR_ADDEND_NONE:
			dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
			break;
		}

		/*
		 * then is the build cert
		 */
		dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
		       FullDataSize, FullData, p, p-(uintptr_t)FullData);
		if (should_mirror_build_cert(v)) {
			efi_status = fill_esl_with_one_signature(*v->build_cert,
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
			dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
			       FullDataSize, FullData, p, p-(uintptr_t)FullData);
		}
		if (v->user_cert_size) {
			CopyMem(p, *v->user_cert, *v->user_cert_size);
			p += *v->user_cert_size;
		}
	}

	/*
	 * last bit is existing data, unless it's the only thing,
	 * in which case it's already there.
	 */
	if (!reuse) {
		dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
		       FullDataSize, FullData, p, p-(uintptr_t)FullData);
		if (v->data && v->data_size) {
			CopyMem(p, v->data, v->data_size);
			p += v->data_size;
		}
		dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
		       FullDataSize, FullData, p, p-(uintptr_t)FullData);
	}

	/*
	 * We always want to create our key databases, so in this case we
	 * need a dummy entry
	 */
	if ((v->flags & MOK_MIRROR_KEYDB) && FullDataSize == 0) {
		efi_status = variable_create_esl_with_one_signature(
				null_sha256, sizeof(null_sha256),
				&EFI_CERT_SHA256_GUID, &SHIM_LOCK_GUID,
				&FullData, &FullDataSize);
		if (EFI_ERROR(efi_status)) {
			perror(L"Failed to allocate %lu bytes for %s\n",
			       FullDataSize, v->name);
			return efi_status;
		}
		p = FullData + FullDataSize;
		dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
		       FullDataSize, FullData, p, p-(uintptr_t)FullData);
	}

	dprint(L"FullDataSize:%lu FullData:0x%llx p:0x%llx pos:%lld\n",
	       FullDataSize, FullData, p, p-(uintptr_t)FullData);
	if (FullDataSize && v->flags & MOK_MIRROR_KEYDB) {
		dprint(L"calling mirror_mok_db(\"%s\",  datasz=%lu)\n",
		       v->rtname, FullDataSize);
		efi_status = mirror_mok_db(v->rtname, (CHAR8 *)v->rtname8, v->guid,
					   attrs, FullData, FullDataSize,
					   only_first);
		dprint(L"mirror_mok_db(\"%s\",  datasz=%lu) returned %r\n",
		       v->rtname, FullDataSize, efi_status);
	} else if (FullDataSize && only_first) {
		efi_status = SetVariable(v->rtname, v->guid, attrs,
					 FullDataSize, FullData);
	}
	if (FullDataSize && only_first) {
		if (measure) {
			/*
			 * Measure this into PCR 7 in the Microsoft format
			 */
			efi_status = tpm_measure_variable(v->name, *v->guid,
							  FullDataSize, FullData);
			if (EFI_ERROR(efi_status)) {
				dprint(L"tpm_measure_variable(\"%s\",%lu,0x%llx)->%r\n",
				       v->name, FullDataSize, FullData, efi_status);
				return efi_status;
			}
		}

		if (log) {
			/*
			 * Log this variable into whichever PCR the table
			 * says.
			 */
			EFI_PHYSICAL_ADDRESS datap =
					(EFI_PHYSICAL_ADDRESS)(UINTN)FullData,
			efi_status = tpm_log_event(datap, FullDataSize,
						   v->pcr, (CHAR8 *)v->name8);
			if (EFI_ERROR(efi_status)) {
				dprint(L"tpm_log_event(0x%llx, %lu, %lu, \"%s\")->%r\n",
				       FullData, FullDataSize, v->pcr, v->name,
				       efi_status);
				return efi_status;
			}
		}

	}
	if (v->data && v->data_size && v->data != FullData) {
		FreePool(v->data);
		v->data = NULL;
		v->data_size = 0;
	}
	v->data = FullData;
	v->data_size = FullDataSize;
	dprint(L"returning %r\n", efi_status);
	return efi_status;
}

/*
 * Mirror a variable if it has an rtname, and preserve any
 * EFI_SECURITY_VIOLATION status at the same time.
 */
static EFI_STATUS NONNULL(1)
maybe_mirror_one_mok_variable(struct mok_state_variable *v,
			      EFI_STATUS ret, BOOLEAN only_first)
{
	EFI_STATUS efi_status;
	BOOLEAN present = FALSE;

	if (v->rtname) {
		if (only_first && (v->flags & MOK_MIRROR_DELETE_FIRST)) {
			dprint(L"deleting \"%s\"\n", v->rtname);
			efi_status = LibDeleteVariable(v->rtname, v->guid);
			dprint(L"LibDeleteVariable(\"%s\",...) => %r\n", v->rtname, efi_status);
		}

		efi_status = mirror_one_mok_variable(v, only_first);
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

	return ret;
}

EFI_STATUS import_one_mok_state(struct mok_state_variable *v,
				BOOLEAN only_first)
{
	EFI_STATUS ret = EFI_SUCCESS;
	EFI_STATUS efi_status;

	UINT32 attrs = 0;
	BOOLEAN delete = FALSE;

	dprint(L"importing mok state for \"%s\"\n", v->name);

	if (!v->data && !v->data_size) {
		efi_status = get_variable_attr(v->name,
					       &v->data, &v->data_size,
					       *v->guid, &attrs);
		if (efi_status == EFI_NOT_FOUND &&
		    v->flags & MOK_VARIABLE_INVERSE) {
			v->data = AllocateZeroPool(4);
			if (!v->data) {
				perror(L"Out of memory\n");
				return EFI_OUT_OF_RESOURCES;
			}
			v->data[0] = 0x01;
			v->data_size = 1;
		} else if (efi_status == EFI_NOT_FOUND) {
			v->data = NULL;
			v->data_size = 0;
		} else if (EFI_ERROR(efi_status)) {
			perror(L"Could not verify %s: %r\n", v->name,
			       efi_status);
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
			if (v->flags & MOK_VARIABLE_INVERSE) {
				FreePool(v->data);
				v->data = NULL;
				v->data_size = 0;
			}
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

	dprint(L"maybe mirroring \"%s\".  original data:\n", v->name);
	if (v->data && v->data_size) {
		dhexdumpat(v->data, v->data_size, 0);
	}

	ret = maybe_mirror_one_mok_variable(v, ret, only_first);
	dprint(L"returning %r\n", ret);
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
	trust_mok_list = 0;

	UINT64 config_sz = 0;
	UINT8 *config_table = NULL;
	size_t npages = 0;
	struct mok_variable_config_entry config_template;

	dprint(L"importing minimal mok state variables\n");
	for (i = 0; mok_state_variables[i].name != NULL; i++) {
		struct mok_state_variable *v = &mok_state_variables[i];

		efi_status = import_one_mok_state(v, TRUE);
		if (EFI_ERROR(efi_status)) {
			dprint(L"import_one_mok_state(ih, \"%s\", TRUE): %r\n",
			       v->rtname);
			/*
			 * don't clobber EFI_SECURITY_VIOLATION from some
			 * other variable in the list.
			 */
			if (ret != EFI_SECURITY_VIOLATION)
				ret = efi_status;
		}

		if (v->data && v->data_size) {
			config_sz += v->data_size;
			config_sz += sizeof(config_template);
		}
	}

	/*
	 * Alright, so we're going to copy these to a config table.  The
	 * table is a packed array of N+1 struct mok_variable_config_entry
	 * items, with the last item having all zero's in name and
	 * data_size.
	 */
	if (config_sz) {
		config_sz += sizeof(config_template);
		npages = ALIGN_VALUE(config_sz, PAGE_SIZE) >> EFI_PAGE_SHIFT;
		config_table = NULL;
		efi_status = BS->AllocatePages(
			AllocateAnyPages, EfiRuntimeServicesData, npages,
			(EFI_PHYSICAL_ADDRESS *)&config_table);
		if (EFI_ERROR(efi_status) || !config_table) {
			console_print(L"Allocating %lu pages for mok config table failed: %r\n",
				      npages, efi_status);
			config_table = NULL;
		} else {
			ZeroMem(config_table, npages << EFI_PAGE_SHIFT);
		}
	}

	UINT8 *p = (UINT8 *)config_table;
	for (i = 0; p && mok_state_variables[i].name != NULL; i++) {
		struct mok_state_variable *v = &mok_state_variables[i];

		ZeroMem(&config_template, sizeof(config_template));
		strncpy(config_template.name, (CHAR8 *)v->rtname8, 255);
		config_template.name[255] = '\0';

		config_template.data_size = v->data_size;

		if (v->data && v->data_size) {
			CopyMem(p, &config_template, sizeof(config_template));
			p += sizeof(config_template);
			CopyMem(p, v->data, v->data_size);
			p += v->data_size;
		}
	}
	if (p) {
		ZeroMem(&config_template, sizeof(config_template));
		CopyMem(p, &config_template, sizeof(config_template));

		efi_status = BS->InstallConfigurationTable(&MOK_VARIABLE_STORE,
		                                           config_table);
		if (EFI_ERROR(efi_status)) {
			console_print(L"Couldn't install MoK configuration table\n");
		}
	}

	/*
	 * This is really just to make it easy for userland.
	 */
	dprint(L"importing full mok state variables\n");
	for (i = 0; mok_state_variables[i].name != NULL; i++) {
		struct mok_state_variable *v = &mok_state_variables[i];

		import_one_mok_state(v, FALSE);
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
		/*
		 * don't clobber EFI_SECURITY_VIOLATION
		 */
		if (ret != EFI_SECURITY_VIOLATION)
			ret = efi_status;
		return ret;
	}

	dprint(L"returning %r\n", ret);
	return ret;
}

// vim:fenc=utf-8:tw=75:noet
