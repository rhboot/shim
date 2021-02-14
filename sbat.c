// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * sbat.c - parse SBAT data from the .sbat section data
 */

#include "sbat.h"
#include <string.h>

CHAR8 *
get_sbat_field(CHAR8 *current, CHAR8 *end, const CHAR8 **field, char delim)
{
	CHAR8 *offset;

	if (!field || !current || !end || current >= end)
		return NULL;

	offset = strchrnula(current, delim);
	*field = current;

	if (!*offset)
		return NULL;

	*offset = '\0';
	return offset + 1;
}

EFI_STATUS
parse_sbat_entry(CHAR8 **current, CHAR8 *end, struct sbat_entry **sbat_entry)
{
	struct sbat_entry *entry = NULL;

	entry = AllocateZeroPool(sizeof(*entry));
	if (!entry)
		return EFI_OUT_OF_RESOURCES;

	*current = get_sbat_field(*current, end, &entry->component_name, ',');
	if (!entry->component_name)
		goto error;

	*current = get_sbat_field(*current, end, &entry->component_generation,
	                          ',');
	if (!entry->component_generation)
		goto error;

	*current = get_sbat_field(*current, end, &entry->vendor_name, ',');
	if (!entry->vendor_name)
		goto error;

	*current =
		get_sbat_field(*current, end, &entry->vendor_package_name, ',');
	if (!entry->vendor_package_name)
		goto error;

	*current = get_sbat_field(*current, end, &entry->vendor_version, ',');
	if (!entry->vendor_version)
		goto error;

	*current = get_sbat_field(*current, end, &entry->vendor_url, '\n');
	if (!entry->vendor_url)
		goto error;

	*sbat_entry = entry;

	return EFI_SUCCESS;

error:
	FreePool(entry);
	return EFI_INVALID_PARAMETER;
}

EFI_STATUS
parse_sbat(char *sbat_base, size_t sbat_size, char *buffer, struct sbat *sbat)
{
	CHAR8 *current = (CHAR8 *)sbat_base;
	CHAR8 *end = (CHAR8 *)sbat_base + sbat_size;
	EFI_STATUS efi_status = EFI_SUCCESS;
	struct sbat_entry *entry;
	struct sbat_entry **entries;
	unsigned int i;

	while ((*current == '\r' || *current == '\n') && current < end)
		current++;

	if (current == end)
		return EFI_INVALID_PARAMETER;

	while ((*end == '\r' || *end == '\n') && end < current)
		end--;

	*(end - 1) = '\0';

	do {
		entry = NULL;
		efi_status = parse_sbat_entry(&current, end, &entry);
		if (EFI_ERROR(efi_status))
			goto error;

		if (end < current) {
			efi_status = EFI_INVALID_PARAMETER;
			goto error;
		}

		if (entry) {
			entries = ReallocatePool(
				sbat->entries, sbat->size * sizeof(entry),
				(sbat->size + 1) * sizeof(entry));
			if (!entries) {
				efi_status = EFI_OUT_OF_RESOURCES;
				goto error;
			}

			sbat->entries = entries;
			sbat->entries[sbat->size] = entry;
			sbat->size++;
		}
	} while (entry && *current != '\0');

	return efi_status;
error:
	perror(L"Failed to parse SBAT data: %r\n", efi_status);
	for (i = 0; i < sbat->size; i++)
		FreePool(sbat->entries[i]);
	return efi_status;
}

