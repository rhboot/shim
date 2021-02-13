// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * sbat.c - parse SBAT data from the .sbat section data
 */

#include "sbat.h"
#include <string.h>

CHAR8 *
get_sbat_field(CHAR8 *current, CHAR8 *end, const CHAR8 ** field, char delim)
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

EFI_STATUS parse_sbat_entry(CHAR8 **current, CHAR8 *end,
			    struct sbat_entry **sbat_entry)
{
	struct sbat_entry *entry = NULL;

	entry = AllocateZeroPool(sizeof(*entry));
	if (!entry)
		return EFI_OUT_OF_RESOURCES;

	*current = get_sbat_field(*current, end, &entry->component_name, ',');
	if (!entry->component_name)
		goto error;

	*current = get_sbat_field(*current, end, &entry->component_generation,',');
	if (!entry->component_generation)
		goto error;

	*current = get_sbat_field(*current, end, &entry->vendor_name,',');
	if (!entry->vendor_name)
		goto error;

	*current = get_sbat_field(*current, end, &entry->vendor_package_name, ',');
	if (!entry->vendor_package_name)
		goto error;

	*current = get_sbat_field(*current, end, &entry->vendor_version,',');
	if (!entry->vendor_version)
		goto error;

	*current = get_sbat_field(*current, end, &entry->vendor_url,'\n');
	if (!entry->vendor_url)
		goto error;

	*sbat_entry = entry;

	return EFI_SUCCESS;

error:
	FreePool(entry);
	return EFI_INVALID_PARAMETER;
}

EFI_STATUS parse_sbat(char *sbat_base, size_t sbat_size, char *buffer,
		      struct sbat *sbat)
{
	CHAR8 *current = (CHAR8 *) sbat_base;
	CHAR8 *end = (CHAR8 *) sbat_base + sbat_size;
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
			entries = ReallocatePool(sbat->entries,
						 sbat->size * sizeof(entry),
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

EFI_STATUS verify_sbat (struct sbat *sbat, struct sbat_var *sbat_var_root)
{
	unsigned int i;
	struct sbat_entry *entry = NULL;
	for (i = 0; i < sbat->size; i++) {
		entry = sbat->entries[i];
		struct sbat_var *sbat_var_entry = sbat_var_root;
		while (sbat_var_entry != NULL) {
			if (strcmp(entry->component_name,sbat_var_entry->component_name) == 0) {
				dprint(L"component %a has a matching SBAT variable entry, verifying\n", entry->component_name);
				/* atoi returns zero for failed conversion, so essentially
				   badly parsed component_generation will be treated as zero 
				*/
				if (atoi(entry->component_generation) < atoi(sbat_var_entry->component_generation)) {
					dprint(L"component's %a generation: %d. Conflicts with SBAT variable generation %d\n",
					       entry->component_name,
					       atoi(entry->component_generation),
					       atoi(sbat_var_entry->component_generation));
					LogError(L"image did not pass SBAT verification\n");
					return EFI_SECURITY_VIOLATION;
				}
			}
			sbat_var_entry = sbat_var_entry->next;
		}
	}
		dprint(L"all entries from SBAT section verified\n");
		return EFI_SUCCESS;
}

static BOOLEAN is_utf8_bom(CHAR8 *buf)
{
	unsigned char bom[] = { 0xEF,0xBB,0xBF };
	return !!CompareMem(buf, bom, MIN(sizeof(bom), sizeof(buf)));
}

struct sbat_var* new_entry(const CHAR8 *comp_gen, const CHAR8 *comp_name_size,
			   const CHAR8 *comp_name)
{
	struct sbat_var *new_entry = AllocatePool(sizeof(*new_entry));
	new_entry->next = NULL;
	new_entry->component_generation = comp_gen;
	new_entry->component_name_size = comp_name_size;
	new_entry->component_name = comp_name;
	return new_entry;
}

struct sbat_var* add_entry(struct sbat_var *n, const CHAR8 *comp_gen, const CHAR8 *comp_name_size,
			   const CHAR8 *comp_name)
{
	if ( n == NULL )
		return NULL;
	while (n->next)
		n = n->next;
	return (n->next = new_entry(comp_gen, comp_name_size, comp_name));
}

struct sbat_var* parse_sbat_var()
{
	UINT8 *data = 0;
	UINTN datasize;
	EFI_STATUS efi_status;

	efi_status = get_variable(L"SBAT", &data, &datasize, SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
		return NULL;
	}

	struct sbat_var *root = new_entry((CHAR8 *)"0",(CHAR8 *)"0",(CHAR8 *)"entries");
	struct sbat_var *nodename = root;
	CHAR8 *start = (CHAR8 *) data;
	CHAR8 *end = (CHAR8 *) data + datasize;
	while ((*end == '\r' || *end == '\n') && end < start)
		end--;
	*end = '\0';
	if (is_utf8_bom(start))
		start += 3;
	dprint(L"SBAT variable data:\n");
	while (start[0] != '\0') {
		const CHAR8 *comp_name_size, *comp_gen, *comp_name;

		start = get_sbat_field(start, end, &comp_gen, ',');

		start = get_sbat_field(start, end, &comp_name_size, ',');

		start = get_sbat_field(start, end, &comp_name, '\n');

		dprint(L"component %a with generation %a\n",comp_name, comp_gen);

		add_entry(nodename, comp_gen, comp_name_size, comp_name);
		nodename = nodename->next;
	}
	return root;
}

// vim:fenc=utf-8:tw=75:noet
