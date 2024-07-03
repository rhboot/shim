// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * sbat.c - parse SBAT data from the .sbat section data
 */

#include "shim.h"
#include "ssp.h"
#include "ssp_var_defs.h"

extern struct {
	UINT32 automatic_offset;
	UINT32 latest_offset;
} sbat_var_payload_header;

static UINT8 sbat_policy = POLICY_NOTREAD;
static UINT8 ssp_policy = POLICY_NOTREAD;

EFI_STATUS
parse_sbat_section(char *section_base, size_t section_size,
		   size_t *n_entries,
		   struct sbat_section_entry ***entriesp)
{
	struct sbat_section_entry *entry = NULL, **entries = NULL;
	EFI_STATUS efi_status = EFI_SUCCESS;
	list_t csv, *pos = NULL;
	char * end = section_base + section_size - 1;
	size_t allocsz = 0;
	size_t n;
	char *strtab;

	if (!section_base || !section_size || !n_entries || !entriesp) {
		dprint(L"section_base:0x%lx section_size:0x%lx\n",
		       section_base, section_size);
		dprint(L"n_entries:0x%lx entriesp:0x%lx\n",
		       n_entries, entriesp);
		return EFI_INVALID_PARAMETER;
	}

	INIT_LIST_HEAD(&csv);

	efi_status =
		parse_csv_data(section_base, end, SBAT_SECTION_COLUMNS, &csv);
	if (EFI_ERROR(efi_status)) {
		dprint(L"parse_csv_data failed: %r\n", efi_status);
		return efi_status;
	}

	n = 0;
	list_for_each(pos, &csv) {
		struct csv_row * row;
		size_t i;

		row = list_entry(pos, struct csv_row, list);

		if (row->n_columns < SBAT_SECTION_COLUMNS) {
			efi_status = EFI_INVALID_PARAMETER;
			dprint(L"row->n_columns:%lu SBAT_SECTION_COLUMNS:%lu\n",
			       row->n_columns, SBAT_SECTION_COLUMNS);
			goto err;
		}

		allocsz += sizeof(struct sbat_section_entry *);
		allocsz += sizeof(struct sbat_section_entry);
		for (i = 0; i < row->n_columns; i++) {
			if (row->columns[i][0] == '\000') {
				dprint(L"row[%lu].columns[%lu][0] == '\\000'\n", n, i);
				efi_status = EFI_INVALID_PARAMETER;
				goto err;
			}
			allocsz += strlen(row->columns[i]) + 1;
		}
		n++;
	}

	/*
	 * Not necessarily actually an *error* since we eat newlines and
	 * the like; it could actually just be /empty/.
	 */
	if (n == 0)
		goto out;

	strtab = AllocateZeroPool(allocsz);
	if (!strtab) {
		efi_status = EFI_OUT_OF_RESOURCES;
		goto err;
	}

	entries = (struct sbat_section_entry **)strtab;
	strtab += sizeof(struct sbat_section_entry *) * n;
	entry = (struct sbat_section_entry *)strtab;
	strtab += sizeof(struct sbat_section_entry) * n;
	n = 0;

	list_for_each(pos, &csv) {
		struct csv_row * row;
		size_t i;
		const char **ptrs[] = {
			&entry->component_name,
			&entry->component_generation,
			&entry->vendor_name,
			&entry->vendor_package_name,
			&entry->vendor_version,
			&entry->vendor_url,
		};


		row = list_entry(pos, struct csv_row, list);
		for (i = 0; i < row->n_columns; i++) {
			*(ptrs[i]) = strtab;
			strtab = stpcpy(strtab, row->columns[i]) + 1;
		}
		entries[n] = entry;
		entry++;
		n++;
	}
out:
	*entriesp = entries;
	*n_entries = n;
err:
	free_csv_list(&csv);
	return efi_status;
}

