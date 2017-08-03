#include <efi.h>
#include <efilib.h>
#include <string.h>
#include <stdint.h>

#include "tpm.h"

extern UINT8 in_protocol;

#define perror(fmt, ...) ({                                             \
			UINTN __perror_ret = 0;                               \
			if (!in_protocol)                                     \
				__perror_ret = Print((fmt), ##__VA_ARGS__);   \
			__perror_ret;                                         \
		})


typedef struct {
	CHAR16 *VariableName;
	EFI_GUID *VendorGuid;
	VOID *Data;
	UINTN Size;
} VARIABLE_RECORD;

UINTN measuredcount = 0;
VARIABLE_RECORD *measureddata = NULL;

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

static EFI_STATUS tpm_locate_protocol(efi_tpm_protocol_t **tpm,
				      efi_tpm2_protocol_t **tpm2,
				      BOOLEAN *old_caps_p,
				      EFI_TCG2_BOOT_SERVICE_CAPABILITY *capsp)
{
	EFI_STATUS status;

	*tpm = NULL;
	*tpm2 = NULL;
	status = LibLocateProtocol(&tpm2_guid, (VOID **)tpm2);
	/* TPM 2.0 */
	if (status == EFI_SUCCESS) {
		BOOLEAN old_caps;
		EFI_TCG2_BOOT_SERVICE_CAPABILITY caps;

		status = tpm2_get_caps(*tpm2, &caps, &old_caps);
		if (EFI_ERROR(status))
			return status;

		if (tpm2_present(&caps, old_caps)) {
			if (old_caps_p)
				*old_caps_p = old_caps;
			if (capsp)
				memcpy(capsp, &caps, sizeof(caps));
			return EFI_SUCCESS;
		}
	} else {
		status = LibLocateProtocol(&tpm_guid, (VOID **)tpm);
		if (EFI_ERROR(status))
			return status;

		if (tpm_present(*tpm))
			return EFI_SUCCESS;
	}

	return EFI_NOT_FOUND;
}

static EFI_STATUS tpm_log_event_raw(EFI_PHYSICAL_ADDRESS buf, UINTN size,
				    UINT8 pcr, const CHAR8 *log, UINTN logsize,
				    UINT32 type, CHAR8 *hash)
{
	EFI_STATUS status;
	efi_tpm_protocol_t *tpm;
	efi_tpm2_protocol_t *tpm2;
	BOOLEAN old_caps;
	EFI_TCG2_BOOT_SERVICE_CAPABILITY caps;

	status = tpm_locate_protocol(&tpm, &tpm2, &old_caps, &caps);
	if (EFI_ERROR(status)) {
		return status;
	} else if (tpm2) {
		EFI_TCG2_EVENT *event;
		EFI_TCG2_EVENT_LOG_BITMAP supported_logs;

		supported_logs = tpm2_get_supported_logs(tpm2, &caps, old_caps);

		status = trigger_tcg2_final_events_table(tpm2, supported_logs);
		if (EFI_ERROR(status)) {
			perror(L"Unable to trigger tcg2 final events table: %r\n", status);
			return status;
		}

		event = AllocatePool(sizeof(*event) + logsize);
		if (!event) {
			perror(L"Unable to allocate event structure\n");
			return EFI_OUT_OF_RESOURCES;
		}

		event->Header.HeaderSize = sizeof(EFI_TCG2_EVENT_HEADER);
		event->Header.HeaderVersion = 1;
		event->Header.PCRIndex = pcr;
		event->Header.EventType = type;
		event->Size = sizeof(*event) - sizeof(event->Event) + logsize + 1;
		CopyMem(event->Event, (VOID *)log, logsize);
		if (hash) {
			/* TPM 2 systems will generate the appropriate hash
			   themselves if we pass PE_COFF_IMAGE
			*/
			status = uefi_call_wrapper(tpm2->hash_log_extend_event,
						   5, tpm2, PE_COFF_IMAGE, buf,
						   (UINT64) size, event);
		} else {
			status = uefi_call_wrapper(tpm2->hash_log_extend_event,
						   5, tpm2, 0, buf,
						   (UINT64) size, event);
		}
		FreePool(event);
		return status;
	} else if (tpm) {
		TCG_PCR_EVENT *event;
		UINT32 eventnum = 0;
		EFI_PHYSICAL_ADDRESS lastevent;

		status = LibLocateProtocol(&tpm_guid, (VOID **)&tpm);

		if (status != EFI_SUCCESS)
			return EFI_SUCCESS;

		if (!tpm_present(tpm))
			return EFI_SUCCESS;

		event = AllocatePool(sizeof(*event) + logsize);

		if (!event) {
			perror(L"Unable to allocate event structure\n");
			return EFI_OUT_OF_RESOURCES;
		}

		event->PCRIndex = pcr;
		event->EventType = type;
		event->EventSize = logsize;
		CopyMem(event->Event, (VOID *)log, logsize);
		if (hash) {
			/* TPM 1.2 devices require us to pass the Authenticode
			   hash rather than allowing the firmware to attempt
			   to calculate it */
			CopyMem(event->digest, hash, sizeof(event->digest));
			status = uefi_call_wrapper(tpm->log_extend_event, 7,
						   tpm, 0, 0, TPM_ALG_SHA,
						   event, &eventnum,
						   &lastevent);
		} else {
			status = uefi_call_wrapper(tpm->log_extend_event, 7,
						   tpm, buf, (UINT64)size,
						   TPM_ALG_SHA, event,
						   &eventnum, &lastevent);
		}
		FreePool(event);
		return status;
	}

	return EFI_SUCCESS;
}

