/*
 * Copyright 2018 Ruslan Nikolaev <nruslandev@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*----------------------------------------------------------------------------

  This implementation is based on the public domain implementation
  of printf available at http://my.execpc.com/~geezer/code/printf.c
  The code has been heavily modified & expanded to suit 'shim' purposes,
  and the 64-bit BCD conversion function has been added.

  ----------------------------------------------------------------------------

  ORIGINAL NOTICE:

  Stripped-down printf()
  Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer

  I, the copyright holder of this work, hereby release it into the
  public domain. This applies worldwide. If this is not legally possible:
  I grant any entity the right to use this work for any purpose,
  without any conditions, unless such conditions are required by law.

  xxx - current implementation of printf("%n" ...) is wrong -- RTFM

  16 Feb 2014:
  - test for NULL buffer moved from sprintf() to vsprintf()

  3 Dec 2013:
  - do_printf() restructured to get rid of confusing goto statements
  - do_printf() now returns EOF if an error occurs
    (currently, this happens only if out-of-memory in vasprintf_help())
  - added vasprintf() and asprintf()
  - added support for %Fs (far pointer to string)
  - compile-time option (PRINTF_UCP) to display pointer values
    as upper-case hex (A-F), instead of lower-case (a-f)
  - the code to check for "%--6d", "%---6d", etc. has been removed;
    these are now treated the same as "%-6d"

  3 Feb 2008:
  - sprintf() now works with NULL buffer; returns size of output
  - changed va_start() macro to make it compatible with ANSI C

  12 Dec 2003:
  - fixed vsprintf() and sprintf() in test code

  28 Jan 2002:
  - changes to make characters 0x80-0xFF display properly

  10 June 2001:
  - changes to make vsprintf() terminate string with '\0'

  12 May 2000:
  - math in DO_NUM (currently num2asc()) is now unsigned, as it should be
  - %0 flag (pad left with zeroes) now works
  - actually did some TESTING, maybe fixed some other bugs

  ----------------------------------------------------------------------------

  %[flag][width][.prec][mod][conv]
  flag:	-	left justify, pad right w/ blanks		DONE
	0	pad left w/ 0 for numerics			DONE
	+	always print sign, + or -			no
	' '	(blank)						no
	#	(???)						no

  width:		(field width)				DONE

  prec:		(precision)					no

  conv:	f,e,g,E,G float						no
	d,i	decimal int					DONE
	u	decimal unsigned				DONE
	o	octal						DONE
	x,X	hex						DONE
	r	EFI_STATUS					DONE
	g	EFI_GUID					DONE
	c	CHAR16						DONE
	a	CHAR8 string					DONE
	s	CHAR16 string					DONE
	p	ptr						DONE

  mod:	h	short int					DONE
	hh	char						DONE
	l	long int					DONE
	ll	long long int					DONE
	z,t	INTN						DONE

  To do:
  - implement '+' flag
  - implement ' ' flag

----------------------------------------------------------------------------*/

#include <efi.h>
#include <efilib.h>

/* display pointers in upper-case hex (A-F) instead of lower-case (a-f) */
#define	PRINTF_UCP	1

/* flags used in processing format string */
#define	PR_POINTER	0x001	/* 0x prefix for pointers */
#define	PR_NEGATIVE	0x002	/* PR_DO_SIGN set and num was < 0 */
#define	PR_LEFTJUST	0x004	/* left justify */
#define	PR_PADLEFT0	0x008	/* pad left with '0' instead of ' ' */
#define	PR_DO_SIGN	0x010	/* signed numeric conversion (%d vs. %u) */
#define	PR_8		0x020	/* 8 bit numeric conversion */
#define	PR_16		0x040	/* 16 bit numeric conversion */
#define	PR_64		0x080	/* 64 bit numeric conversion */
#define	PR_ASCII	0x100	/* ASCII string */

#define PR_BUFLEN	48		/* needs to fit GUID */

static CHAR16 *write_uintn_base10(CHAR16 *where, UINTN num)
{
	do {
		UINTN temp = num % 10;
		*--where = temp + '0';
		num = num / 10;
	} while (num != 0);
	return where;
}

