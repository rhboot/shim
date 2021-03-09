// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * csv.c - CSV parser
 */

#include "shim.h"

void NONNULL(1, 3, 4)
parse_csv_line(CHAR8 * line, size_t max, size_t *n_columns, const CHAR8 *columns[])
{
	CHAR8 *next = line;
	size_t n = 0, new_n = n;
	const CHAR8 * const delims = (CHAR8 *)",";
	CHAR8 state = 0;
	CHAR8 *token = NULL;

	bool valid = true;
	for (n = 0; n < *n_columns; n++) {

		if (valid) {
			valid = strntoken(next, max, delims, &token, &state);
		}
		if (valid) {
			next += strlena(token) + 1;
			max -= strlena(token) + 1;
			columns[n] = token;
			new_n = n + 1;
		} else {
			columns[n] = NULL;
			continue;
		}
	}
	*n_columns = new_n;
}

void
free_csv_list(list_t *list)
{
	list_t *pos = NULL, *tmp = NULL;
	list_for_each_safe(pos, tmp, list) {
		struct csv_row *row;

		row = list_entry(pos, struct csv_row, list);
		list_del(&row->list);
		FreePool(row);
	}
}

EFI_STATUS
parse_csv_data(CHAR8 *data, CHAR8 *data_end, size_t n_columns, list_t *list)
{
	EFI_STATUS efi_status = EFI_OUT_OF_RESOURCES;
	CHAR8 delims[] = "\r\n";
	CHAR8 *line = data;
	size_t max = 0;
	CHAR8 *end = data_end;

	if (!data || !end || end <= data || !n_columns || !list)
		return EFI_INVALID_PARAMETER;

	max = (uintptr_t)end - (uintptr_t)line + (end > line ? 1 : 0);

	if (line && is_utf8_bom(line, max))
		line += UTF8_BOM_SIZE;

	while (line && line <= data_end) {
		size_t entrysz = sizeof(CHAR8 *) * n_columns + sizeof(struct csv_row);
		struct csv_row *entry;
		size_t m_columns = n_columns;
		CHAR8 *delim;
		bool found = true;

		end = data_end;
		max = (uintptr_t)end - (uintptr_t)line + (end > line ? 1 : 0);
		while (max && found) {
			found = false;
			for (delim = &delims[0]; max && *delim; delim++) {
				if (line[0] == *delim) {
					line++;
					max--;
					found = true;
				}
			}
		}
		for (delim = &delims[0]; *delim; delim++) {
			CHAR8 *tmp = strnchrnul(line, max, *delim);
			if (tmp < end)
				end = tmp;
		}
		max = (uintptr_t)end - (uintptr_t)line + (end > line ? 1 : 0);
		*end = '\0';

		if (line == data_end || max == 0) {
			line = end + 1;
			continue;
		}

		entry = AllocateZeroPool(entrysz);
		if (!entry) {
			efi_status = EFI_OUT_OF_RESOURCES;
			goto err_oom;
		}

		INIT_LIST_HEAD(&entry->list);
		list_add_tail(&entry->list, list);

		for (delim = &delims[0]; *delim; delim++) {
			CHAR8 *tmp = strnchrnul(line, max, *delim);
			if (tmp < end)
				end = tmp;
		}

		parse_csv_line(line, max, &m_columns, (const CHAR8 **)entry->columns);
		entry->n_columns = m_columns;
		line = end + 1;
	}

	return EFI_SUCCESS;
err_oom:
	free_csv_list(list);
	return efi_status;
}

// vim:fenc=utf-8:tw=75:noet
