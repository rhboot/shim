#include <efi.h>
#include <efilib.h>
#include <string.h>

#include "tpm.h"

extern UINT8 in_protocol;

#define perror(fmt, ...) ({                                             \
			UINTN __perror_ret = 0;                               \
			if (!in_protocol)                                     \
				__perror_ret = Print((fmt), ##__VA_ARGS__);   \
			__perror_ret;                                         \
		})


EFI_GUID tpm_guid = EFI_TPM_GUID;
EFI_GUID tpm2_guid = EFI_TPM2_GUID;

static BOOLEAN tpm_present(efi_tpm_protocol_t *tpm)
{
	EFI_STATUS status;
	TCG_EFI_BOOT_SERVICE_CAPABILITY caps;
	UINT32 flags;
	EFI_PHYSICAL_ADDRESS eventlog, lastevent;

	caps.Size = (UINT8)sizeof(caps);
	status = uefi_call_wrapper(tpm->status_check, 5, tpm, &caps, &flags,
				   &eventlog, &lastevent);

	if (status != EFI_SUCCESS || caps.TPMDeactivatedFlag
	    || !caps.TPMPresentFlag)
		return FALSE;

	return TRUE;
}

static BOOLEAN tpm2_present(efi_tpm2_protocol_t *tpm)
{
	EFI_STATUS status;
	EFI_TCG2_BOOT_SERVICE_CAPABILITY caps;
	EFI_TCG2_BOOT_SERVICE_CAPABILITY_1_0 *caps_1_0;

	caps.Size = (UINT8)sizeof(caps);

	status = uefi_call_wrapper(tpm->get_capability, 2, tpm, &caps);

	if (status != EFI_SUCCESS)
		return FALSE;

	if (caps.StructureVersion.Major == 1 &&
	    caps.StructureVersion.Minor == 0) {
		caps_1_0 = (EFI_TCG2_BOOT_SERVICE_CAPABILITY_1_0 *)&caps;
		if (caps_1_0->TPMPresentFlag)
			return TRUE;
	} else {
		if (caps.TPMPresentFlag)
			return TRUE;
	}

	return FALSE;
}

EFI_STATUS tpm_log_event(EFI_PHYSICAL_ADDRESS buf, UINTN size, UINT8 pcr,
			 const CHAR8 *description)
{
	EFI_STATUS status;
	efi_tpm_protocol_t *tpm;
	efi_tpm2_protocol_t *tpm2;

	status = LibLocateProtocol(&tpm2_guid, (VOID **)&tpm2);
	/* TPM 2.0 */
	if (status == EFI_SUCCESS) {
		EFI_TCG2_EVENT *event;

		if (!tpm2_present(tpm2))
			return EFI_SUCCESS;

		event = AllocatePool(sizeof(*event) + strlen(description) + 1);
		if (!event) {
			perror(L"Unable to allocate event structure\n");
			return EFI_OUT_OF_RESOURCES;
		}

		event->Header.HeaderSize = sizeof(EFI_TCG2_EVENT_HEADER);
		event->Header.HeaderVersion = 1;
		event->Header.PCRIndex = pcr;
		event->Header.EventType = 0x0d;
		event->Size = sizeof(*event) - sizeof(event->Event) + strlen(description) + 1;
		memcpy(event->Event, description, strlen(description) + 1);
		status = uefi_call_wrapper(tpm2->hash_log_extend_event, 5, tpm2,
					   0, buf, (UINT64) size, event);
		FreePool(event);
		return status;
	} else {
		TCG_PCR_EVENT *event;
		UINT32 algorithm, eventnum = 0;
		EFI_PHYSICAL_ADDRESS lastevent;

		status = LibLocateProtocol(&tpm_guid, (VOID **)&tpm);

		if (status != EFI_SUCCESS)
			return EFI_SUCCESS;

		if (!tpm_present(tpm))
			return EFI_SUCCESS;

		event = AllocatePool(sizeof(*event) + strlen(description) + 1);

		if (!event) {
			perror(L"Unable to allocate event structure\n");
			return EFI_OUT_OF_RESOURCES;
		}

		event->PCRIndex = pcr;
		event->EventType = 0x0d;
		event->EventSize = strlen(description) + 1;
		algorithm = 0x00000004;
		status = uefi_call_wrapper(tpm->log_extend_event, 7, tpm, buf,
					   (UINT64)size, algorithm, event,
					   &eventnum, &lastevent);
		FreePool(event);
		return status;
	}

	return EFI_SUCCESS;
}
