/*
 * Copyright 2012 <James.Bottomley@HansenPartnership.com>
 * Copyright 2013 Red Hat Inc. <pjones@redhat.com>
 *
 * see COPYING file
 */
#include <efi.h>
#include <efilib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "shim.h"

static UINT8 console_text_mode = 0;

static int
count_lines(CHAR16 *str_arr[])
{
	int i = 0;

	while (str_arr[i])
		i++;
	return i;
}

static void
SetMem16(CHAR16 *dst, UINT32 n, CHAR16 c)
{
	unsigned int i;

	for (i = 0; i < n/2; i++) {
		dst[i] = c;
	}
}

EFI_STATUS
console_get_keystroke(EFI_INPUT_KEY *key)
{
	SIMPLE_INPUT_INTERFACE *ci = ST->ConIn;
	UINTN EventIndex;
	EFI_STATUS efi_status;

	do {
		gBS->WaitForEvent(1, &ci->WaitForKey, &EventIndex);
		efi_status = ci->ReadKeyStroke(ci, key);
	} while (efi_status == EFI_NOT_READY);

	return efi_status;
}

static VOID setup_console (int text)
{
	EFI_STATUS efi_status;
	EFI_CONSOLE_CONTROL_PROTOCOL *concon;
	static EFI_CONSOLE_CONTROL_SCREEN_MODE mode =
					EfiConsoleControlScreenGraphics;
	EFI_CONSOLE_CONTROL_SCREEN_MODE new_mode;

	efi_status = LibLocateProtocol(&EFI_CONSOLE_CONTROL_GUID,
				       (VOID **)&concon);
	if (EFI_ERROR(efi_status))
		return;

	if (text) {
		new_mode = EfiConsoleControlScreenText;

		efi_status = concon->GetMode(concon, &mode, 0, 0);
		/* If that didn't work, assume it's graphics */
		if (EFI_ERROR(efi_status))
			mode = EfiConsoleControlScreenGraphics;
		if (text < 0) {
			if (mode == EfiConsoleControlScreenGraphics)
				console_text_mode = 0;
			else
				console_text_mode = 1;
			return;
		}
	} else {
		new_mode = mode;
	}

	concon->SetMode(concon, new_mode);
	console_text_mode = text;
}

VOID console_fini(VOID)
{
	if (console_text_mode)
		setup_console(0);
}

UINTN
console_print(const CHAR16 *fmt, ...)
{
	va_list args;
	UINTN ret;

	if (!console_text_mode)
		setup_console(1);

	va_start(args, fmt);
	ret = VPrint(fmt, args);
	va_end(args);

	return ret;
}

UINTN
console_print_at(UINTN col, UINTN row, const CHAR16 *fmt, ...)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *co = ST->ConOut;
	va_list args;
	UINTN ret;

	if (!console_text_mode)
		setup_console(1);

	co->SetCursorPosition(co, col, row);

	va_start(args, fmt);
	ret = VPrint(fmt, args);
	va_end(args);

	return ret;
}


