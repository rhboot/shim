// SPDX-License-Identifier: BSD-2-Clause-Patent
#include "shim.h"

EFI_STATUS tdx_log_event_raw(EFI_PHYSICAL_ADDRESS buf, UINTN size,
			     UINT8 pcr, const CHAR8 *log, UINTN logsize,
			     UINT32 type, CHAR8 *hash)
{
	EFI_STATUS efi_status;
	EFI_TCG2_EVENT *event;
	efi_tdx_protocol_t *tdx;

        efi_status = LibLocateProtocol(&EFI_TDX_GUID, (VOID **)&tdx);
	if (EFI_ERROR(efi_status))
		return EFI_SUCCESS;

	UINTN event_size = sizeof(*event) - sizeof(event->Event) +
		logsize;

	event = AllocatePool(event_size);
	if (!event) {
		perror(L"Unable to allocate event structure\n");
		return EFI_OUT_OF_RESOURCES;
	}

	event->Header.HeaderSize = sizeof(EFI_TCG2_EVENT_HEADER);
	event->Header.HeaderVersion = 1;
	event->Header.PCRIndex = pcr;
	event->Header.EventType = type;
	event->Size = event_size;
	CopyMem(event->Event, (VOID *)log, logsize);
	if (hash) {
		/* TCG 2 systems will generate the appropriate hash
			themselves if we pass PE_COFF_IMAGE.  In case that
			fails we fall back to measuring without it.
		*/
		efi_status = tdx->hash_log_extend_event(tdx,
			PE_COFF_IMAGE, buf, (UINT64) size, event);
	}

	if (!hash || EFI_ERROR(efi_status)) {
		efi_status = tdx->hash_log_extend_event(tdx,
			0, buf, (UINT64) size, event);
	}
	FreePool(event);
	return efi_status;
}