void
cleanup_sbat_section_entries(size_t n, struct sbat_section_entry **entries)
{
	if (!n || !entries)
		return;

	FreePool(entries);
}

EFI_STATUS
verify_single_entry(struct sbat_section_entry *entry, struct sbat_var_entry *sbat_var_entry, bool *found)
{
	UINT16 sbat_gen, sbat_var_gen;

	if (strcmp((const char *)entry->component_name, (const char *)sbat_var_entry->component_name) == 0) {
		dprint(L"component %a has a matching SBAT variable entry, verifying\n",
			entry->component_name);
		*found = true;

		/*
		 * atoi returns zero for failed conversion, so essentially
		 * badly parsed component_generation will be treated as zero
		 */
		sbat_gen = atoi((const char *)entry->component_generation);
		sbat_var_gen = atoi((const char *)sbat_var_entry->component_generation);

		if (sbat_gen < sbat_var_gen) {
			dprint(L"component %a, generation %d, was revoked by %s variable\n",
			       entry->component_name, sbat_gen, SBAT_VAR_NAME);
			LogError(L"image did not pass SBAT verification\n");
			return EFI_SECURITY_VIOLATION;
		}
	}
	return EFI_SUCCESS;
}

void
cleanup_sbat_var(list_t *entries)
{
	list_t *pos = NULL, *tmp = NULL;
	struct sbat_var_entry *entry;
	void *first = NULL;

	list_for_each_safe(pos, tmp, entries) {
		entry = list_entry(pos, struct sbat_var_entry, list);

		if (first == NULL || (uintptr_t)entry < (uintptr_t)first)
			first = entry;

		list_del(&entry->list);
	}
	if (first)
		FreePool(first);
}

EFI_STATUS
verify_sbat_helper(list_t *local_sbat_var, size_t n, struct sbat_section_entry **entries)
{
	unsigned int i;
	list_t *pos = NULL;
	EFI_STATUS efi_status = EFI_SUCCESS;
	struct sbat_var_entry *sbat_var_entry;

	if (list_empty(local_sbat_var)) {
		dprint(L"%s variable not present\n", SBAT_VAR_NAME);
		return EFI_SUCCESS;
	}

	for (i = 0; i < n; i++) {
		list_for_each(pos, local_sbat_var) {
			bool found = false;
			sbat_var_entry = list_entry(pos, struct sbat_var_entry, list);
			efi_status = verify_single_entry(entries[i], sbat_var_entry, &found);
			if (EFI_ERROR(efi_status))
				goto out;
			if (found)
				break;
		}
	}

out:
	dprint(L"finished verifying SBAT data: %r\n", efi_status);
	return efi_status;
}

EFI_STATUS
verify_sbat(size_t n, struct sbat_section_entry **entries)
{
	EFI_STATUS efi_status;

	efi_status = verify_sbat_helper(&sbat_var, n, entries);
	return efi_status;
}