void
console_print_box_at(CHAR16 *str_arr[], int highlight,
		     int start_col, int start_row,
		     int size_cols, int size_rows,
		     int offset, int lines)
{
	int i;
	SIMPLE_TEXT_OUTPUT_INTERFACE *co = ST->ConOut;
	UINTN rows, cols;
	CHAR16 *Line;

	if (lines == 0)
		return;

	if (!console_text_mode)
		setup_console(1);

	co->QueryMode(co, co->Mode->Mode, &cols, &rows);

	/* last row on screen is unusable without scrolling, so ignore it */
	rows--;

	if (size_rows < 0)
		size_rows = rows + size_rows + 1;
	if (size_cols < 0)
		size_cols = cols + size_cols + 1;

	if (start_col < 0)
		start_col = (cols + start_col + 2)/2;
	if (start_row < 0)
		start_row = (rows + start_row + 2)/2;
	if (start_col < 0)
		start_col = 0;
	if (start_row < 0)
		start_row = 0;

	if (start_col > (int)cols || start_row > (int)rows) {
		console_print(L"Starting Position (%d,%d) is off screen\n",
			      start_col, start_row);
		return;
	}
	if (size_cols + start_col > (int)cols)
		size_cols = cols - start_col;
	if (size_rows + start_row > (int)rows)
		size_rows = rows - start_row;

	if (lines > size_rows - 2)
		lines = size_rows - 2;

	Line = AllocatePool((size_cols+1)*sizeof(CHAR16));
	if (!Line) {
		console_print(L"Failed Allocation\n");
		return;
	}

	SetMem16 (Line, size_cols * 2, BOXDRAW_HORIZONTAL);

	Line[0] = BOXDRAW_DOWN_RIGHT;
	Line[size_cols - 1] = BOXDRAW_DOWN_LEFT;
	Line[size_cols] = L'\0';
	co->SetCursorPosition(co, start_col, start_row);
	co->OutputString(co, Line);

	int start;
	if (offset == 0)
		/* middle */
		start = (size_rows - lines)/2 + start_row + offset;
	else if (offset < 0)
		/* from bottom */
		start = start_row + size_rows - lines + offset - 1;
	else
		/* from top */
		start = start_row + offset;

	for (i = start_row + 1; i < size_rows + start_row - 1; i++) {
		int line = i - start;

		SetMem16 (Line, size_cols*2, L' ');
		Line[0] = BOXDRAW_VERTICAL;
		Line[size_cols - 1] = BOXDRAW_VERTICAL;
		Line[size_cols] = L'\0';
		if (line >= 0 && line < lines) {
			CHAR16 *s = str_arr[line];
			int len = StrLen(s);
			int col = (size_cols - 2 - len)/2;

			if (col < 0)
				col = 0;

			CopyMem(Line + col + 1, s, min(len, size_cols - 2)*2);
		}
		if (line >= 0 && line == highlight)
			co->SetAttribute(co, EFI_LIGHTGRAY |
					       EFI_BACKGROUND_BLACK);
		co->SetCursorPosition(co, start_col, i);
		co->OutputString(co, Line);
		if (line >= 0 && line == highlight)
			co->SetAttribute(co, EFI_LIGHTGRAY |
					       EFI_BACKGROUND_BLUE);

	}
	SetMem16 (Line, size_cols * 2, BOXDRAW_HORIZONTAL);
	Line[0] = BOXDRAW_UP_RIGHT;
	Line[size_cols - 1] = BOXDRAW_UP_LEFT;
	Line[size_cols] = L'\0';
	co->SetCursorPosition(co, start_col, i);
	co->OutputString(co, Line);

	FreePool (Line);

}

void
console_print_box(CHAR16 *str_arr[], int highlight)
{
	SIMPLE_TEXT_OUTPUT_MODE SavedConsoleMode;
	SIMPLE_TEXT_OUTPUT_INTERFACE *co = ST->ConOut;
	EFI_INPUT_KEY key;

	if (!console_text_mode)
		setup_console(1);

	CopyMem(&SavedConsoleMode, co->Mode, sizeof(SavedConsoleMode));
	co->EnableCursor(co, FALSE);
	co->SetAttribute(co, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);

	console_print_box_at(str_arr, highlight, 0, 0, -1, -1, 0,
			     count_lines(str_arr));

	console_get_keystroke(&key);

	co->EnableCursor(co, SavedConsoleMode.CursorVisible);
	co->SetCursorPosition(co, SavedConsoleMode.CursorColumn,
				SavedConsoleMode.CursorRow);
	co->SetAttribute(co, SavedConsoleMode.Attribute);
}