/* A specialized function for 32-bit CPUs, so that we avoid 64-bit
   division and linking against libgcc. Better to use MAX_UINTN but
   it's poorly defined, so using MAX_ADDRESS instead. */

#if MAX_ADDRESS == 0xFFFFFFFFU

/* The idea is to use 64-bit BCD conversion as described in
   http://www.divms.uiowa.edu/~jones/bcd/decimal.html */

static CHAR16 *write_uint64_base10(CHAR16 *where, UINT64 num)
{
	CHAR16 *end;
	UINT32 d3, d2, d1, d0, q;

	if (num == 0) {
		*--where = '0';
		return where;
	}

	d0 = (UINT16) num;
	d1 = (UINT16) (num >> 16);
	d2 = (UINT16) (num >> 32);
	d3 = (UINT16) (num >> 48);

	d0 = 656 * d3 + 7296 * d2 + 5536 * d1 + d0;
	q = d0 / 10000;
	d0 = d0 % 10000;

	end = where - 4;
	where = write_uintn_base10(where, d0);
	while (where != end) *--where = '0';

	d1 = q + 7671 * d3 + 9496 * d2 + 6 * d1;
	q = d1 / 10000;
	d1 = d1 % 10000;

	end = where - 4;
	where = write_uintn_base10(where, d1);
	while (where != end) *--where = '0';

	d2 = q + 4749 * d3 + 42 * d2;
	q = d2 / 10000;
	d2 = d2 % 10000;

	end = where - 4;
	where = write_uintn_base10(where, d2);
	while (where != end) *--where = '0';

	d3 = q + 281 * d3;
	q = d3 / 10000;
	d3 = d3 % 10000;

	end = where - 4;
	where = write_uintn_base10(where, d3);
	while (where != end) *--where = '0';
	where = write_uintn_base10(where, q);
	while (*where == '0') where++;
	return where;
}

#endif

static CHAR8 HexDigits[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f'
};

typedef void (*fnptr_t) (CHAR16, void *);

