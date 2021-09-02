// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * csv.c - CSV parser
 */

#include "shim.h"

void NONNULL(1, 3, 4)
parse_csv_line(char * line, size_t max, size_t *n_columns, const char *columns[])
{
	char *next = line;
	size_t n = 0, new_n = n;
	const char * const delims = ",";
	char state = 0;
	char *token = NULL;

	bool valid = true;

	for (n = 0; n < *n_columns; n++) {

		if (valid) {
			valid = strntoken(next, max, delims, &token, &state);
		}
		if (valid) {
			next += strlen(token) + 1;
			max -= strlen(token) + 1;
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
parse_csv_data(char *data, char *data_end, size_t n_columns, list_t *list)
{
	EFI_STATUS efi_status = EFI_OUT_OF_RESOURCES;
	char delims[] = "\r\n";
	char *line = data;
	size_t max = 0;
	char *end = data_end;

	if (!data || !end || end <= data || !n_columns || !list) {
		dprint(L"data:0x%lx end:0x%lx n_columns:%lu list:0x%lx\n",
		       data, end, n_columns, list);
		return EFI_INVALID_PARAMETER;
	}

	max = (uintptr_t)end - (uintptr_t)line + (end > line ? 1 : 0);
	if (is_utf8_bom(line, max))

		line += UTF8_BOM_SIZE;

	while (line <= data_end && *line) {
		size_t entrysz = sizeof(char *) * n_columns + sizeof(struct csv_row);
		struct csv_row *entry;
		size_t m_columns = n_columns;
		char *delim;
		bool found = true;
		bool eof = false;

		end = data_end;
		max = (uintptr_t)end - (uintptr_t)line + (end > line ? 1 : 0);
		/* Skip the delimiter(s) of the previous line */
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
		/* Find the first delimiter of the current line */
		for (delim = &delims[0]; *delim; delim++) {
			char *tmp = strnchrnul(line, max, *delim);
			if (tmp < end)
				end = tmp;
		}
		max = (uintptr_t)end - (uintptr_t)line + (end > line ? 1 : 0);

		if (!*end)
			eof = true;
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
			char *tmp = strnchrnul((const char *)line, max, *delim);
			if (tmp < end)
				end = tmp;
		}

		parse_csv_line(line, max, &m_columns, (const char **)entry->columns);
		entry->n_columns = m_columns;
		if (eof)
			break;

		line = end + 1;
	}

	return EFI_SUCCESS;
err_oom:
	free_csv_list(list);
	return efi_status;
}

// vim:fenc=utf-8:tw=75:noet