EFI_STATUS
parse_sbat_var_data(list_t *entry_list, UINT8 *data, UINTN datasize)
{
	struct sbat_var_entry *entry = NULL, **entries;
	EFI_STATUS efi_status = EFI_SUCCESS;
	list_t csv, *pos = NULL;
	char * start = (char *)data;
	char * end = (char *)data + datasize - 1;
	size_t allocsz = 0;
	size_t n;
	char *strtab;

	if (!entry_list|| !data || datasize == 0)
		return EFI_INVALID_PARAMETER;

	INIT_LIST_HEAD(&csv);

	efi_status = parse_csv_data(start, end, SBAT_VAR_COLUMNS, &csv);
	if (EFI_ERROR(efi_status)) {
		return efi_status;
	}

	n = 0;
	list_for_each(pos, &csv) {
		struct csv_row * row;
		size_t i;

		row = list_entry(pos, struct csv_row, list);

		if (row->n_columns < SBAT_VAR_REQUIRED_COLUMNS) {
			efi_status = EFI_INVALID_PARAMETER;
			goto err;
		}


		allocsz += sizeof(struct sbat_var_entry *);
		allocsz += sizeof(struct sbat_var_entry);
		for (i = 0; i < row->n_columns; i++) {
			if (!row->columns[i][0]) {
				efi_status = EFI_INVALID_PARAMETER;
				goto err;
			}
			allocsz += strlen(row->columns[i]) + 1;
		}
		n++;
	}

	strtab = AllocateZeroPool(allocsz);
	if (!strtab) {
		efi_status = EFI_OUT_OF_RESOURCES;
		goto err;
	}

	INIT_LIST_HEAD(entry_list);

	entry = (struct sbat_var_entry *)strtab;
	strtab += sizeof(struct sbat_var_entry) * n;
	entries = (struct sbat_var_entry **)strtab;
	strtab += sizeof(struct sbat_var_entry *) * n;
	n = 0;

	list_for_each(pos, &csv) {
		struct csv_row * row;
		size_t i;
		const char **ptrs[] = {
			&entry->component_name,
			&entry->component_generation,
			&entry->sbat_datestamp,
		};

		row = list_entry(pos, struct csv_row, list);
		for (i = 0; i < row->n_columns; i++) {
			*(ptrs[i]) = strtab;
			strtab = stpcpy(strtab, row->columns[i]) + 1;
		}
		INIT_LIST_HEAD(&entry->list);
		list_add_tail(&entry->list, entry_list);
		entries[n] = entry;
		entry++;
		n++;
	}
err:
	free_csv_list(&csv);
	return efi_status;
}

EFI_STATUS
parse_sbat_var(list_t *entries, char *sbat_var_candidate)
{
	UINT8 *data = 0;
	UINTN datasize;
	EFI_STATUS efi_status;
	list_t *pos = NULL;

	if (!entries) {
		dprint(L"entries is NULL\n");
		return EFI_INVALID_PARAMETER;
	}

	if (sbat_var_candidate == NULL) {
		dprint(L"sbat_var_candidate is NULL, reading variable\n");
		efi_status = get_variable(SBAT_VAR_NAME, &data, &datasize, SHIM_LOCK_GUID);
		if (EFI_ERROR(efi_status)) {
			LogError(L"Failed to read SBAT variable\n", efi_status);
			return efi_status;
		}
	} else {
		datasize = strlen(sbat_var_candidate);
		data = AllocatePool(datasize + 1);
		memcpy(data, sbat_var_candidate, datasize);
	}

	/*
	 * We've intentionally made sure there's a NUL byte on all variable
	 * allocations, so use that here.
	 */
	efi_status = parse_sbat_var_data(entries, data, datasize+1);
	if (EFI_ERROR(efi_status)) {
		dprint(L"parse_sbat_var_data() failed datasize: %d\n", datasize);
		goto out;
	}

	dprint(L"SBAT variable entries:\n");
	list_for_each(pos, entries) {
		struct sbat_var_entry *entry;

		entry = list_entry(pos, struct sbat_var_entry, list);
		dprint(L"%a, %a, %a\n", entry->component_name,
		       entry->component_generation, entry->sbat_datestamp);
	}

out:
	FreePool(data);
	return efi_status;
}

static bool
check_sbat_var_attributes(UINT32 attributes)
{
#ifdef ENABLE_SHIM_DEVEL
	return attributes == UEFI_VAR_NV_BS_RT;
#else
	return attributes == UEFI_VAR_NV_BS ||
	       attributes == UEFI_VAR_NV_BS_TIMEAUTH;
#endif
}

static char *
nth_sbat_field(char *str, size_t limit, int n)
{
	size_t i;
	for (i = 0; i < limit && str[i] != '\0'; i++) {
		if (n == 0)
			return &str[i];
		if (str[i] == ',')
			n--;
	}
	return &str[i];
}

