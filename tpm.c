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

#if defined ENABLE_PCRCHECKS
EFI_STATUS get_tpm2_pcr(efi_tpm2_protocol_t *tpm2, uint32_t pcr, TPM_ALG_ID Algorithm, TPM_PCR_DIGEST *TPM_PCR_Data){
	EFI_STATUS efi_status;

    TPM2_PCR_READ_COMMAND	SendBuffer;
    TPM2_PCR_READ_RESPONSE	RecvBuffer;
    uint32_t				SendBufferSize = 0;
    uint32_t				RecvBufferSize = 0;
    uint32_t                count = 1;

	//Setting up Buffer Sizes
	SendBufferSize = sizeof(TPM2_PCR_READ_COMMAND);
    RecvBufferSize = sizeof(TPM2_PCR_READ_RESPONSE);

    //Setting up SendBuffer Header
    SendBuffer.Header.tag = swap_uint16(TPM_ST_NO_SESSIONS);
    SendBuffer.Header.commandCode = swap_uint32(TPM_CC_PCR_Read);

    //Setting up PcrSelectionIn for Sendbuffer
    SendBuffer.pcrSelectionIn.count = swap_uint32(count);

    SendBuffer.pcrSelectionIn.pcrSelections[0].hash = swap_uint16(Algorithm);
    SendBuffer.pcrSelectionIn.pcrSelections[0].sizeofSelect = 3;
	SendBuffer.pcrSelectionIn.pcrSelections[0].pcrSelect[0] = 0; //Fixes unitialized error.
    SendBuffer.pcrSelectionIn.pcrSelections[0].pcrSelect[((pcr)/8)] |= (1 << ((pcr) % 8));
   
    //Buffer size initialization
    SendBuffer.Header.paramSize  = swap_uint32(SendBufferSize);

    efi_status = tpm2->submit_command(tpm2 ,SendBufferSize, (uint8_t *)&SendBuffer, RecvBufferSize, (uint8_t *)&RecvBuffer);

    /*****************************************ERROR_CHECHING*****************************************/
    if (EFI_ERROR(efi_status)){
        perror(L"-TPM2 ERROR: SubmitCommand failed [%d]\n", efi_status);
        msleep(2000000);
        return efi_status;
    }

    if (RecvBufferSize < sizeof (TPM2_RESPONSE_HEADER)){
        perror(L"-TPM2 ERROR: RecvBufferSize [%x]\n", RecvBufferSize);
        msleep(2000000);
        return EFI_BUFFER_TOO_SMALL;
    }

    if (swap_uint32(RecvBuffer.Header.responseCode) != TPM_RC_SUCCESS){
        perror(L"-TPM2 ERROR: Tpm2 Responce Code [%x]\n", swap_uint32(RecvBuffer.Header.responseCode));
        msleep(2000000);
        return EFI_OUT_OF_RESOURCES;
    }

    //PCR selection check
    if (!(RecvBuffer.pcrSelectionOut.pcrSelections[0].pcrSelect[((pcr) / 8)] & (1 << ((pcr) % 8)))){
        perror(L"-TPM2 ERROR: PCR Selection and pcrSelect does not match\n");
		return EFI_PROTOCOL_ERROR;
	}
    /*****************************************ERROR_CHECHING*****************************************/

	TPM_PCR_Data->PCR = pcr;
	TPM_PCR_Data->DIGEST_SIZE = swap_uint16(RecvBuffer.pcrValues.digests[0].size);
	TPM_PCR_Data->hashAlg = swap_uint16(RecvBuffer.pcrSelectionOut.pcrSelections[0].hash);
	CopyMem(TPM_PCR_Data->buffer, RecvBuffer.pcrValues.digests[0].buffer, swap_uint16(RecvBuffer.pcrValues.digests[0].size));

	/*********************************************DEBUG*********************************************/
#ifdef ENABLE_PCRCHECKS_DEBUG
	EFI_INPUT_KEY key;
	console_print(L"\n\n-TPM2.0: Return Code: [%r]\n", swap_uint32(RecvBuffer.Header.responseCode));
    console_print(L"-TPM2.0: PCR Selection = [%d]; HashAlg = [0x%08x]; Digest Size = [%d]\n", TPM_PCR_Data->PCR, TPM_PCR_Data->hashAlg, swap_uint16(RecvBuffer.pcrValues.digests[0].size));
    hexdump_old(RecvBuffer.pcrValues.digests[0].buffer, swap_uint16(RecvBuffer.pcrValues.digests[0].size));
    console_print(L"Press any key...\n");
    console_get_keystroke(&key);
#endif
	/*********************************************DEBUG*********************************************/

	SetMem(&RecvBuffer, 0, (UINT8)sizeof(RecvBuffer));
    return EFI_SUCCESS;
}