int
console_select(CHAR16 *title[], CHAR16* selectors[], unsigned int start)
{
	SIMPLE_TEXT_OUTPUT_MODE SavedConsoleMode;
	SIMPLE_TEXT_OUTPUT_INTERFACE *co = ST->ConOut;
	EFI_INPUT_KEY k;
	EFI_STATUS efi_status;
	int selector;
	unsigned int selector_lines = count_lines(selectors);
	int selector_max_cols = 0;
	unsigned int i;
	int offs_col, offs_row, size_cols, size_rows, lines;
	unsigned int selector_offset;
	UINTN cols, rows;

	if (!console_text_mode)
		setup_console(1);

	co->QueryMode(co, co->Mode->Mode, &cols, &rows);

	for (i = 0; i < selector_lines; i++) {
		int len = StrLen(selectors[i]);

		if (len > selector_max_cols)
			selector_max_cols = len;
	}

	if (start >= selector_lines)
		start = selector_lines - 1;

	offs_col = - selector_max_cols - 4;
	size_cols = selector_max_cols + 4;

	if (selector_lines > rows - 10) {
		int title_lines = count_lines(title);
		offs_row =  title_lines + 1;
		size_rows = rows - 3 - title_lines;
		lines = size_rows - 2;
	} else {
		offs_row = - selector_lines - 4;
		size_rows = selector_lines + 2;
		lines = selector_lines;
	}

	if (start > (unsigned)lines) {
		selector = lines;
		selector_offset = start - lines;
	} else {
		selector = start;
		selector_offset = 0;
	}

	CopyMem(&SavedConsoleMode, co->Mode, sizeof(SavedConsoleMode));
	co->EnableCursor(co, FALSE);
	co->SetAttribute(co, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);

	console_print_box_at(title, -1, 0, 0, -1, -1, 1, count_lines(title));

	console_print_box_at(selectors, selector, offs_col, offs_row,
			     size_cols, size_rows, 0, lines);

	do {
		efi_status = console_get_keystroke(&k);
		if (EFI_ERROR (efi_status)) {
			console_print(L"Failed to read the keystroke: %r",
				      efi_status);
			selector = -1;
			break;
		}

		if (k.ScanCode == SCAN_ESC) {
			selector = -1;
			break;
		}

		if (k.ScanCode == SCAN_UP) {
			if (selector > 0)
				selector--;
			else if (selector_offset > 0)
				selector_offset--;
		} else if (k.ScanCode == SCAN_DOWN) {
			if (selector < lines - 1)
				selector++;
			else if (selector_offset < (selector_lines - lines))
				selector_offset++;
		}

		console_print_box_at(&selectors[selector_offset], selector,
				     offs_col, offs_row,
				     size_cols, size_rows, 0, lines);
	} while (!(k.ScanCode == SCAN_NULL
		   && k.UnicodeChar == CHAR_CARRIAGE_RETURN));

	co->EnableCursor(co, SavedConsoleMode.CursorVisible);
	co->SetCursorPosition(co, SavedConsoleMode.CursorColumn,
			      SavedConsoleMode.CursorRow);
	co->SetAttribute(co, SavedConsoleMode.Attribute);

	if (selector < 0)
		/* ESC pressed */
		return selector;
	return selector + selector_offset;
}


int
console_yes_no(CHAR16 *str_arr[])
{
	CHAR16 *yes_no[] = { L"No", L"Yes", NULL };
	return console_select(str_arr, yes_no, 0);
}

void
console_alertbox(CHAR16 **title)
{
	CHAR16 *okay[] = { L"OK", NULL };
	console_select(title, okay, 0);
}

void
console_errorbox(CHAR16 *err)
{
	CHAR16 **err_arr = (CHAR16 *[]){
		L"ERROR",
		L"",
		0,
		0,
	};

	err_arr[2] = err;

	console_alertbox(err_arr);
}

void
console_notify(CHAR16 *string)
{
	CHAR16 **str_arr = (CHAR16 *[]){
		0,
		0,
	};

	str_arr[0] = string;

	console_alertbox(str_arr);
}

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

