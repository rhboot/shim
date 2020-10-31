#ifndef SHIM_TPM_H
#define SHIM_TPM_H

EFI_STATUS tpm_log_event(EFI_PHYSICAL_ADDRESS buf, UINTN size, UINT8 pcr,
			 const CHAR8 *description);
EFI_STATUS fallback_should_prefer_reset(void);

EFI_STATUS tpm_log_pe(EFI_PHYSICAL_ADDRESS buf, UINTN size,
		      EFI_PHYSICAL_ADDRESS addr, EFI_DEVICE_PATH *path,
		      UINT8 *sha1hash, UINT8 pcr);

EFI_STATUS tpm_measure_variable(CHAR16 *dbname, EFI_GUID *guid, UINTN size, CONST VOID *data);

#endif /* SHIM_TPM_H */
// vim:fenc=utf-8:tw=75
