// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * verify.h - verification routines for UEFI Secure Boot
 * Copyright Peter Jones <pjones@redhat.com>
 * Copyright Matthew Garrett
 *
 * Significant portions of this code are derived from Tianocore
 * (http://tianocore.sf.net) and are Copyright 2009-2012 Intel
 * Corporation.
 */
#pragma once

EFI_STATUS
verify_buffer (char *data, int datasize,
	       PE_COFF_LOADER_IMAGE_CONTEXT *context,
	       UINT8 *sha256hash, UINT8 *sha1hash,
	       bool parent_verified);

void
init_openssl(void);

/*
 * Protocol v1 entry points.
 */

/*
 * If secure boot is enabled, verify that the provided buffer is signed
 * with a trusted key.
 */
EFI_STATUS
shim_verify(void *buffer, UINT32 size);

EFI_STATUS
shim_hash(char *data, int datasize, PE_COFF_LOADER_IMAGE_CONTEXT *context,
          UINT8 *sha256hash, UINT8 *sha1hash);

EFI_STATUS
shim_read_header(void *data, unsigned int datasize,
                 PE_COFF_LOADER_IMAGE_CONTEXT *context);

// vim:fenc=utf-8:tw=75:noet