/*****************************************************************************
  name:	do_printf
  action:	minimal subfunction for ?printf, calls function
	'fn' with arg 'ptr' for each character to be output
  returns:total number of characters output
*****************************************************************************/
static UINTN do_vprintf(CONST CHAR16 *fmt, fnptr_t fn, void *ptr, VA_LIST args)
{
	CHAR16 *where, buf[PR_BUFLEN];
	CONST CHAR8 *digits;
	UINTN count, actual_wd, given_wd;
	unsigned int state, flags, shift;
	UINTN num;
#if MAX_ADDRESS == 0xFFFFFFFFU
	UINT64 num64;
#endif

	count = given_wd = 0;
	state = flags = 0;
/* for() and switch() on the same line looks jarring
but the indentation gets out of hand otherwise */
	for (; *fmt; fmt++) switch(state)
	{
/* STATE 0: AWAITING '%' */
	case 0:
/* echo text until '%' seen */
		if (*fmt != '%')
		{
			fn(*fmt, ptr);
			count++;
			break;
		}
/* found %, get next char and advance state to check if next char is a flag */
		fmt++;
		state++;
		flags = 0;
		/* FALL THROUGH */
/* STATE 1: AWAITING FLAGS ('%' or '-' or '0') */
	case 1:
		if (*fmt == '%')	/* %% */
		{
			fn(*fmt, ptr);
			count++;
			state = 0;
			break;
		}
		if (*fmt == '-')
		{
			flags |= PR_LEFTJUST;
			break;
		}
		if (*fmt == '0')
		{
			flags |= PR_PADLEFT0;
/* '0' could be flag or field width -- fall through */
			fmt++;
		}
/* '0' or not a flag char: advance state to check if it's field width */
		state++;
		given_wd = 0;
		/* FALL THROUGH */
/* STATE 2: AWAITING (NUMERIC) FIELD WIDTH */
	case 2:
		if (*fmt >= '0' && *fmt <= '9')
		{
			given_wd = 10 * given_wd + (*fmt - '0');
			break;
		}
/* not field width: advance state to check if it's a modifier */
		state++;
		/* FALL THROUGH */
/* STATE 3: AWAITING MODIFIER CHARACTERS */
	case 3:
		if (*fmt == 'z' || *fmt == 't') {
#if MAX_ADDRESS > 0xFFFFFFFFU
			flags |= PR_64;
#endif
			break;
		}
		if (*fmt == 'l') {
			if(*(fmt + 1) == 'l') {
				fmt++;
				flags |= PR_64;
			} else if (sizeof(long) == 8) {
				flags |= PR_64;
			}
			break;
		}
		if (*fmt == 'h') {
			if (*(fmt + 1) == 'h') {
				fmt++;
				flags |= PR_8;
			} else {
				flags |= PR_16;
			}
			break;
		}
/* not a modifier: advance state to check if it's a conversion char */
		state++;
		/* FALL THROUGH */
/* STATE 4: AWAITING CONVERSION CHARACTER */
	case 4:
		where = &buf[PR_BUFLEN - 1];
		*where = '\0';
		digits = HexDigits;
/* pointer and numeric conversions */
		switch(*fmt) {
		case 'p':
#ifndef PRINT_UCP
			digits = HexDigits + 16;
#endif
			flags &= ~(PR_DO_SIGN | PR_NEGATIVE | PR_64 | PR_16 | PR_8);
#if MAX_ADDRESS == 0xFFFFFFFFU
			flags |= PR_64;
#endif
			shift = 4; /* display pointers in hex */
			num = (UINTN) VA_ARG(args, void *);
			if (!num) {
				flags &= ~PR_PADLEFT0;
				where = L"(nil)";
				break;
			}
			flags |= PR_POINTER;
			goto DO_NUM_OUT;
		case 'x':
			digits = HexDigits + 16;
			/* FALL THROUGH */
		case 'X':
			flags &= ~PR_DO_SIGN;
			shift = 4; /* display pointers in hex */
			goto DO_NUM;
		case 'd':
		case 'i':
			flags |= PR_DO_SIGN;
			shift = 0;
			goto DO_NUM;
		case 'u':
			flags &= ~PR_DO_SIGN;
			shift = 0;
			goto DO_NUM;

		case 'o':
			flags &= ~PR_DO_SIGN;
			shift = 3;
DO_NUM:
			if (flags & PR_DO_SIGN) {
				INTN snum;

				if (flags & PR_64) {
#if MAX_ADDRESS == 0xFFFFFFFFU
					INT64 snum64 = VA_ARG(args, INT64);
					if (snum64 < 0) {
						flags |= PR_NEGATIVE;
						snum64 = -snum64;
					}
					num64 = snum64;
					goto DO_NUM_64;
#else
					snum = VA_ARG(args, INT64);
#endif
				} else {
					snum = VA_ARG(args, int);
					if (flags & (PR_16 | PR_8))
						snum = (flags & PR_16) ? (INT16) snum : (INT8) snum;
				}
				if (snum < 0) {
					flags |= PR_NEGATIVE;
					snum = -snum;
				}
				num = snum;
			} else {
				if (flags & PR_64) {
#if MAX_ADDRESS == 0xFFFFFFFFU
					num64 = VA_ARG(args, UINT64);
DO_NUM_64:
					if (!shift) {
						where = write_uint64_base10(where, num64);
					} else {
						UINTN mask = ((UINTN) 1U << shift) - 1;
						do {
							UINTN temp = num64 & mask;
							*--where = digits[temp];
							num64 = num64 >> shift;
						} while (num64 != 0);
					}
					break;
#else
					num = VA_ARG(args, UINT64);
#endif
				} else {
					num = VA_ARG(args, unsigned int);
					if (flags & (PR_16 | PR_8))
						num = (flags & PR_16) ? (UINT16) num : (UINT8) num;
				}
			}

DO_NUM_OUT:
			/* Convert binary to octal/decimal/hex ASCII;
			   the math here is _always_ unsigned */
			if (!shift) {
				where = write_uintn_base10(where, num);
			} else {
				UINTN mask = (1U << shift) - 1;
				do {
					UINTN temp = num & mask;
					*--where = digits[temp];
					num = num >> shift;
				} while (num != 0);
			}
			break;

		case 'c':
/* disallow these modifiers for %c */
			flags &= ~(PR_DO_SIGN | PR_NEGATIVE | PR_PADLEFT0);
/* yes; we're converting a character to a string here: */
			where--;
			*where = (CHAR16) VA_ARG(args, int);
			break;
		case 'r':
/* EFI errors */
			flags &= ~(PR_DO_SIGN | PR_NEGATIVE | PR_PADLEFT0);
			where = (CHAR16 *) StatusToString(buf, VA_ARG(args, EFI_STATUS));
			break;
		case 'a':
			flags |= PR_ASCII;
/* fall through */
		case 's':
/* disallow these modifiers for %s */
			flags &= ~(PR_DO_SIGN | PR_NEGATIVE | PR_PADLEFT0);
			where = VA_ARG(args, CHAR16 *);
			if (!where) {
				flags &= ~PR_ASCII;
				where = L"(null)";
			}
			break;
		case 'g':
			flags &= ~(PR_DO_SIGN | PR_NEGATIVE | PR_PADLEFT0);
			where = (CHAR16 *) GuidToString(buf, VA_ARG(args, EFI_GUID *));
			break;
/* bogus conversion character -- copy it to output and go back to state 0 */
		default:
			fn(*fmt, ptr);
			count++;
			state = flags = given_wd = 0;
			continue;
		}

/* emit formatted string */
		actual_wd = (flags & PR_ASCII) ? strlena((CHAR8 *) where) :
				StrLen(where);
		if (flags & (PR_POINTER | PR_NEGATIVE))
		{
			actual_wd += 1 + ((flags & PR_POINTER) != 0);
/* if we pad left with ZEROES, do the sign now
(for numeric values; not for %c or %s) */
			if (flags & PR_PADLEFT0) {
				if (flags & PR_POINTER) {
					fn('0', ptr);
					fn('x', ptr);
					count += 2;
				} else {
					fn('-', ptr);
					count++;
				}
			}
		}
/* pad on left with spaces or zeroes (for right justify) */
		if ((flags & PR_LEFTJUST) == 0)
		{
			for (; given_wd > actual_wd; given_wd--)
			{
				fn(flags & PR_PADLEFT0 ? '0' : ' ', ptr);
				count++;
			}
		}
/* if we pad left with SPACES, do the sign now */
		if ((flags & (PR_POINTER | PR_NEGATIVE) &&
					!(flags & PR_PADLEFT0)))
		{
			if (flags & PR_POINTER) {
				fn('0', ptr);
				fn('x', ptr);
				count += 2;
			} else {
				fn('-', ptr);
				count++;
			}
		}
/* emit converted number/char/string */
		if (flags & PR_ASCII) {
			/* ISO-8859-1 to UCS-2 */
			UINT8 *where_ascii = (UINT8 *) where;
			for (; *where_ascii != '\0'; where_ascii++)
			{
				fn(*where_ascii, ptr);
				count++;
			}
		} else {
			for (; *where != '\0'; where++)
			{
				fn(*where, ptr);
				count++;
			}
		}
/* pad on right with spaces (for left justify) */
		if (given_wd < actual_wd)
			given_wd = 0;
		else
			given_wd -= actual_wd;
		for (; given_wd; given_wd--)
		{
			fn(' ', ptr);
			count++;
		}
		/* FALL THROUGH */
	default:
		state = flags = given_wd = 0;
		break;
	}

	fn('\0', ptr);

	return count;
}

