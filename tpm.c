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

static EFI_STATUS tpm2_get_caps(efi_tpm2_protocol_t *tpm,
				EFI_TCG2_BOOT_SERVICE_CAPABILITY *caps,
				BOOLEAN *old_caps)
{
	EFI_STATUS status;

	caps->Size = (UINT8)sizeof(*caps);

	status = uefi_call_wrapper(tpm->get_capability, 2, tpm, caps);

	if (status != EFI_SUCCESS)
		return status;

	if (caps->StructureVersion.Major == 1 &&
	    caps->StructureVersion.Minor == 0)
		*old_caps = TRUE;

	return EFI_SUCCESS;
}

static BOOLEAN tpm2_present(EFI_TCG2_BOOT_SERVICE_CAPABILITY *caps,
			    BOOLEAN old_caps)
{
	TREE_BOOT_SERVICE_CAPABILITY *caps_1_0;

	if (old_caps) {
		caps_1_0 = (TREE_BOOT_SERVICE_CAPABILITY *)caps;
		if (caps_1_0->TrEEPresentFlag)
			return TRUE;
	}

	if (caps->TPMPresentFlag)
		return TRUE;

	return FALSE;
}

static inline EFI_TCG2_EVENT_LOG_BITMAP
tpm2_get_supported_logs(efi_tpm2_protocol_t *tpm,
			EFI_TCG2_BOOT_SERVICE_CAPABILITY *caps,
			BOOLEAN old_caps)
{
	if (old_caps)
		return ((TREE_BOOT_SERVICE_CAPABILITY *)caps)->SupportedEventLogs;

	return caps->SupportedEventLogs;
}

/*
 * According to TCG EFI Protocol Specification for TPM 2.0 family,
 * all events generated after the invocation of EFI_TCG2_GET_EVENT_LOG
 * shall be stored in an instance of an EFI_CONFIGURATION_TABLE aka
 * EFI TCG 2.0 final events table. Hence, it is necessary to trigger the
 * internal switch through calling get_event_log() in order to allow
 * to retrieve the logs from OS runtime.
 */
static EFI_STATUS trigger_tcg2_final_events_table(efi_tpm2_protocol_t *tpm2,
						  EFI_TCG2_EVENT_LOG_BITMAP supported_logs)
{
	EFI_TCG2_EVENT_LOG_FORMAT log_fmt;
	EFI_PHYSICAL_ADDRESS start;
	EFI_PHYSICAL_ADDRESS end;
	BOOLEAN truncated;

	if (supported_logs & EFI_TCG2_EVENT_LOG_FORMAT_TCG_2)
		log_fmt = EFI_TCG2_EVENT_LOG_FORMAT_TCG_2;
	else
		log_fmt = EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2;

	return uefi_call_wrapper(tpm2->get_event_log, 5, tpm2, log_fmt,
				 &start, &end, &truncated);
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
		BOOLEAN old_caps;
		EFI_TCG2_EVENT *event;
		EFI_TCG2_BOOT_SERVICE_CAPABILITY caps;
		EFI_TCG2_EVENT_LOG_BITMAP supported_logs;

		status = tpm2_get_caps(tpm2, &caps, &old_caps);
		if (status != EFI_SUCCESS)
			return EFI_SUCCESS;

		if (!tpm2_present(&caps, old_caps))
			return EFI_SUCCESS;

		supported_logs = tpm2_get_supported_logs(tpm2, &caps, old_caps);

		status = trigger_tcg2_final_events_table(tpm2, supported_logs);
		if (EFI_ERROR(status)) {
			perror(L"Unable to trigger tcg2 final events table: %r\n", status);
			return status;
		}

		event = AllocatePool(sizeof(*event) + strlen(description) + 1);
		if (!event) {
			perror(L"Unable to allocate event structure\n");
			return EFI_OUT_OF_RESOURCES;
		}

		event->Header.HeaderSize = sizeof(EFI_TCG2_EVENT_HEADER);
		event->Header.HeaderVersion = 1;
		event->Header.PCRIndex = pcr;
		event->Header.EventType = EV_IPL;
		event->Size = sizeof(*event) - sizeof(event->Event) + strlen(description) + 1;
		memcpy(event->Event, description, strlen(description) + 1);
		status = uefi_call_wrapper(tpm2->hash_log_extend_event, 5, tpm2,
					   0, buf, (UINT64) size, event);
		FreePool(event);
		return status;
	} else {
		TCG_PCR_EVENT *event;
		UINT32 eventnum = 0;
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
		event->EventType = EV_IPL;
		event->EventSize = strlen(description) + 1;
		status = uefi_call_wrapper(tpm->log_extend_event, 7, tpm, buf,
					   (UINT64)size, TPM_ALG_SHA, event,
					   &eventnum, &lastevent);
		FreePool(event);
		return status;
	}

	return EFI_SUCCESS;
}