EFI_STATUS get_tpm_pcr(efi_tpm_protocol_t *tpm, uint32_t PCRIndex, TPM_PCR_DIGEST *TPM_PCR_Data){
	EFI_STATUS efi_status;

	TPM_COMMAND SendBuffer;
	TPM_RESPONSE RecvBuffer;
	uint32_t			SendBufferSize = 0;
	uint32_t			RecvBufferSize = 0;

	//Setting up Buffer Sizes
	SendBufferSize = sizeof(TPM_COMMAND);
	RecvBufferSize = sizeof(TPM_RESPONSE);

	SendBuffer.Header.tag 		= swap_uint16(TPM_TAG_RQU_COMMAND);
	SendBuffer.Header.paramSize = swap_uint32(SendBufferSize);
	SendBuffer.Header.ordinal	= swap_uint32(TPM_ORD_PcrRead);
	SendBuffer.PCRRequested		= swap_uint32(PCRIndex);

	efi_status = tpm->pass_through_to_tpm(tpm, SendBufferSize, (UINT8 *)&SendBuffer, RecvBufferSize, (UINT8 *)&RecvBuffer);
	
    /*****************************************ERROR_CHECHING*****************************************/
	if (EFI_ERROR(efi_status)) {
		perror(L"-ERROR: PassThroughToTpm failed [%r]\n", efi_status);
		return efi_status;
	}

	if ((RecvBuffer.Header.tag != swap_uint16(TPM_TAG_RSP_COMMAND)) || (RecvBuffer.Header.returnCode != 0)) {
		perror(L"-ERROR: TPM command result [%r]\n", swap_uint16(RecvBuffer.Header.returnCode));
		return EFI_DEVICE_ERROR;
	}
    /*****************************************ERROR_CHECHING*****************************************/

	TPM_PCR_Data->PCR = PCRIndex;
	TPM_PCR_Data->DIGEST_SIZE = TPM_SHA1_160_HASH_LEN;
	TPM_PCR_Data->hashAlg = TPM_ALG_SHA1;
	CopyMem(TPM_PCR_Data->buffer, RecvBuffer.pcrValue.digest, TPM_SHA1_160_HASH_LEN);

	/*********************************************DEBUG*********************************************/
#ifdef ENABLE_PCRCHECKS_DEBUG
	EFI_INPUT_KEY key;
	console_print(L"\n\n-TPM1.2: Return Code: [%r]\n", RecvBuffer.Header.returnCode);
    console_print(L"-TPM1.2: PCR Selection = [%d]; HashAlg = [0x%08x]; Digest Size = [%d]\n", TPM_PCR_Data->PCR, TPM_PCR_Data->hashAlg, TPM_PCR_Data->DIGEST_SIZE);
    hexdump_old(RecvBuffer.pcrValue.digest, TPM_SHA1_160_HASH_LEN);
    console_print(L"Press any key...\n");
    console_get_keystroke(&key);
#endif
	/*********************************************DEBUG*********************************************/

	SetMem(&RecvBuffer, 0, (UINT8)sizeof(RecvBuffer));
    return EFI_SUCCESS;
}