/* Copy of gnu-efi-3.0 with the added secure boot strings */
static struct {
    EFI_STATUS      Code;
    WCHAR	    *Desc;
} error_table[] = {
	{  EFI_SUCCESS,                L"Success"},
	{  EFI_LOAD_ERROR,             L"Load Error"},
	{  EFI_INVALID_PARAMETER,      L"Invalid Parameter"},
	{  EFI_UNSUPPORTED,            L"Unsupported"},
	{  EFI_BAD_BUFFER_SIZE,        L"Bad Buffer Size"},
	{  EFI_BUFFER_TOO_SMALL,       L"Buffer Too Small"},
	{  EFI_NOT_READY,              L"Not Ready"},
	{  EFI_DEVICE_ERROR,           L"Device Error"},
	{  EFI_WRITE_PROTECTED,        L"Write Protected"},
	{  EFI_OUT_OF_RESOURCES,       L"Out of Resources"},
	{  EFI_VOLUME_CORRUPTED,       L"Volume Corrupt"},
	{  EFI_VOLUME_FULL,            L"Volume Full"},
	{  EFI_NO_MEDIA,               L"No Media"},
	{  EFI_MEDIA_CHANGED,          L"Media changed"},
	{  EFI_NOT_FOUND,              L"Not Found"},
	{  EFI_ACCESS_DENIED,          L"Access Denied"},
	{  EFI_NO_RESPONSE,            L"No Response"},
	{  EFI_NO_MAPPING,             L"No mapping"},
	{  EFI_TIMEOUT,                L"Time out"},
	{  EFI_NOT_STARTED,            L"Not started"},
	{  EFI_ALREADY_STARTED,        L"Already started"},
	{  EFI_ABORTED,                L"Aborted"},
	{  EFI_ICMP_ERROR,             L"ICMP Error"},
	{  EFI_TFTP_ERROR,             L"TFTP Error"},
	{  EFI_PROTOCOL_ERROR,         L"Protocol Error"},
	{  EFI_INCOMPATIBLE_VERSION,   L"Incompatible Version"},
	{  EFI_SECURITY_VIOLATION,     L"Security Violation"},

	// warnings
	{  EFI_WARN_UNKOWN_GLYPH,      L"Warning Unknown Glyph"},
	{  EFI_WARN_DELETE_FAILURE,    L"Warning Delete Failure"},
	{  EFI_WARN_WRITE_FAILURE,     L"Warning Write Failure"},
	{  EFI_WARN_BUFFER_TOO_SMALL,  L"Warning Buffer Too Small"},
	{  0, NULL}
} ;


static CHAR16 *
err_string (
    IN EFI_STATUS       efi_status
    )
{
	UINTN           Index;

	for (Index = 0; error_table[Index].Desc; Index +=1) {
		if (error_table[Index].Code == efi_status) {
			return error_table[Index].Desc;
		}
	}

	return L"";
}

void
console_error(CHAR16 *err, EFI_STATUS efi_status)
{
	CHAR16 **err_arr = (CHAR16 *[]){
		L"ERROR",
		L"",
		0,
		0,
	};
	CHAR16 str[512];

	SPrint(str, sizeof(str), L"%s: (0x%x) %s", err, efi_status,
	       err_string(efi_status));

	err_arr[2] = str;

	console_alertbox(err_arr);
}

void
console_reset(void)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *co = ST->ConOut;

	if (!console_text_mode)
		setup_console(1);

	co->Reset(co, TRUE);
	/* set mode 0 - required to be 80x25 */
	co->SetMode(co, 0);
	co->ClearScreen(co);
}

UINT32 verbose = 0;

VOID
setup_verbosity(VOID)
{
	EFI_STATUS efi_status;
	UINT8 *verbose_check_ptr = NULL;
	UINTN verbose_check_size;

	verbose_check_size = sizeof(verbose);
	efi_status = get_variable(L"SHIM_VERBOSE", &verbose_check_ptr,
				  &verbose_check_size, SHIM_LOCK_GUID);
	if (!EFI_ERROR(efi_status)) {
		verbose = *(__typeof__(verbose) *)verbose_check_ptr;
		verbose &= (1ULL << (8 * verbose_check_size)) - 1ULL;
		FreePool(verbose_check_ptr);
	}

	setup_console(-1);
}

/* Included here because they mess up the definition of va_list and friends */
#include <Library/BaseCryptLib.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

static int
print_errors_cb(const char *str, size_t len, void *u)
{
	console_print(L"%a", str);

	return len;
}

EFI_STATUS
print_crypto_errors(EFI_STATUS efi_status,
		    char *file, const char *func, int line)
{
	if (!(verbose && EFI_ERROR(efi_status)))
		return efi_status;

	console_print(L"SSL Error: %a:%d %a(): %r\n", file, line, func,
		      efi_status);
	ERR_print_errors_cb(print_errors_cb, NULL);

	return efi_status;
}

VOID
msleep(unsigned long msecs)
{
	gBS->Stall(msecs);
}

/* This is used in various things to determine if we should print to the
 * console */
UINT8 in_protocol = 0;
