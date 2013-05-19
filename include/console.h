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
console_reset(void);
#define NOSEL 0x7fffffff
