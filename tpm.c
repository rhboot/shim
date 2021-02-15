// SPDX-License-Identifier: BSD-2-Clause-Patent

#include <efi.h>
#include <efilib.h>
#include <string.h>
#include <stdint.h>

#include "shim.h"

typedef struct {
	CHAR16 *VariableName;
	EFI_GUID *VendorGuid;
	VOID *Data;
	UINTN Size;
} VARIABLE_RECORD;

UINTN measuredcount = 0;
VARIABLE_RECORD *measureddata = NULL;

static BOOLEAN tpm_present(efi_tpm_protocol_t *tpm)
{
	EFI_STATUS efi_status;
	TCG_EFI_BOOT_SERVICE_CAPABILITY caps;
	UINT32 flags;
	EFI_PHYSICAL_ADDRESS eventlog, lastevent;

	caps.Size = (UINT8)sizeof(caps);
	efi_status = tpm->status_check(tpm, &caps, &flags,
				       &eventlog, &lastevent);
	if (EFI_ERROR(efi_status) ||
	    caps.TPMDeactivatedFlag || !caps.TPMPresentFlag)
		return FALSE;

	return TRUE;
}

static EFI_STATUS tpm2_get_caps(efi_tpm2_protocol_t *tpm,
				EFI_TCG2_BOOT_SERVICE_CAPABILITY *caps,
				BOOLEAN *old_caps)
{
	EFI_STATUS efi_status;

	caps->Size = (UINT8)sizeof(*caps);

	efi_status = tpm->get_capability(tpm, caps);
	if (EFI_ERROR(efi_status))
		return efi_status;

	if (caps->StructureVersion.Major == 1 &&
	    caps->StructureVersion.Minor == 0)
		*old_caps = TRUE;
	else
		*old_caps = FALSE;

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

static EFI_STATUS tpm_locate_protocol(efi_tpm_protocol_t **tpm,
				      efi_tpm2_protocol_t **tpm2,
				      BOOLEAN *old_caps_p,
				      EFI_TCG2_BOOT_SERVICE_CAPABILITY *capsp)
{
	EFI_STATUS efi_status;

	*tpm = NULL;
	*tpm2 = NULL;
	efi_status = LibLocateProtocol(&EFI_TPM2_GUID, (VOID **)tpm2);
	/* TPM 2.0 */
	if (!EFI_ERROR(efi_status)) {
		BOOLEAN old_caps;
		EFI_TCG2_BOOT_SERVICE_CAPABILITY caps;

		efi_status = tpm2_get_caps(*tpm2, &caps, &old_caps);
		if (EFI_ERROR(efi_status))
			return efi_status;

		if (tpm2_present(&caps, old_caps)) {
			if (old_caps_p)
				*old_caps_p = old_caps;
			if (capsp)
				memcpy(capsp, &caps, sizeof(caps));
			return EFI_SUCCESS;
		}
	} else {
		efi_status = LibLocateProtocol(&EFI_TPM_GUID, (VOID **)tpm);
		if (EFI_ERROR(efi_status))
			return efi_status;

		if (tpm_present(*tpm))
			return EFI_SUCCESS;
	}

	return EFI_NOT_FOUND;
}

static EFI_STATUS tpm_log_event_raw(EFI_PHYSICAL_ADDRESS buf, UINTN size,
				    UINT8 pcr, const CHAR8 *log, UINTN logsize,
				    UINT32 type, CHAR8 *hash)
{
	EFI_STATUS efi_status;
	efi_tpm_protocol_t *tpm;
	efi_tpm2_protocol_t *tpm2;
	BOOLEAN old_caps;
	EFI_TCG2_BOOT_SERVICE_CAPABILITY caps;

	efi_status = tpm_locate_protocol(&tpm, &tpm2, &old_caps, &caps);
	if (EFI_ERROR(efi_status)) {
#ifdef REQUIRE_TPM
		perror(L"TPM logging failed: %r\n", efi_status);
		return efi_status;
#else
		if (efi_status != EFI_NOT_FOUND) {
			perror(L"TPM logging failed: %r\n", efi_status);
			return efi_status;
		}
#endif
	} else if (tpm2) {
		EFI_TCG2_EVENT *event;
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
			/* TPM 2 systems will generate the appropriate hash
			   themselves if we pass PE_COFF_IMAGE.  In case that
			   fails we fall back to measuring without it.
			*/
			efi_status = tpm2->hash_log_extend_event(tpm2,
				PE_COFF_IMAGE, buf, (UINT64) size, event);
		}

	        if (!hash || EFI_ERROR(efi_status)) {
			efi_status = tpm2->hash_log_extend_event(tpm2,
				0, buf, (UINT64) size, event);
		}
		FreePool(event);
		return efi_status;
	} else if (tpm) {
		TCG_PCR_EVENT *event;
		UINT32 eventnum = 0;
		EFI_PHYSICAL_ADDRESS lastevent;

		efi_status = LibLocateProtocol(&EFI_TPM_GUID, (VOID **)&tpm);
		if (EFI_ERROR(efi_status))
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
			efi_status = tpm->log_extend_event(tpm, 0, 0,
				TPM_ALG_SHA, event, &eventnum, &lastevent);
		} else {
			efi_status = tpm->log_extend_event(tpm, buf,
				(UINT64)size, TPM_ALG_SHA, event, &eventnum,
				&lastevent);
		}
		FreePool(event);
		return efi_status;
	}

	return EFI_SUCCESS;
}

