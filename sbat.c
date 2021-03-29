// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * sbat.c - parse SBAT data from the .sbat section data
 */

#include "shim.h"

EFI_STATUS
parse_sbat_section(char *section_base, size_t section_size,
		   size_t *n_entries,
		   struct sbat_section_entry ***entriesp)
{
	struct sbat_section_entry *entry = NULL, **entries;
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
verify_single_entry(struct sbat_section_entry *entry, struct sbat_var_entry *sbat_var_entry)
{
	UINT16 sbat_gen, sbat_var_gen;

	if (strcmp((const char *)entry->component_name, (const char *)sbat_var_entry->component_name) == 0) {
		dprint(L"component %a has a matching SBAT variable entry, verifying\n",
			entry->component_name);

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
			sbat_var_entry = list_entry(pos, struct sbat_var_entry, list);
			efi_status = verify_single_entry(entries[i], sbat_var_entry);
			if (EFI_ERROR(efi_status))
				goto out;
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
parse_sbat_var(list_t *entries)
{
	UINT8 *data = 0;
	UINTN datasize;
	EFI_STATUS efi_status;

	if (!entries) {
		dprint(L"entries is NULL\n");
		return EFI_INVALID_PARAMETER;
	}

	efi_status = get_variable(SBAT_VAR_NAME, &data, &datasize, SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
		LogError(L"Failed to read SBAT variable\n", efi_status);
		return efi_status;
	}

	/*
	 * We've intentionally made sure there's a NUL byte on all variable
	 * allocations, so use that here.
	 */
	return parse_sbat_var_data(entries, data, datasize+1);
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

bool
preserve_sbat_uefi_variable(UINT8 *sbat, UINTN sbatsize, UINT32 attributes)
{
	return check_sbat_var_attributes(attributes) &&
	       sbatsize >= strlen(SBAT_VAR_SIG "1") &&
	       !strncmp((const char *)sbat, SBAT_VAR_SIG, strlen(SBAT_VAR_SIG));
}

EFI_STATUS
set_sbat_uefi_variable(void)
{
	EFI_STATUS efi_status = EFI_SUCCESS;
	UINT32 attributes = 0;

	UINT8 *sbat = NULL;
	UINTN sbatsize = 0;

	efi_status = get_variable_attr(SBAT_VAR_NAME, &sbat, &sbatsize,
				       SHIM_LOCK_GUID, &attributes);
	/*
	 * Always set the SbatLevel UEFI variable if it fails to read.
	 *
	 * Don't try to set the SbatLevel UEFI variable if attributes match
	 * and the signature matches.
	 */
	if (EFI_ERROR(efi_status)) {
		dprint(L"SBAT read failed %r\n", efi_status);
	} else if (preserve_sbat_uefi_variable(sbat, sbatsize, attributes)) {
		dprint(L"%s variable is %d bytes, attributes are 0x%08x\n",
		       SBAT_VAR_NAME, sbatsize, attributes);
		FreePool(sbat);
		return EFI_SUCCESS;
	} else {
		FreePool(sbat);

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
	                          sizeof(SBAT_VAR)-1, SBAT_VAR);
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

	if (sbatsize != strlen(SBAT_VAR) ||
	    strncmp((const char *)sbat, SBAT_VAR, strlen(SBAT_VAR)) != 0) {
		dprint("new sbatsize is %d, expected %d\n", sbatsize,
		       strlen(SBAT_VAR));
		efi_status = EFI_INVALID_PARAMETER;
	} else {
		dprint(L"%s variable initialization succeeded\n", SBAT_VAR_NAME);
	}

	FreePool(sbat);

	return efi_status;
}

// vim:fenc=utf-8:tw=75:noet
