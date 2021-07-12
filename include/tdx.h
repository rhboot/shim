// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SHIM_TDX_H
#define SHIM_TDX_H

typedef efi_tpm2_protocol_t efi_tdx_protocol_t;

EFI_STATUS tdx_log_event_raw(EFI_PHYSICAL_ADDRESS buf, UINTN size,
			     UINT8 pcr, const CHAR8 *log, UINTN logsize,
			     UINT32 type, CHAR8 *hash);

#endif /* SHIM_TPM_H */
// vim:fenc=utf-8:tw=75