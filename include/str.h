// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SHIM_STR_H
#define SHIM_STR_H

#if __GNUC__ > 6
#ifdef SHIM_UNIT_TEST
#pragma GCC diagnostic error "-Wnonnull-compare"
#else
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#endif
#endif

static inline UNUSED RETURNS_NONNULL NONNULL(1)
char *
strnchrnul(const char *s, size_t max, int c)
{
	unsigned int i;

	if (!s || !max)
		return (char *)s;

	for (i = 0; i < max && s[i] != '\0' && s[i] != c; i++)
		;

	if (i == max)
		i--;

	return (char *)&s[i];
}

/**
 * strntoken: tokenize a string, with a limit
 * str: your string (will be modified)
 * max: maximum number of bytes to ever touch
 * delims: string of one character delimeters, any of which will tokenize
 * *token: the token we're passing back (must be a pointer to NULL initially)
 * state: a pointer to one char of state for between calls
 *
 * Ensure that both token and state are preserved across calls.  Do:
 *   char state = 0;
 *   char *token = NULL;
 *   for (...) {
 *     valid = strntoken(...)
 * not:
 *   char state = 0;
 *   for (...) {
 *     char *token = NULL;
 *     valid = strntoken(...)
 *
 * - it will not test bytes beyond str[max-1]
 * - it will not set *token to an address beyond &str[max-1]
 * - it will set *token to &str[max-1] without testing &str[max-2] for
 *   &str[max-1] == str
 * - sequences of multiple delimeters will result in empty (pointer to '\0')
 *   tokens.
 * - it expects you to update str and max on successive calls.
 *
 * return:
 * true means it hasn't tested str[max-1] yet and token is valid
 * false means it got to a NUL or str[max-1] and token is invalid
 */
static inline UNUSED NONNULL(1, 3, 4) int
strntoken(char *str, size_t max, const char *delims, char **token, char *state)
{
	char *tokend;
	const char *delim;
	int isdelim = 0;
	int state_is_delim = 0;

	if (!str || !max || !delims || !token || !state)
		return 0;

	tokend = &str[max-1];

	/*
	 * the very special case of "" with max=1, where we have no prior
	 * state to let us know this is the same as right after a delim
	 */
	if (*token == NULL && max == 1 && *str == '\0') {
		state_is_delim = 1;
	}

	for (delim = delims; *delim; delim++) {
		char *tmp = NULL;
		if (*token && *delim == *state)
			state_is_delim = 1;
		tmp = strnchrnul(str, max, *delim);
		if (tmp < tokend)
			tokend = tmp;
		if (*tokend == *delim)
			isdelim = 1;
	}
	*token = str;
	if (isdelim) {
		*state = *tokend;
		*tokend = '\0';
		return 1;
	}
	return state_is_delim;
}

#define UTF8_BOM { 0xef, 0xbb, 0xbf }
#define UTF8_BOM_SIZE 3

static inline UNUSED NONNULL(1) BOOLEAN
is_utf8_bom(CHAR8 *buf, size_t bufsize)
{
	unsigned char bom[] = UTF8_BOM;

	return CompareMem(buf, bom, MIN(UTF8_BOM_SIZE, bufsize)) == 0;
}

/**
 * parse CSV data from data to end.
 * *data	points to the first byte of the data
 * end		points to a NUL byte at the end of the data
 * n_columns	number of columns per entry
 * list		the list head we're adding to
 *
 * On success, list will be populated with individually allocate a list of
 * struct csv_list objects, with one column per entry of the "columns" array,
 * filled left to right with up to n_columns elements, or NULL when a csv line
 * does not have enough elements.
 *
 * Note that the data will be modified; all comma, linefeed, and newline
 * characters will be set to '\000'.  Additionally, consecutive linefeed and
 * newline characters will not result in rows in the results.
 *
 * On failure, list will be empty and all entries on it will have been freed,
 * using free_csv_list(), whether they were there before calling
 * parse_csv_data or not.
 */

struct csv_row {
	list_t list;		/* this is a linked list */
	size_t n_columns;	/* this is how many columns are actually populated */
	char *columns[0];	/* these are pointers to columns */
};

EFI_STATUS parse_csv_data(char *data, char *end, size_t n_columns,
                          list_t *list);
void free_csv_list(list_t *list);

#ifdef SHIM_UNIT_TEST
void NONNULL(1, 3, 4)
parse_csv_line(char * line, size_t max, size_t *n_columns, const char *columns[]);
#endif

#endif /* SHIM_STR_H */