bool
preserve_sbat_uefi_variable(UINT8 *sbat, UINTN sbatsize, UINT32 attributes,
		char *sbat_var)
{
	char *sbatc = (char *)sbat;
	char *current_version, *new_version,
	     *current_datestamp, *new_datestamp;
	int current_version_len, new_version_len;

	/* current metadata is not currupt somehow */
	if (!check_sbat_var_attributes(attributes) ||
               sbatsize < strlen(SBAT_VAR_ORIGINAL) ||
	       strncmp(sbatc, SBAT_VAR_SIG, strlen(SBAT_VAR_SIG)))
		return false;

	/* current metadata version not newer */
	current_version = nth_sbat_field(sbatc, sbatsize, 1);
	new_version = nth_sbat_field(sbat_var, strlen(sbat_var)+1, 1);
	current_datestamp = nth_sbat_field(sbatc, sbatsize, 2);
	new_datestamp = nth_sbat_field(sbat_var, strlen(sbat_var)+1, 2);

	current_version_len = current_datestamp - current_version - 1;
	new_version_len = new_datestamp - new_version - 1;

	if (current_version_len > new_version_len ||
	    (current_version_len == new_version_len &&
	    strncmp(current_version, new_version, new_version_len) > 0))
		return true;

	/* current datestamp is not newer or idential */
	if (strncmp(current_datestamp, new_datestamp,
	    strlen(SBAT_VAR_ORIGINAL_DATE)) >= 0)
                return true;

	return false;
}

/*
 * This looks kind of weird, but it comes directly from the MS
 * documentation:
 * https://support.microsoft.com/en-us/topic/kb5027455-guidance-for-blocking-vulnerable-windows-boot-managers-522bb851-0a61-44ad-aa94-ad11119c5e91
 */
static UINT64
ssp_ver_to_ull(UINT16 *ver)
{
	dprint("major: %u\n", ver[0]);
	dprint("minor: %u\n", ver[1]);
	dprint("rev: %u\n", ver[2]);
	dprint("build: %u\n", ver[3]);

	return ((UINT64)ver[0] << 48)
		+ ((UINT64)ver[1] << 32)
		+ ((UINT64)ver[2] << 16)
		+ ver[3];
}

static bool
preserve_ssp_uefi_variable(UINT8 *ssp_applied, UINTN sspversize, UINT32 attributes,
		uint8_t *ssp_candidate)
{
	UINT64 old, new;

	if (ssp_applied == NULL || ssp_candidate == NULL)
		return false;

	if (sspversize != SSPVER_SIZE)
		return false;

	if (!check_sbat_var_attributes(attributes))
		return false;

	old = ssp_ver_to_ull((UINT16 *)ssp_applied);
	new = ssp_ver_to_ull((UINT16 *)ssp_candidate);

	if (new > old)
		return false;
	else
		return true;
}

static void
clear_sbat_policy()
{
	EFI_STATUS efi_status = EFI_SUCCESS;

	efi_status = del_variable(SBAT_POLICY, SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status))
		console_error(L"Could not reset SBAT Policy", efi_status);
}

