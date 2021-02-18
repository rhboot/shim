// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * sbat.c - parse SBAT data from the .sbat section data
 */

#include "shim.h"

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
parse_sbat(char *sbat_base, size_t sbat_size, size_t *sbats, struct sbat_entry ***sbat)
{
	CHAR8 *current = (CHAR8 *) sbat_base;
	CHAR8 *end = (CHAR8 *) sbat_base + sbat_size - 1;
	EFI_STATUS efi_status = EFI_SUCCESS;
	struct sbat_entry *entry;
	struct sbat_entry **entries = NULL;
	unsigned int i = 0;

	if (!sbat_base || sbat_size == 0 || !sbats || !sbat)
		return EFI_INVALID_PARAMETER;

	if (current == end)
		return EFI_INVALID_PARAMETER;

	*sbats = 0;
	*sbat = 0;

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
				entries, i * sizeof(entry),
				(i + 1) * sizeof(entry));
			if (!entries) {
				efi_status = EFI_OUT_OF_RESOURCES;
				goto error;
			}

			entries[i++] = entry;
		}
	} while (entry && *current != '\0');

	*sbats = i;
	*sbat = entries;

	return efi_status;
error:
	perror(L"Failed to parse SBAT data: %r\n", efi_status);
	while (i)
		FreePool(entries[i--]);
	return efi_status;
}

// vim:fenc=utf-8:tw=75:noet
