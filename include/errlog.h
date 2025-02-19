// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * errlog.h - error logging utilities
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef ERRLOG_H_
#define ERRLOG_H_

extern EFI_STATUS EFIAPI LogError_(const char *file, int line, const char *func,
                                   const CHAR16 *fmt, ...);
extern EFI_STATUS EFIAPI VLogError(const char *file, int line, const char *func,
                                   const CHAR16 *fmt, ms_va_list args);
extern VOID LogHexdump_(const char *file, int line, const char *func,
                        const void *data, size_t sz);
extern VOID PrintErrors(VOID);
extern VOID ClearErrors(VOID);
extern void save_logs(void);
extern UINTN EFIAPI log_debug_print(const CHAR16 *fmt, ...);

#endif /* !ERRLOG_H_ */
// vim:fenc=utf-8:tw=75:noet
