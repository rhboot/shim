// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SHIM_CONSOLE_H
#define SHIM_CONSOLE_H

#define Print(fmt, ...) \
	({"Do not directly call Print() use console_print() instead" = 1;});

#define PrintAt(fmt, ...) \
	({"Do not directly call PrintAt() use console_print_at() instead" = 1;});

#if !defined(EFI_WARN_UNKNOWN_GLYPH) && defined(EFI_WARN_UNKOWN_GLYPH)
#define EFI_WARN_UNKNOWN_GLYPH EFI_WARN_UNKOWN_GLYPH
#elif !defined(EFI_WARN_UNKNOWN_GLYPH)
#define EFI_WARN_UNKNOWN_GLYPH EFIWARN(1)
#endif

EFI_STATUS
console_get_keystroke(EFI_INPUT_KEY *key);
UINTN EFIAPI
console_print(const CHAR16 *fmt, ...);
UINTN EFIAPI
console_print_at(UINTN col, UINTN row, const CHAR16 *fmt, ...);
void
console_print_box_at(CHAR16 *str_arr[], int highlight,
		     int start_col, int start_row,
		     int size_cols, int size_rows,
		     int offset, int lines);
void
console_print_box(CHAR16 *str_arr[], int highlight);
int
console_yes_no(CHAR16 *str_arr[]);
int
console_select(CHAR16 *title[], CHAR16* selectors[], unsigned int start);
void
console_errorbox(CHAR16 *err);
void
console_error(CHAR16 *err, EFI_STATUS);
void
console_alertbox(CHAR16 **title);
void
console_notify(CHAR16 *string);
void
console_save_and_set_mode(SIMPLE_TEXT_OUTPUT_MODE * SavedMode);
void
console_restore_mode(SIMPLE_TEXT_OUTPUT_MODE * SavedMode);
int
console_countdown(CHAR16* title, const CHAR16* message, int timeout);
void
console_reset(void);
void
console_mode_handle(void);
void
clear_screen(void);

#define NOSEL 0x7fffffff

typedef struct _EFI_CONSOLE_CONTROL_PROTOCOL   EFI_CONSOLE_CONTROL_PROTOCOL;

typedef enum {
  EfiConsoleControlScreenText,
  EfiConsoleControlScreenGraphics,
  EfiConsoleControlScreenMaxValue
} EFI_CONSOLE_CONTROL_SCREEN_MODE;

typedef
EFI_STATUS
(EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_GET_MODE) (
  IN  EFI_CONSOLE_CONTROL_PROTOCOL      *This,
  OUT EFI_CONSOLE_CONTROL_SCREEN_MODE   *Mode,
  OUT BOOLEAN                           *GopUgaExists,  OPTIONAL
  OUT BOOLEAN                           *StdInLocked    OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE) (
  IN  EFI_CONSOLE_CONTROL_PROTOCOL      *This,
  IN  EFI_CONSOLE_CONTROL_SCREEN_MODE   Mode
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_LOCK_STD_IN) (
  IN  EFI_CONSOLE_CONTROL_PROTOCOL      *This,
  IN CHAR16                             *Password
  );

struct _EFI_CONSOLE_CONTROL_PROTOCOL {
  EFI_CONSOLE_CONTROL_PROTOCOL_GET_MODE           GetMode;
  EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE           SetMode;
  EFI_CONSOLE_CONTROL_PROTOCOL_LOCK_STD_IN        LockStdIn;
};

extern VOID console_fini(VOID);
extern VOID setup_verbosity(VOID);
extern UINT32 verbose;
#ifndef SHIM_UNIT_TEST
#define dprint_(fmt, ...) ({							\
		UINTN __dprint_ret = 0;						\
		if (verbose)							\
			__dprint_ret = console_print((fmt), ##__VA_ARGS__);	\
		__dprint_ret;							\
	})
#define dprint(fmt, ...)                                              \
	dprint_(L"%a:%d:%a() " fmt, __FILE__, __LINE__ - 1, __func__, \
	        ##__VA_ARGS__)
#else
#define dprint_(...) ({ ; })
#define dprint(fmt, ...) ({ ; })
#endif

extern EFI_STATUS EFIAPI vdprint_(const CHAR16 *fmt, const char *file, int line,
                                  const char *func, ms_va_list args);
#if defined(SHIM_UNIT_TEST)
#define vdprint(fmt, ...)
#else
#define vdprint(fmt, ...) \
	vdprint_(fmt, __FILE__, __LINE__ - 1, __func__, ##__VA_ARGS__)
#endif

extern EFI_STATUS print_crypto_errors(EFI_STATUS rc, char *file, const char *func, int line);
#define crypterr(rc) print_crypto_errors((rc), __FILE__, __func__, __LINE__)

#ifndef SHIM_UNIT_TEST
extern VOID usleep(unsigned long usecs);
#endif

/* This is used in various things to determine if we should print to the
 * console */
extern UINT8 in_protocol;

#endif /* SHIM_CONSOLE_H */
