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
#include <console.h>
#include <variables.h>
#include <errors.h>
#include <Library/BaseCryptLib.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

static EFI_GUID SHIM_LOCK_GUID = { 0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23} };

static int min(int a, int b)
{
	if (a < b)
		return a;
	return b;
}

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
	UINTN EventIndex;
	EFI_STATUS status;

	do {
		uefi_call_wrapper(BS->WaitForEvent, 3, 1, &ST->ConIn->WaitForKey, &EventIndex);
		status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, key);
	} while (status == EFI_NOT_READY);

	return status;
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

	uefi_call_wrapper(co->QueryMode, 4, co, co->Mode->Mode, &cols, &rows);

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
		Print(L"Starting Position (%d,%d) is off screen\n",
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
		Print(L"Failed Allocation\n");
		return;
	}

	SetMem16 (Line, size_cols * 2, BOXDRAW_HORIZONTAL);

	Line[0] = BOXDRAW_DOWN_RIGHT;
	Line[size_cols - 1] = BOXDRAW_DOWN_LEFT;
	Line[size_cols] = L'\0';
	uefi_call_wrapper(co->SetCursorPosition, 3, co, start_col, start_row);
	uefi_call_wrapper(co->OutputString, 2, co, Line);

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
			uefi_call_wrapper(co->SetAttribute, 2, co, EFI_LIGHTGRAY | EFI_BACKGROUND_BLACK);
		uefi_call_wrapper(co->SetCursorPosition, 3, co, start_col, i);
		uefi_call_wrapper(co->OutputString, 2, co, Line);
		if (line >= 0 && line == highlight)
			uefi_call_wrapper(co->SetAttribute, 2, co, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);

	}
	SetMem16 (Line, size_cols * 2, BOXDRAW_HORIZONTAL);
	Line[0] = BOXDRAW_UP_RIGHT;
	Line[size_cols - 1] = BOXDRAW_UP_LEFT;
	Line[size_cols] = L'\0';
	uefi_call_wrapper(co->SetCursorPosition, 3, co, start_col, i);
	uefi_call_wrapper(co->OutputString, 2, co, Line);

	FreePool (Line);

}

