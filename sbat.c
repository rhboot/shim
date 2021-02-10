// SPDX-License-Identifier: BSD/Tiano-XXX-FIXME
/*
 * sbat.c - parse SBAT data from the .sbat section data
 */

#include "sbat.h"

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

// vim:fenc=utf-8:tw=75:noet