struct VSPrint_State {
	CHAR16 *Cur;
	UINTN Num;
};

static void VSPrint_Output(CHAR16 ch, void * _state)
{
	struct VSPrint_State *state = (struct VSPrint_State *) _state;

	if (state->Num != 0) {
		state->Num--;
		*state->Cur++ = ch;
	} else {
		*state->Cur = '\0';
	}
}

static void VSPrint_Discard(CHAR16 ch, void * _state) { }

UINTN
VSPrint (OUT CHAR16	*buf,
	IN UINTN	bufSize,
	IN CONST CHAR16	*fmt,
	VA_LIST args)
{
	UINTN n = (bufSize / sizeof(CHAR16)) - 1;
	struct VSPrint_State state = { .Cur = buf, .Num = n };
	return do_vprintf(fmt, buf ? VSPrint_Output : VSPrint_Discard,
			&state, args);
}

UINTN
SPrint (OUT CHAR16	*buf,
	IN UINTN	bufSize,
	IN CONST CHAR16	*fmt,
	...)
{
	VA_LIST args;
	UINTN rv;

	VA_START(args, fmt);
	rv = VSPrint(buf, bufSize, fmt, args);
	VA_END(args);
	return rv;
}

#define POOL_PRINT_STEP	256

static void VCatPrint_Output(CHAR16 ch, void * _state)
{
	POOL_PRINT *state = (POOL_PRINT *) _state;

	/* Reallocate if the buffer is too small */
	if (state->len == state->max)
	{
		CHAR16 *str;
		state->max += POOL_PRINT_STEP;
		str = (CHAR16 *) ReallocatePool(state->str,
							state->len * sizeof(CHAR16),
							state->max * sizeof(CHAR16));
		/* XXX: Not clear what to do if we run out of memory */
		if (!str)
		{
			if (state->str) {
				FreePool(state->str);
				state->str = NULL;
			}
			state->len = 0;
			state->max = 0;
			return;
		}
		state->str = str;
	}

	state->str[state->len++] = ch;
}