void
console_print_box(CHAR16 *str_arr[], int highlight)
{
	SIMPLE_TEXT_OUTPUT_MODE SavedConsoleMode;
	SIMPLE_TEXT_OUTPUT_INTERFACE *co = ST->ConOut;
	EFI_INPUT_KEY key;

	CopyMem(&SavedConsoleMode, co->Mode, sizeof(SavedConsoleMode));
	uefi_call_wrapper(co->EnableCursor, 2, co, FALSE);
	uefi_call_wrapper(co->SetAttribute, 2, co, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);

	console_print_box_at(str_arr, highlight, 0, 0, -1, -1, 0,
			     count_lines(str_arr));

	console_get_keystroke(&key);

	uefi_call_wrapper(co->EnableCursor, 2, co, SavedConsoleMode.CursorVisible);
	uefi_call_wrapper(co->SetCursorPosition, 3, co, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
	uefi_call_wrapper(co->SetAttribute, 2, co, SavedConsoleMode.Attribute);
}

int
console_select(CHAR16 *title[], CHAR16* selectors[], unsigned int start)
{
	SIMPLE_TEXT_OUTPUT_MODE SavedConsoleMode;
	SIMPLE_TEXT_OUTPUT_INTERFACE *co = ST->ConOut;
	EFI_INPUT_KEY k;
	EFI_STATUS status;
	int selector;
	unsigned int selector_lines = count_lines(selectors);
	int selector_max_cols = 0;
	unsigned int i;
	int offs_col, offs_row, size_cols, size_rows, lines;
	unsigned int selector_offset;
	UINTN cols, rows;

	uefi_call_wrapper(co->QueryMode, 4, co, co->Mode->Mode, &cols, &rows);

	for (i = 0; i < selector_lines; i++) {
		int len = StrLen(selectors[i]);

		if (len > selector_max_cols)
			selector_max_cols = len;
	}

	if (start < 0)
		start = 0;
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
	uefi_call_wrapper(co->EnableCursor, 2, co, FALSE);
	uefi_call_wrapper(co->SetAttribute, 2, co, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);

	console_print_box_at(title, -1, 0, 0, -1, -1, 1, count_lines(title));

	console_print_box_at(selectors, selector, offs_col, offs_row,
			     size_cols, size_rows, 0, lines);

	do {
		status = console_get_keystroke(&k);
		if (EFI_ERROR (status)) {
			Print(L"Failed to read the keystroke: %r", status);
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

	uefi_call_wrapper(co->EnableCursor, 2, co, SavedConsoleMode.CursorVisible);
	uefi_call_wrapper(co->SetCursorPosition, 3, co, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
	uefi_call_wrapper(co->SetAttribute, 2, co, SavedConsoleMode.Attribute);

	if (selector < 0)
		/* ESC pressed */
		return selector;
	return selector + selector_offset;
}


int
console_yes_no(CHAR16 *str_arr[])
{
	return console_select(str_arr, (CHAR16 *[]){ L"No", L"Yes", NULL }, 0);
}

void
console_alertbox(CHAR16 **title)
{
	console_select(title, (CHAR16 *[]){ L"OK", 0 }, 0);
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
    IN EFI_STATUS       Status
    )
{
	UINTN           Index;

	for (Index = 0; error_table[Index].Desc; Index +=1) {
		if (error_table[Index].Code == Status) {
			return error_table[Index].Desc;
		}
	}

	return L"";
}

void
console_error(CHAR16 *err, EFI_STATUS status)
{
	CHAR16 **err_arr = (CHAR16 *[]){
		L"ERROR",
		L"",
		0,
		0,
	};
	CHAR16 str[512];

	SPrint(str, sizeof(str), L"%s: (0x%x) %s", err, status, err_string(status));


	err_arr[2] = str;

	console_alertbox(err_arr);
}

void
console_reset(void)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE *co = ST->ConOut;

	uefi_call_wrapper(co->Reset, 2, co, TRUE);
	/* set mode 0 - required to be 80x25 */
	uefi_call_wrapper(co->SetMode, 2, co, 0);
	uefi_call_wrapper(co->ClearScreen, 1, co);
}

UINT8 verbose;

VOID
setup_verbosity(VOID)
{
	EFI_STATUS status;
	EFI_GUID guid = SHIM_LOCK_GUID;
	UINT8 verbose_check;
	UINTN verbose_check_size;

	verbose_check_size = 1;
	status = get_variable(L"SHIM_VERBOSE", (void *)&verbose_check,
				  &verbose_check_size, guid);
	verbose = 0;
	if (!EFI_ERROR(status))
		verbose = verbose_check;
}

VOID setup_console (int text)
{
	EFI_STATUS status;
	EFI_GUID console_control_guid = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
	EFI_CONSOLE_CONTROL_PROTOCOL *concon;
	static EFI_CONSOLE_CONTROL_SCREEN_MODE mode =
					EfiConsoleControlScreenGraphics;
	EFI_CONSOLE_CONTROL_SCREEN_MODE new_mode;

	status = LibLocateProtocol(&console_control_guid, (VOID **)&concon);
	if (status != EFI_SUCCESS)
		return;

	if (text) {
		new_mode = EfiConsoleControlScreenText;

		status = uefi_call_wrapper(concon->GetMode, 4, concon, &mode,
						0, 0);
		/* If that didn't work, assume it's graphics */
		if (status != EFI_SUCCESS)
			mode = EfiConsoleControlScreenGraphics;
	} else {
		new_mode = mode;
	}

	uefi_call_wrapper(concon->SetMode, 2, concon, new_mode);
}

static int
print_errors_cb(const char *str, size_t len, void *u)
{
	Print(L"%a", str);

	return len;
}

EFI_STATUS
print_crypto_errors(EFI_STATUS rc, char *file, const char *func, int line)
{
	if (!(verbose && EFI_ERROR(rc)))
		return rc;

	Print(L"SSL Error: %a:%d %a(): %r\n", file, line, func, rc);
	ERR_print_errors_cb(print_errors_cb, NULL);

	return rc;
}

VOID
msleep(unsigned long msecs)
{
	uefi_call_wrapper(BS->Stall, 1, msecs);
}