EFI_STATUS tpm_log_event(EFI_PHYSICAL_ADDRESS buf, UINTN size, UINT8 pcr,
			 const CHAR8 *description)
{
	return tpm_log_event_raw(buf, size, pcr, description,
				 strlen(description) + 1, 0xd, NULL);
}

EFI_STATUS tpm_log_pe(EFI_PHYSICAL_ADDRESS buf, UINTN size,
		      EFI_PHYSICAL_ADDRESS addr, EFI_DEVICE_PATH *path,
		      UINT8 *sha1hash, UINT8 pcr)
{
	EFI_IMAGE_LOAD_EVENT *ImageLoad = NULL;
	EFI_STATUS efi_status;
	UINTN path_size = 0;

	if (path)
		path_size = DevicePathSize(path);

	ImageLoad = AllocateZeroPool(sizeof(*ImageLoad) + path_size);
	if (!ImageLoad) {
		perror(L"Unable to allocate image load event structure\n");
		return EFI_OUT_OF_RESOURCES;
	}

	ImageLoad->ImageLocationInMemory = buf;
	ImageLoad->ImageLengthInMemory = size;
	ImageLoad->ImageLinkTimeAddress = addr;

	if (path_size > 0) {
		CopyMem(ImageLoad->DevicePath, path, path_size);
		ImageLoad->LengthOfDevicePath = path_size;
	}

	efi_status = tpm_log_event_raw(buf, size, pcr, (CHAR8 *)ImageLoad,
				       sizeof(*ImageLoad) + path_size,
				       EV_EFI_BOOT_SERVICES_APPLICATION,
				       (CHAR8 *)sha1hash);
	FreePool(ImageLoad);

	return efi_status;
}

typedef struct {
	EFI_GUID VariableName;
	UINT64 UnicodeNameLength;
	UINT64 VariableDataLength;
	CHAR16 UnicodeName[1];
	INT8 VariableData[1];
} __attribute__ ((packed)) EFI_VARIABLE_DATA_TREE;

static BOOLEAN tpm_data_measured(CHAR16 *VarName, EFI_GUID VendorGuid, UINTN VarSize, VOID *VarData)
{
	UINTN i;

	for (i=0; i<measuredcount; i++) {
		if ((StrCmp (VarName, measureddata[i].VariableName) == 0) &&
		    (CompareGuid (&VendorGuid, measureddata[i].VendorGuid) == 0) &&
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
	EFI_STATUS efi_status;
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

	efi_status = tpm_log_event_raw((EFI_PHYSICAL_ADDRESS)(intptr_t)VarLog,
				       VarLogSize, 7, (CHAR8 *)VarLog, VarLogSize,
				       EV_EFI_VARIABLE_AUTHORITY, NULL);

	FreePool(VarLog);

	if (EFI_ERROR(efi_status))
		return efi_status;

	return tpm_record_data_measurement(VarName, VendorGuid, VarSize,
					   VarData);
}

EFI_STATUS
fallback_should_prefer_reset(void)
{
	EFI_STATUS efi_status;
	efi_tpm_protocol_t *tpm;
	efi_tpm2_protocol_t *tpm2;

	efi_status = tpm_locate_protocol(&tpm, &tpm2, NULL, NULL);
	if (EFI_ERROR(efi_status))
		return EFI_NOT_FOUND;
	return EFI_SUCCESS;
}