static inline CHAR16 *VCatPrint(
	IN OUT POOL_PRINT	*buf,
	IN CONST CHAR16		*fmt,
	VA_LIST args)
{
	do_vprintf(fmt, VCatPrint_Output, buf, args);
	buf->len--; /* exclude '\0' */
	return buf->str;
}

CHAR16 *CatPrint (
	IN OUT POOL_PRINT	*buf,
	IN CONST CHAR16		*fmt,
	...)
{
	VA_LIST args;
	CHAR16 *rv;

	VA_START(args, fmt);
	rv = VCatPrint(buf, fmt, args);
	VA_END(args);
	return rv;
}

CHAR16 *VPoolPrint (
	IN CONST CHAR16	*fmt,
	VA_LIST		args
	)
{
	POOL_PRINT state = { .str = NULL, .max = 0, .len = 0 };
	return VCatPrint(&state, fmt, args);
}

CHAR16 *PoolPrint (
	IN CONST CHAR16	*fmt,
	...
	)
{
	POOL_PRINT state = { .str = NULL, .max = 0, .len = 0 };
	VA_LIST args;
	CHAR16 *rv;

	VA_START(args, fmt);
	rv = VCatPrint(&state, fmt, args);
	VA_END(args);
	return rv;
}

struct VPrint_State {
	CHAR16 Buffer[192];
	UINTN Length;
};

static void VPrint_Output(CHAR16 ch, void * _state)
{
	struct VPrint_State *state = (struct VPrint_State *) _state;

	/* Flush the buffer (reserve extra space for '\r' and '\0') */
	if ((ch == '\0' && state->Length != 0) ||
			state->Length >= (sizeof(state->Buffer) /
				sizeof(state->Buffer[0]) - 2))
	{
		/* Remove the last surrogate character if any */
		CHAR16 last = '\0';
		if ((state->Buffer[state->Length-1] & 0xFC00) == 0xD800
				&& ch != '\0') {
			last = state->Buffer[--state->Length];
		}
		state->Buffer[state->Length] = '\0';
		state->Length = 0;
		gST->ConOut->OutputString(gST->ConOut, state->Buffer);
		/* Insert the surrogate pair if any */
		if (last != '\0') {
			state->Buffer[0] = last;
			state->Buffer[1] = ch;
			state->Length = 2;
		}
	}
	else
	{
		if (ch == '\n')
			state->Buffer[state->Length++] = '\r';
		state->Buffer[state->Length++] = ch;
	}
}

UINTN
VPrint (IN CONST CHAR16 *fmt,
	VA_LIST args
	)
{
	struct VPrint_State state;
	state.Length = 0;
	return do_vprintf(fmt, VPrint_Output, &state, args);
}