EFI_STATUS tpm2_error(TPM_PCR_DIGEST *PCR_Data){
#ifdef ENABLE_PCRCHECKS_DEBUG
	EFI_INPUT_KEY key;
	console_print(L"\n-TPM2.0: Comparison: [ERROR] - Invalid PCR Value in file\n");
	console_print(L"-TPM2.0: PCR Selection = [%d]; HashAlg = [0x%08x]; Digest Size = [%d]\n", PCR_Data->PCR, PCR_Data->hashAlg, PCR_Data->DIGEST_SIZE),
	hexdump_old(PCR_Data->buffer, PCR_Data->DIGEST_SIZE);
	console_get_keystroke(&key);
	return EFI_SUCCESS;
#else
	perror(L"-Invalid PCR Value [%d] in config: [%r]\n", PCR_Data->PCR ,EFI_SECURITY_VIOLATION);
	return EFI_SECURITY_VIOLATION;
#endif
}

EFI_STATUS tpm_error(TPM_PCR_DIGEST *PCR_Data){
#ifdef ENABLE_PCRCHECKS_DEBUG
	EFI_INPUT_KEY key;
	console_print(L"\n-TPM1.2: Comparison: [ERROR] - Invalid PCR Value in file\n");
	console_print(L"-TPM1.2: PCR Selection = [%d]; HashAlg = [0x%08x]; Digest Size = [%d]\n", PCR_Data->PCR, PCR_Data->hashAlg, PCR_Data->DIGEST_SIZE),
	hexdump_old(PCR_Data->buffer, PCR_Data->DIGEST_SIZE);
	console_get_keystroke(&key);
	return EFI_SUCCESS;
#else
	perror(L"-Invalid PCR Value [%d] in config: [%r]\n", PCR_Data->PCR ,EFI_SECURITY_VIOLATION);
	return EFI_SECURITY_VIOLATION;
#endif
}

