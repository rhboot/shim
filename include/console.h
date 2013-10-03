#ifndef _SHIM_LIB_CONSOLE_H
#define _SHIM_LIB_CONSOLE_H 1

EFI_INPUT_KEY
console_get_keystroke(void);
void
console_print_box_at(CHAR16 *str_arr[], int highlight, int start_col, int start_row, int size_cols, int size_rows, int offset, int lines);
void
console_print_box(CHAR16 *str_arr[], int highlight);
int
console_yes_no(CHAR16 *str_arr[]);
int
console_select(CHAR16 *title[], CHAR16* selectors[], int start);
void
console_errorbox(CHAR16 *err);
void
console_error(CHAR16 *err, EFI_STATUS);
void
console_alertbox(CHAR16 **title);
void
console_notify(CHAR16 *string);
void
console_notify_ascii(CHAR8 *string);
void
console_reset(void);
#define NOSEL 0x7fffffff

#define EFI_CONSOLE_CONTROL_PROTOCOL_GUID \
  { 0xf42f7782, 0x12e, 0x4c12, {0x99, 0x56, 0x49, 0xf9, 0x43, 0x4, 0xf7, 0x21} }

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

extern VOID setup_console (int text);

#endif /* _SHIM_LIB_CONSOLE_H */
