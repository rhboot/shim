// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * ctype.h - standard ctype functions
 */
#ifdef SHIM_UNIT_TEST
#include_next <ctype.h>
#else
#ifndef _CTYPE_H
#define _CTYPE_H

#define isprint(c) ((c) >= 0x20 && (c) <= 0x7e)

/* Determines if a particular character is a decimal-digit character */
static inline __attribute__((__unused__)) int
isdigit(int c)
{
	//
	// <digit> ::= [0-9]
	//
	return (('0' <= (c)) && ((c) <= '9'));
}

/* Determine if an integer represents character that is a hex digit */
static inline __attribute__((__unused__)) int
isxdigit(int c)
{
	//
	// <hexdigit> ::= [0-9] | [a-f] | [A-F]
	//
	return ((('0' <= (c)) && ((c) <= '9')) ||
	        (('a' <= (c)) && ((c) <= 'f')) ||
	        (('A' <= (c)) && ((c) <= 'F')));
}

/* Determines if a particular character represents a space character */
static inline __attribute__((__unused__)) int
isspace(int c)
{
	//
	// <space> ::= [ ]
	//
	return ((c) == ' ');
}

/* Determine if a particular character is an alphanumeric character */
static inline __attribute__((__unused__)) int
isalnum(int c)
{
	//
	// <alnum> ::= [0-9] | [a-z] | [A-Z]
	//
	return ((('0' <= (c)) && ((c) <= '9')) ||
	        (('a' <= (c)) && ((c) <= 'z')) ||
	        (('A' <= (c)) && ((c) <= 'Z')));
}

/* Determines if a particular character is in upper case */
static inline __attribute__((__unused__)) int
isupper(int c)
{
	//
	// <uppercase letter> := [A-Z]
	//
	return (('A' <= (c)) && ((c) <= 'Z'));
}

/* Convert character to lowercase */
static inline __attribute__((__unused__)) int
tolower(int c)
{
	if (('A' <= (c)) && ((c) <= 'Z')) {
		return (c - ('A' - 'a'));
	}
	return (c);
}

static inline __attribute__((__unused__)) int
toupper(int c)
{
	return ((c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c);
}

#endif /* !_CTYPE_H */
#endif /* !SHIM_UNIT_TEST */
// vim:fenc=utf-8:tw=75:noet
