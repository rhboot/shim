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

EFI_STATUS
verify_single_entry(struct sbat_entry *entry, struct sbat_var *sbat_var_entry)
{
        UINT16 sbat_gen, sbat_var_gen;

        if (strcmp(entry->component_name, sbat_var_entry->component_name) == 0) {
                dprint(L"component %a has a matching SBAT variable entry, verifying\n",
                       entry->component_name);
                /* atoi returns zero for failed conversion, so essentially
                badly parsed component_generation will be treated as zero
                */
                sbat_gen = atoi(entry->component_generation);
                sbat_var_gen = atoi(sbat_var_entry->component_generation);
                if ( sbat_gen < sbat_var_gen ) {
                        dprint(L"component %a, generation %d, was revoked by SBAT variable",
                                 entry->component_name, sbat_gen);
                        LogError(L"image did not pass SBAT verification\n");
                        return EFI_SECURITY_VIOLATION;
                }
        }
        return EFI_SUCCESS;
}

static void
clean_up_vars(list_t *entries)
{
        list_t *pos = NULL, *tmp = NULL;
        for_each_sbat_var_safe(pos, tmp, entries) {
                struct sbat_var *entry = list_entry(pos, struct sbat_var, list);
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

EFI_STATUS
verify_sbat(struct sbat *sbat, list_t *sbat_entries)
{
	unsigned int i;
	list_t *pos = NULL;
	EFI_STATUS efi_status;
	struct sbat_entry *entry;
	struct sbat_var *sbat_var_entry;
	
	for (i = 0; i < sbat->size; i++) {
		entry = sbat->entries[i];
		for_each_sbat_var(pos, sbat_entries) {
			sbat_var_entry = list_entry(pos, struct sbat_var, list);
			efi_status = verify_single_entry(entry, sbat_var_entry);
			if (EFI_ERROR(efi_status))
				return efi_status;
		}
	}
	dprint(L"all entries from SBAT section verified\n");
	clean_up_vars(sbat_entries);
	return efi_status;
}

static BOOLEAN
is_utf8_bom(CHAR8 *buf, size_t bufsize)
{
	unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
	return CompareMem(buf, bom, MIN(sizeof(bom), bufsize)) == 0;
}

static struct sbat_var *
new_entry(const CHAR8 *comp_gen, const CHAR8 *comp_name_size,
          const CHAR8 *comp_name)
{
	struct sbat_var *new_entry = AllocatePool(sizeof(*new_entry));
	if (!new_entry)
		return NULL;
	INIT_LIST_HEAD(&new_entry->list); 
	new_entry->component_generation = comp_gen;
	new_entry->component_name_size = comp_name_size;
	new_entry->component_name = comp_name;
	return new_entry;
}

EFI_STATUS
add_entry(list_t *list, const CHAR8 *comp_gen, const CHAR8 *comp_name_size,
          const CHAR8 *comp_name)
{
	struct sbat_var *new;
	new = new_entry(comp_gen, comp_name_size, comp_name);
	if (!new)
		return EFI_OUT_OF_RESOURCES;

	list_add_tail(&new->list, list);
	return EFI_SUCCESS;
}

EFI_STATUS
parse_sbat_var(list_t *entries)
{
	UINT8 *data = 0;
	UINTN datasize, i;
	EFI_STATUS efi_status;

	INIT_LIST_HEAD(entries); 
	efi_status = get_variable(L"SBAT", &data, &datasize, SHIM_LOCK_GUID);
	if (EFI_ERROR(efi_status)) {
			console_error(L"Failed to read SBAT variable\n",
				      efi_status);
			return efi_status;
	}

	CHAR8 *start = (CHAR8 *)data;
	CHAR8 *end = (CHAR8 *)data + datasize;
	if (is_utf8_bom(start, datasize))
		start += 3;
	while ((*end == '\r' || *end == '\n') && end < start)
		end--;
	*end = '\0';

	dprint(L"SBAT variable data:\n");

	char delim;
	while (start) {
		const CHAR8 *fields[3] = { NULL, };
		for (i = 0; i < 4; i++) {
			const CHAR8 *tmp;
			/* on fourth iteration we check if we had extra stuff on line while parsing
			   component_name. If delimeter on 3rd iteration was ',', this means that
			   we have comments after component_name. get_sbat_field in this if condition
			   parses comments, if they are present and drops them.
			*/
			if (i == 3 && start[0] != '\0') {
				if (delim == ',')
					start = get_sbat_field(start, end, &tmp, '\n');
				break;
			}
			delim = ',';
			/* we do not want to jump to next line and grab stuff from that
			*/
			if ( (strchrnula(start,'\n') - start + 1) <= (strchrnula(start,',') - start + 1)) {
				delim = '\n';
			}
			if (!start) {
				goto error;
			}
			start = get_sbat_field(start, end, &tmp, delim);
			/*   to be replaced when we have strdupa()
			*/
			fields[i] = strndupa(tmp, strlen(tmp));
			if (!fields[i]) {
				goto error;
			}
		}
		dprint(L"component %a with generation %a\n", fields[2], fields[0]);
		efi_status = add_entry(entries, fields[0], fields[1], fields[2]);
		if (EFI_ERROR(efi_status))
			goto error;
	}
	FreePool(data);
	return EFI_SUCCESS;
error:
	perror(L"failed to parse SBAT variable\n");
	clean_up_vars(entries);
	return EFI_INVALID_PARAMETER;
}

// vim:fenc=utf-8:tw=75:noet