EFI_STATUS
set_sbat_uefi_variable(char *sbat_var_automatic, char *sbat_var_latest)
{
	EFI_STATUS efi_status = EFI_SUCCESS;
	UINT32 attributes = 0;

	UINT8 *sbat = NULL;
	UINT8 *sbat_policyp = NULL;
	UINTN sbatsize = 0;
	UINTN sbat_policysize = 0;

	char *sbat_var_candidate = NULL;
	bool reset_sbat = false;

	if (sbat_policy == POLICY_NOTREAD) {
		efi_status = get_variable_attr(SBAT_POLICY, &sbat_policyp,
		                               &sbat_policysize, SHIM_LOCK_GUID,
		                               &attributes);
		if (!EFI_ERROR(efi_status)) {
			sbat_policy = *sbat_policyp;
			clear_sbat_policy();
		}
	}

	if (EFI_ERROR(efi_status)) {
		dprint("Default sbat policy: automatic\n");
		if (secure_mode()) {
			sbat_var_candidate = sbat_var_automatic;
		} else {
			reset_sbat = true;
			sbat_var_candidate = SBAT_VAR_ORIGINAL;
		}
	} else {
		switch (sbat_policy) {
		case POLICY_LATEST:
			dprint("Custom sbat policy: latest\n");
			sbat_var_candidate = sbat_var_latest;
			break;
		case POLICY_AUTOMATIC:
			dprint("Custom sbat policy: automatic\n");
			sbat_var_candidate = sbat_var_automatic;
			break;
		case POLICY_RESET:
			if (secure_mode()) {
				console_print(L"Cannot reset SBAT policy: Secure Boot is enabled.\n");
				sbat_var_candidate = sbat_var_automatic;
			} else {
				dprint(L"Custom SBAT policy: reset OK\n");
				reset_sbat = true;
				sbat_var_candidate = SBAT_VAR_ORIGINAL;
			}
			break;
		default:
			console_error(L"SBAT policy state %llu is invalid",
				      EFI_INVALID_PARAMETER);
			if (secure_mode()) {
				sbat_var_candidate = sbat_var_automatic;
			} else {
				reset_sbat = true;
				sbat_var_candidate = SBAT_VAR_ORIGINAL;
			}
			break;
		}
	}

	efi_status = get_variable_attr(SBAT_VAR_NAME, &sbat, &sbatsize,
					SHIM_LOCK_GUID, &attributes);
	/*
	 * Always set the SbatLevel UEFI variable if it fails to read.
	 */
	if (EFI_ERROR(efi_status)) {
		dprint(L"SBAT read failed %r\n", efi_status);
	} else if (!reset_sbat &&
		   preserve_sbat_uefi_variable(sbat, sbatsize, attributes,
	                                       sbat_var_candidate)) {
		dprint(L"preserving %s variable it is %d bytes, attributes are 0x%08x\n",
		       SBAT_VAR_NAME, sbatsize, attributes);
		FreePool(sbat);
		return EFI_SUCCESS;
	} else {
		FreePool(sbat);

		/*
		 * parse the candidate SbatLevel and check that shim will not
		 * self revoke before writing SbatLevel variable
		 */
		dprint(L"shim SBAT reparse before application\n");
		efi_status = parse_sbat_var(&sbat_var, sbat_var_candidate);
		if (EFI_ERROR(efi_status)) {
			dprint(L"proposed SbatLevel failed to parse\n");
			return efi_status;
		}

#ifndef SHIM_UNIT_TEST
		char *sbat_start = (char *)&_sbat;
		char *sbat_end = (char *)&_esbat;
		efi_status = verify_sbat_section(sbat_start, sbat_end - sbat_start - 1);
		if (EFI_ERROR(efi_status)) {
			CHAR16 *title = L"New SbatLevel would self-revoke current shim. Not applied";
			CHAR16 *message = L"Press any key to continue";
			console_countdown(title, message, 10);
			return efi_status;
		}
#endif /* SHIM_UNIT_TEST */

		/* delete previous variable */
		dprint("%s variable is %d bytes, attributes are 0x%08x\n",
		       SBAT_VAR_NAME, sbatsize, attributes);
		dprint("Deleting %s variable.\n", SBAT_VAR_NAME);
		efi_status = set_variable(SBAT_VAR_NAME, SHIM_LOCK_GUID,
		                          attributes, 0, "");
		if (EFI_ERROR(efi_status)) {
			dprint(L"%s variable delete failed %r\n", SBAT_VAR_NAME,
					efi_status);
			return efi_status;
		}
	}

	/* set variable */
	efi_status = set_variable(SBAT_VAR_NAME, SHIM_LOCK_GUID, SBAT_VAR_ATTRS,
	                          strlen(sbat_var_candidate), sbat_var_candidate);
	if (EFI_ERROR(efi_status)) {
		dprint(L"%s variable writing failed %r\n", SBAT_VAR_NAME,
				efi_status);
		return efi_status;
	}

	/* verify that the expected data is there */
	efi_status = get_variable(SBAT_VAR_NAME, &sbat, &sbatsize,
				  SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
		dprint(L"%s read failed %r\n", SBAT_VAR_NAME, efi_status);
		return efi_status;
	}

	if (sbatsize != strlen(sbat_var_candidate) ||
	    strncmp((const char *)sbat, sbat_var_candidate,
		    strlen(sbat_var_candidate)) != 0) {
		dprint("new sbatsize is %d, expected %d\n", sbatsize,
		       strlen(sbat_var_candidate));
		efi_status = EFI_INVALID_PARAMETER;
	} else {
		dprint(L"%s variable initialization succeeded\n", SBAT_VAR_NAME);
	}

	FreePool(sbat);
	return efi_status;
}