EFI_STATUS tpm_check_pcr(TPM_PCR_DIGEST *PCR_Data, uint32_t PCR_Data_Size){
	EFI_STATUS efi_status;
	TPM_PCR_DIGEST *TPM_PCR_Data	= NULL;
	TPM_PCR_DIGEST *TPM_PCR_Data2	= NULL;
	efi_tpm_protocol_t *tpm;
	efi_tpm2_protocol_t *tpm2;
	BOOLEAN old_caps;
	EFI_TCG2_BOOT_SERVICE_CAPABILITY caps;

	TPM_PCR_Data  = AllocatePool(sizeof(TPM_PCR_DIGEST));
	TPM_PCR_Data2 = AllocatePool(sizeof(TPM_PCR_DIGEST));

	efi_status = tpm_locate_protocol(&tpm, &tpm2, &old_caps, &caps);
	if (EFI_ERROR(efi_status)) {
#ifdef REQUIRE_TPM
		perror(L"TPM logging failed: [%r]\n", efi_status);
		return efi_status;
#else
		if (efi_status != EFI_NOT_FOUND) {
			perror(L"TPM logging failed: [%r]\n", efi_status);
			return efi_status;
		}
#endif
	} else if (tpm2) {

        uint32_t temp = 0;
		BOOL VALID = false;
		/*****************************************TPM2_PCR_Checks*****************************************/
        while(temp < PCR_Data_Size){
			//If not at the last PCR in the struct
			if(temp < PCR_Data_Size-1){
				//If there are multiple same PCR values for a PCR
				if(PCR_Data[temp].PCR == PCR_Data[temp+1].PCR){
					//While there are multiple PCR Values for a PCR
					while(PCR_Data[temp].PCR == PCR_Data[temp+1].PCR && temp < PCR_Data_Size-1){
						efi_status = get_tpm2_pcr(tpm2, PCR_Data[temp].PCR, PCR_Data[temp].hashAlg, TPM_PCR_Data);
						if(EFI_ERROR(efi_status)){
							return efi_status;
						}
#ifdef ENABLE_PCRCHECKS_DEBUG
						if(CompareMem(PCR_Data[temp].buffer, TPM_PCR_Data->buffer, PCR_Data[temp].DIGEST_SIZE))
								tpm2_error(&PCR_Data[temp]);
#endif
						//Must due two checks at once to make the loops check the last value
						efi_status = get_tpm2_pcr(tpm2, PCR_Data[temp+1].PCR, PCR_Data[temp+1].hashAlg, TPM_PCR_Data2);
						if(EFI_ERROR(efi_status)){
							return efi_status;
						}
#ifdef ENABLE_PCRCHECKS_DEBUG
						if(CompareMem(PCR_Data[temp+1].buffer, TPM_PCR_Data2->buffer, PCR_Data[temp+1].DIGEST_SIZE))
								tpm2_error(&PCR_Data[temp+1]);
#endif
						//If one of the values are correct then there is a valid value
						if(!CompareMem(PCR_Data[temp].buffer, TPM_PCR_Data->buffer, PCR_Data[temp].DIGEST_SIZE) || !CompareMem(PCR_Data[temp+1].buffer, TPM_PCR_Data2->buffer, PCR_Data[temp+1].DIGEST_SIZE)){
							VALID = TRUE;
						}
						temp++;
					}
					//If there are no valid values then KILL IT
					if(!VALID){
#ifndef ENABLE_PCRCHECKS_DEBUG
						if (tpm2_error(&PCR_Data[temp]) == EFI_SECURITY_VIOLATION){
							return EFI_SECURITY_VIOLATION;
						}
#endif
					}
					//Resets Valid to false.
					VALID = false;
					temp++;
				}
				//If there are not multiple same PCR values
				//A normal check
				else{
					efi_status = get_tpm2_pcr(tpm2, PCR_Data[temp].PCR, PCR_Data[temp].hashAlg, TPM_PCR_Data);
					if(EFI_ERROR(efi_status)){
						return efi_status;
					}

					if(CompareMem(PCR_Data[temp].buffer, TPM_PCR_Data->buffer, PCR_Data[temp].DIGEST_SIZE) || TPM_PCR_Data->DIGEST_SIZE != PCR_Data[temp].DIGEST_SIZE || PCR_Data[temp].hashAlg != TPM_PCR_Data->hashAlg){
						if (tpm2_error(&PCR_Data[temp]) == EFI_SECURITY_VIOLATION){
							return EFI_SECURITY_VIOLATION;
						}
					}
					 temp++;
				}	
			}
			//This check takes care of the off cases where there is only one type of pcr at the end of the list
			else{
				efi_status = get_tpm2_pcr(tpm2, PCR_Data[temp].PCR, PCR_Data[temp].hashAlg, TPM_PCR_Data);
				if(EFI_ERROR(efi_status)){
					return efi_status;
				}

				if(CompareMem(PCR_Data[temp].buffer, TPM_PCR_Data->buffer, PCR_Data[temp].DIGEST_SIZE) || TPM_PCR_Data->DIGEST_SIZE != PCR_Data[temp].DIGEST_SIZE || PCR_Data[temp].hashAlg != TPM_PCR_Data->hashAlg){
					if (tpm2_error(&PCR_Data[temp]) == EFI_SECURITY_VIOLATION){
						return EFI_SECURITY_VIOLATION;
					}
				}
				 temp++;
			}	
        }
		/*****************************************TPM2_PCR_Checks*****************************************/

		//Cleanup
		FreePool(TPM_PCR_Data);
		FreePool(TPM_PCR_Data2);
		return efi_status;
	} else if (tpm) {
        uint32_t temp = 0;
		BOOL VALID = false;
		/*****************************************TPM_PCR_Checks*****************************************/
        while(temp < PCR_Data_Size){
			//If not at the last PCR in the struct
			if(temp < PCR_Data_Size-1){
				//If there are multiple same PCR values for a PCR
				if(PCR_Data[temp].PCR == PCR_Data[temp+1].PCR){
					//While there are multiple PCR Values for a PCR
					while(PCR_Data[temp].PCR == PCR_Data[temp+1].PCR && temp < PCR_Data_Size-1){
						efi_status = get_tpm_pcr(tpm, PCR_Data[temp].PCR, TPM_PCR_Data);
						if(EFI_ERROR(efi_status)){
							return efi_status;
						}
#ifdef ENABLE_PCRCHECKS_DEBUG
						if(CompareMem(PCR_Data[temp].buffer, TPM_PCR_Data->buffer, PCR_Data[temp].DIGEST_SIZE))
								tpm_error(&PCR_Data[temp]);
#endif

						//Must due two checks at once to make the loops check the last value
						efi_status = get_tpm_pcr(tpm, PCR_Data[temp+1].PCR, TPM_PCR_Data2);
						if(EFI_ERROR(efi_status)){
							return efi_status;
						}
#ifdef ENABLE_PCRCHECKS_DEBUG
						if(CompareMem(PCR_Data[temp+1].buffer, TPM_PCR_Data2->buffer, PCR_Data[temp+1].DIGEST_SIZE))
								tpm_error(&PCR_Data[temp+1]);
#endif
						//If one of the values are correct then there is a valid value
						if(!CompareMem(PCR_Data[temp].buffer, TPM_PCR_Data->buffer, PCR_Data[temp].DIGEST_SIZE) || !CompareMem(PCR_Data[temp+1].buffer, TPM_PCR_Data2->buffer, PCR_Data[temp+1].DIGEST_SIZE)){
							VALID = TRUE;
						}
						temp++;
					}
					//If there are no valid values then KILL IT
					if(!VALID){
#ifndef ENABLE_PCRCHECKS_DEBUG
						if (tpm_error(&PCR_Data[temp]) == EFI_SECURITY_VIOLATION){
							return EFI_SECURITY_VIOLATION;
						}
#endif
					}
					//Resets Valid to false.
					VALID = false;
					temp++;
				}
				//If there are not multiple same PCR values
				//A normal check
				else{
					efi_status = get_tpm_pcr(tpm, PCR_Data[temp].PCR, TPM_PCR_Data);
					if(EFI_ERROR(efi_status)){
						return efi_status;
					}

					if(CompareMem(PCR_Data[temp].buffer, TPM_PCR_Data->buffer, PCR_Data[temp].DIGEST_SIZE) || TPM_PCR_Data->DIGEST_SIZE != PCR_Data[temp].DIGEST_SIZE || PCR_Data[temp].hashAlg != TPM_PCR_Data->hashAlg){
						if (tpm_error(&PCR_Data[temp]) == EFI_SECURITY_VIOLATION){
							return EFI_SECURITY_VIOLATION;
						}
					}
					 temp++;
				}	
			}
			//This check takes care of the off cases where there is only one type of pcr at the end of the list
			else{
				efi_status = get_tpm_pcr(tpm, PCR_Data[temp].PCR, TPM_PCR_Data);
				if(EFI_ERROR(efi_status)){
					return efi_status;
				}

				if(CompareMem(PCR_Data[temp].buffer, TPM_PCR_Data->buffer, PCR_Data[temp].DIGEST_SIZE) || TPM_PCR_Data->DIGEST_SIZE != PCR_Data[temp].DIGEST_SIZE || PCR_Data[temp].hashAlg != TPM_PCR_Data->hashAlg){
					if (tpm_error(&PCR_Data[temp]) == EFI_SECURITY_VIOLATION){
						return EFI_SECURITY_VIOLATION;
					}
				}
				 temp++;
			}	
        }
		/*****************************************TPM_PCR_Checks*****************************************/

		//Cleanup
		FreePool(TPM_PCR_Data);
		FreePool(TPM_PCR_Data2);

		return efi_status;
	}

	return EFI_SUCCESS;
}
#endif