EFI_STATUS
verify_sbat(struct sbat *sbat, struct sbat_var *sbat_var_root)
{
	unsigned int i;
	list_t *pos;

	for (i = 0; i < sbat->size; i++) {
		struct sbat_entry *entry = entry = sbat->entries[i];
		for_each_sbat_var (pos, &sbat_var_root->list) {
			struct sbat_var *sbat_var_entry =
				list_entry(pos, struct sbat_var, list);

			if (strcmp(entry->component_name,
			           sbat_var_entry->component_name) == 0) {
				dprint(L"component %a has a matching SBAT variable entry, verifying\n", entry->component_name);
				/* atoi returns zero for failed conversion, so essentially
				   badly parsed component_generation will be treated as zero 
				*/
				if (atoi(entry->component_generation) <
				    atoi(sbat_var_entry->component_generation)) {
					dprint(L"component's %a generation: %d. Conflicts with SBAT variable generation %d\n",
					       entry->component_name,
					       atoi(entry->component_generation),
					       atoi(sbat_var_entry->component_generation));
					LogError(L"image did not pass SBAT verification\n");
					return EFI_SECURITY_VIOLATION;
				}
			}
		}
	}
	dprint(L"all entries from SBAT section verified\n");
	return EFI_SUCCESS;
}

static BOOLEAN
is_utf8_bom(CHAR8 *buf)
{
	unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
	return !!CompareMem(buf, bom, MIN(sizeof(bom), sizeof(buf)));
}

static struct sbat_var *
new_entry(const CHAR8 *comp_gen, const CHAR8 *comp_name_size,
          const CHAR8 *comp_name)
{
	struct sbat_var *new_entry = AllocatePool(sizeof(*new_entry));

	INIT_LIST_HEAD(&new_entry->list);
	new_entry->component_generation = comp_gen;
	new_entry->component_name_size = comp_name_size;
	new_entry->component_name = comp_name;

	return new_entry;
}

static int
add_entry(list_t *list, const CHAR8 *comp_gen, const CHAR8 *comp_name_size,
          const CHAR8 *comp_name)
{
	struct sbat_var *new;

	new = new_entry(comp_gen, comp_name_size, comp_name);
	if (!new)
		return -1;

	list_add_tail(&new->list, list);
	return 0;
}

static void
clean_up_vars(list_t *entries)
{
	list_t *pos = NULL, *tmp = NULL;
	for_each_sbat_var_safe(pos, tmp, entries) {
		struct sbat_var *entry;

		entry = list_entry(pos, struct sbat_var, list);
		list_del(&entry->list);
		if (entry->component_generation)
			FreePool((CHAR8 *)entry->component_generation);
		if (entry->component_name_size)
			FreePool((CHAR8 *)entry->component_name_size);
		if (entry->component_name)
			FreePool((CHAR8 *)entry->component_name);
		FreePool(entry);
	}
}

int
parse_sbat_var(list_t *entries)
{
	UINT8 *data = 0;
	UINTN datasize, i;
	EFI_STATUS efi_status;
	int rc;

	INIT_LIST_HEAD(entries);
	efi_status = get_variable(L"SBAT", &data, &datasize, SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
		return -1;
	}

	CHAR8 *start = (CHAR8 *)data;
	CHAR8 *end = (CHAR8 *)data + datasize;
	while ((*end == '\r' || *end == '\n') && end < start)
		end--;
	*end = '\0';
	if (is_utf8_bom(start))
		start += 3;
	dprint(L"SBAT variable data:\n");

	rc = 0;
	char delim;
	while (start[0] != '\0') {
		const CHAR8 *fields[3] = { NULL, };

		for (i = 0; i < 4; i++) {
			const CHAR8 *tmp;
			if (i == 3 && start[0] != '\0') {
				if (delim == ',') {
					start = get_sbat_field(start, end, &tmp, '\n');
					break;
				}
				else
					break;
			}
			delim = ',';
			if ( (strchrnula(start,'\n') - start +1) <= (strchrnula(start,',') - start +1)) {
				delim = '\n';
			if (!start) {
				rc = -1;
				break;
			start = get_sbat_field(start, end, &tmp, delim);

			fields[i] = tmp;
			if (!fields[i]) {
				rc = -1;
				break;
			}
		}
		if (rc < 0)
			break;
		dprint(L"component %a with generation %a\n", fields[2], fields[0]);

		rc = add_entry(entries, fields[0], fields[1], fields[2]);
		if (rc < 0)
			break;
	}
	if (rc < 0)
		clean_up_vars(entries);

	FreePool(data);
	return rc;
}

// vim:fenc=utf-8:tw=75:noet