EFI_STATUS
set_sbat_uefi_variable_internal(void)
{
	char *sbat_var_automatic;
	char *sbat_var_latest;

	sbat_var_automatic = (char *)&sbat_var_payload_header +
			    sbat_var_payload_header.automatic_offset;
	sbat_var_latest = (char *)&sbat_var_payload_header +
			  sbat_var_payload_header.latest_offset;

	return set_sbat_uefi_variable(sbat_var_automatic, sbat_var_latest);
}

static void
clear_ssp_policy(void)
{
	EFI_STATUS efi_status = EFI_SUCCESS;

	efi_status = del_variable(SSP_POLICY, SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status))
		console_error(L"Could not reset SSP Policy", efi_status);
}

static EFI_STATUS
clear_ssp_uefi_variables(void)
{
	EFI_STATUS efi_status, rc = EFI_SUCCESS;

	/* delete previous variable */
	dprint("Deleting %s variable.\n", SSPVER_VAR_NAME);
	efi_status = del_variable(SSPVER_VAR_NAME, SECUREBOOT_EFI_NAMESPACE_GUID);
	if (EFI_ERROR(efi_status)) {
		dprint(L"%s variable delete failed %r\n", SSPVER_VAR_NAME,
				efi_status);
		rc = efi_status;
	}
	dprint("Deleting %s variable.\n", SSPSIG_VAR_NAME);
	efi_status = del_variable(SSPSIG_VAR_NAME, SECUREBOOT_EFI_NAMESPACE_GUID);
	if (EFI_ERROR(efi_status)) {
		dprint(L"%s variable delete failed %r\n", SSPSIG_VAR_NAME,
				efi_status);
		rc = efi_status;
	}

	return rc;
}