EFI_STATUS tpm_log_event(EFI_PHYSICAL_ADDRESS buf, UINTN size, UINT8 pcr,
			 const CHAR8 *description)
{
	return tpm_log_event_raw(buf, size, pcr, description,
				 strlen(description) + 1, 0xd, NULL);
}

EFI_STATUS tpm_log_pe(EFI_PHYSICAL_ADDRESS buf, UINTN size, UINT8 *sha1hash,
		      UINT8 pcr)
{
	EFI_IMAGE_LOAD_EVENT ImageLoad;

	// All of this is informational and forces us to do more parsing before
	// we can generate it, so let's just leave it out for now
	ImageLoad.ImageLocationInMemory = 0;
	ImageLoad.ImageLengthInMemory = 0;
	ImageLoad.ImageLinkTimeAddress = 0;
	ImageLoad.LengthOfDevicePath = 0;

	return tpm_log_event_raw(buf, size, pcr, (CHAR8 *)&ImageLoad,
				 sizeof(ImageLoad),
				 EV_EFI_BOOT_SERVICES_APPLICATION, sha1hash);
}

typedef struct {
	EFI_GUID VariableName;
	UINT64 UnicodeNameLength;
	UINT64 VariableDataLength;
	CHAR16 UnicodeName[1];
	INT8 VariableData[1];
} EFI_VARIABLE_DATA_TREE;

static BOOLEAN tpm_data_measured(CHAR16 *VarName, EFI_GUID VendorGuid, UINTN VarSize, VOID *VarData)
{
	UINTN i;

	for (i=0; i<measuredcount; i++) {
		if ((StrCmp (VarName, measureddata[i].VariableName) == 0) &&
		    (CompareGuid (&VendorGuid, measureddata[i].VendorGuid)) &&
		    (VarSize == measureddata[i].Size) &&
		    (CompareMem (VarData, measureddata[i].Data, VarSize) == 0)) {
			return TRUE;
		}
	}

	return FALSE;
}

static EFI_STATUS tpm_record_data_measurement(CHAR16 *VarName, EFI_GUID VendorGuid, UINTN VarSize, VOID *VarData)
{
	if (measureddata == NULL) {
		measureddata = AllocatePool(sizeof(*measureddata));
	} else {
		measureddata = ReallocatePool(measureddata, measuredcount * sizeof(*measureddata),
					      (measuredcount + 1) * sizeof(*measureddata));
	}

	if (measureddata == NULL)
		return EFI_OUT_OF_RESOURCES;

	measureddata[measuredcount].VariableName = AllocatePool(StrSize(VarName));
	measureddata[measuredcount].VendorGuid = AllocatePool(sizeof(EFI_GUID));
	measureddata[measuredcount].Data = AllocatePool(VarSize);

	if (measureddata[measuredcount].VariableName == NULL ||
	    measureddata[measuredcount].VendorGuid == NULL ||
	    measureddata[measuredcount].Data == NULL) {
		return EFI_OUT_OF_RESOURCES;
	}

	StrCpy(measureddata[measuredcount].VariableName, VarName);
	CopyMem(measureddata[measuredcount].VendorGuid, &VendorGuid, sizeof(EFI_GUID));
	CopyMem(measureddata[measuredcount].Data, VarData, VarSize);
	measureddata[measuredcount].Size = VarSize;
	measuredcount++;

	return EFI_SUCCESS;
}

EFI_STATUS tpm_measure_variable(CHAR16 *VarName, EFI_GUID VendorGuid, UINTN VarSize, VOID *VarData)
{
	EFI_STATUS Status;
	UINTN VarNameLength;
	EFI_VARIABLE_DATA_TREE *VarLog;
	UINT32 VarLogSize;

	/* Don't measure something that we've already measured */
	if (tpm_data_measured(VarName, VendorGuid, VarSize, VarData))
		return EFI_SUCCESS;

	VarNameLength = StrLen (VarName);
	VarLogSize = (UINT32)(sizeof (*VarLog) +
			      VarNameLength * sizeof (*VarName) +
			      VarSize -
			      sizeof (VarLog->UnicodeName) -
			      sizeof (VarLog->VariableData));

	VarLog = (EFI_VARIABLE_DATA_TREE *) AllocateZeroPool (VarLogSize);
	if (VarLog == NULL) {
		return EFI_OUT_OF_RESOURCES;
	}

	CopyMem (&VarLog->VariableName, &VendorGuid,
		 sizeof(VarLog->VariableName));
	VarLog->UnicodeNameLength  = VarNameLength;
	VarLog->VariableDataLength = VarSize;
	CopyMem (VarLog->UnicodeName, VarName,
		 VarNameLength * sizeof (*VarName));
	CopyMem ((CHAR16 *)VarLog->UnicodeName + VarNameLength, VarData,
		 VarSize);

	Status = tpm_log_event_raw((EFI_PHYSICAL_ADDRESS)(intptr_t)VarLog,
				   VarLogSize, 7, (CHAR8 *)VarLog, VarLogSize,
				   EV_EFI_VARIABLE_AUTHORITY, NULL);

	FreePool(VarLog);

	if (Status != EFI_SUCCESS)
		return Status;

	return tpm_record_data_measurement(VarName, VendorGuid, VarSize,
					   VarData);
}

EFI_STATUS
fallback_should_prefer_reset(void)
{
	EFI_STATUS status;
	efi_tpm_protocol_t *tpm;
	efi_tpm2_protocol_t *tpm2;

	status = tpm_locate_protocol(&tpm, &tpm2, NULL, NULL);
	if (EFI_ERROR(status))
		return EFI_NOT_FOUND;
	return EFI_SUCCESS;
}
