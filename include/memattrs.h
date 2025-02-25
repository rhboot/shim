// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * memattrs.h - EFI and DXE memory attribute helpers
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_MEMATTRS_H_
#define SHIM_MEMATTRS_H_

extern EFI_STATUS get_mem_attrs (uintptr_t addr, size_t size, uint64_t *attrs);
extern EFI_STATUS update_mem_attrs(uintptr_t addr, uint64_t size,
				   uint64_t set_attrs, uint64_t clear_attrs);

extern void get_hsi_mem_info(void);
extern char *decode_hsi_bits(UINTN hsi);

#endif /* !SHIM_MEMATTRS_H_ */
// vim:fenc=utf-8:tw=75:noet
