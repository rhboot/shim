// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * Copyright 2015 SUSE LINUX GmbH <glin@suse.com>
 *
 * Significant portions of this code are derived from Tianocore
 * (http://tianocore.sf.net) and are Copyright 2009-2012 Intel
 * Corporation.
 */

#ifndef SHIM_HTTPBOOT_H
#define SHIM_HTTPBOOT_H

extern BOOLEAN find_httpboot(EFI_HANDLE device);
extern EFI_STATUS httpboot_fetch_buffer(EFI_HANDLE image, VOID **buffer,
					UINT64 *buf_size, CHAR8 *name);

#endif /* SHIM_HTTPBOOT_H */