EFI_STATUS
set_ssp_uefi_variable(uint8_t *ssp_ver_automatic, uint8_t *ssp_sig_automatic,
		uint8_t *ssp_ver_latest, uint8_t *ssp_sig_latest)
{
	EFI_STATUS efi_status = EFI_SUCCESS;
	UINT32 attributes = 0;

	UINT8 *sspver = NULL;
	UINT8 *policyp = NULL;
	UINTN sspversize = 0;
	UINTN policysize = 0;

	uint8_t *ssp_ver = NULL;
	uint8_t *ssp_sig = NULL;
	bool reset_ssp = false;

	_Static_assert(sizeof(SkuSiPolicyVersion) == SSPVER_SIZE,
			"SkuSiPolicyVersion has unexpected size");
	_Static_assert(sizeof(SkuSiPolicyUpdateSigners) == SSPSIG_SIZE,
			"SkuSiPolicyUpdateSigners has unexpected size");

	if (ssp_policy == POLICY_NOTREAD) {
		efi_status = get_variable_attr(SSP_POLICY, &policyp,
				       &policysize, SHIM_LOCK_GUID,
				       &attributes);
		if (!EFI_ERROR(efi_status)) {
			ssp_policy = *policyp;
			clear_ssp_policy();
		}
	}

	if (EFI_ERROR(efi_status)) {
		dprint("Default SSP policy: automatic\n");
		ssp_ver = ssp_ver_automatic;
		ssp_sig = ssp_sig_automatic;
	} else {
		switch (ssp_policy) {
			case POLICY_LATEST:
				dprint("Custom SSP policy: latest\n");\
				ssp_ver = ssp_ver_latest;
				ssp_sig = ssp_sig_latest;
				break;
			case POLICY_AUTOMATIC:
				dprint("Custom SSP policy: automatic\n");
				ssp_ver = ssp_ver_automatic;
				ssp_sig = ssp_sig_automatic;
				break;
			case POLICY_RESET:
				if (secure_mode()) {
					console_print(L"Cannot reset SSP policy: Secure Boot is enabled.\n");
					ssp_ver = ssp_ver_automatic;
					ssp_sig = ssp_sig_automatic;
				} else {
					dprint(L"Custom SSP policy: reset OK\n");
					reset_ssp = true;
				}
				break;
			default:
				console_error(L"SSP policy state %llu is invalid",
					      EFI_INVALID_PARAMETER);
				ssp_ver = ssp_ver_automatic;
				ssp_sig = ssp_sig_automatic;
				break;
		}
	}

	if (!ssp_ver && !ssp_sig && !reset_ssp) {
		dprint(L"No supplied SSP data, not setting variables\n");
		return EFI_SUCCESS;
	}

	efi_status = get_variable_attr(SSPVER_VAR_NAME, &sspver, &sspversize,
				       SECUREBOOT_EFI_NAMESPACE_GUID, &attributes);
	/*
	 * Since generally we want bootmgr to manage its own revocations,
	 * we are much less agressive trying to set those variables
	 */
	if (EFI_ERROR(efi_status)) {
		dprint(L"SkuSiPolicyVersion read failed %r\n", efi_status);
	} else if (preserve_ssp_uefi_variable(sspver, sspversize, attributes, ssp_ver)
		   && !reset_ssp) {
		FreePool(sspver);

		dprint(L"preserving %s variable it is %d bytes, attributes are 0x%08x\n",
		       SSPVER_VAR_NAME, sspversize, attributes);
		return EFI_SUCCESS;
	} else {
		FreePool(sspver);

		efi_status = clear_ssp_uefi_variables();
	}

	if (reset_ssp)
		return efi_status;

	/* set variable */
	efi_status = set_variable(SSPVER_VAR_NAME, SECUREBOOT_EFI_NAMESPACE_GUID,
			SSP_VAR_ATTRS, SSPVER_SIZE, ssp_ver);
	if (EFI_ERROR(efi_status)) {
		dprint(L"%s variable writing failed %r\n", SSPVER_VAR_NAME,
				efi_status);
		return efi_status;
	}
	dprint("done setting %s variable.\n", SSPSIG_VAR_NAME);

	efi_status = set_variable(SSPSIG_VAR_NAME, SECUREBOOT_EFI_NAMESPACE_GUID,
			SSP_VAR_ATTRS, SSPSIG_SIZE, ssp_sig);
	if (EFI_ERROR(efi_status)) {
		dprint(L"%s variable writing failed %r\n", SSPSIG_VAR_NAME,
				efi_status);
		return efi_status;
	}
	dprint("done setting %s variable.\n", SSPSIG_VAR_NAME);

	return efi_status;
}

EFI_STATUS
set_ssp_uefi_variable_internal(void)
{
	return set_ssp_uefi_variable(NULL, NULL, SkuSiPolicyVersion,
	                             SkuSiPolicyUpdateSigners);
}
// vim:fenc=utf-8:tw=75:noet
