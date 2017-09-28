#ifndef SHIM_TPM_H
#define SHIM_TPM_H

#include <efilib.h>

#define TPM_ALG_SHA 0x00000004
#define EV_IPL      0x0000000d

EFI_STATUS tpm_log_event(EFI_PHYSICAL_ADDRESS buf, UINTN size, UINT8 pcr,
			 const CHAR8 *description);
EFI_STATUS fallback_should_prefer_reset(void);

EFI_STATUS tpm_log_pe(EFI_PHYSICAL_ADDRESS buf, UINTN size, UINT8 *sha1hash,
		      UINT8 pcr);

EFI_STATUS tpm_measure_variable(CHAR16 *dbname, EFI_GUID guid, UINTN size, void *data);

typedef struct {
  uint8_t Major;
  uint8_t Minor;
  uint8_t RevMajor;
  uint8_t RevMinor;
} TCG_VERSION;

typedef struct _TCG_EFI_BOOT_SERVICE_CAPABILITY {
  uint8_t          Size;                /// Size of this structure.
  TCG_VERSION    StructureVersion;
  TCG_VERSION    ProtocolSpecVersion;
  uint8_t          HashAlgorithmBitmap; /// Hash algorithms .
  char        TPMPresentFlag;      /// 00h = TPM not present.
  char        TPMDeactivatedFlag;  /// 01h = TPM currently deactivated.
} TCG_EFI_BOOT_SERVICE_CAPABILITY;

typedef struct _TCG_PCR_EVENT {
  uint32_t PCRIndex;
  uint32_t EventType;
  uint8_t digest[20];
  uint32_t EventSize;
  uint8_t  Event[1];
} TCG_PCR_EVENT;

typedef struct _EFI_IMAGE_LOAD_EVENT {
  EFI_PHYSICAL_ADDRESS ImageLocationInMemory;
  UINTN ImageLengthInMemory;
  UINTN ImageLinkTimeAddress;
  UINTN LengthOfDevicePath;
  EFI_DEVICE_PATH DevicePath[1];
} EFI_IMAGE_LOAD_EVENT;

struct efi_tpm_protocol
{
  EFI_STATUS (EFIAPI *status_check) (struct efi_tpm_protocol *this,
				     TCG_EFI_BOOT_SERVICE_CAPABILITY *ProtocolCapability,
				     uint32_t *TCGFeatureFlags,
				     EFI_PHYSICAL_ADDRESS *EventLogLocation,
				     EFI_PHYSICAL_ADDRESS *EventLogLastEntry);
  EFI_STATUS (EFIAPI *hash_all) (struct efi_tpm_protocol *this,
				 uint8_t *HashData,
				 uint64_t HashLen,
				 uint32_t AlgorithmId,
				 uint64_t *HashedDataLen,
				 uint8_t **HashedDataResult);
  EFI_STATUS (EFIAPI *log_event) (struct efi_tpm_protocol *this,
				  TCG_PCR_EVENT *TCGLogData,
				  uint32_t *EventNumber,
				  uint32_t Flags);
  EFI_STATUS (EFIAPI *pass_through_to_tpm) (struct efi_tpm_protocol *this,
					    uint32_t TpmInputParameterBlockSize,
					    uint8_t *TpmInputParameterBlock,
					    uint32_t TpmOutputParameterBlockSize,
					    uint8_t *TpmOutputParameterBlock);
  EFI_STATUS (EFIAPI *log_extend_event) (struct efi_tpm_protocol *this,
					 EFI_PHYSICAL_ADDRESS HashData,
					 uint64_t HashDataLen,
					 uint32_t AlgorithmId,
					 TCG_PCR_EVENT *TCGLogData,
					 uint32_t *EventNumber,
					 EFI_PHYSICAL_ADDRESS *EventLogLastEntry);
};

typedef struct efi_tpm_protocol efi_tpm_protocol_t;

typedef uint32_t TREE_EVENT_LOG_BITMAP;

typedef uint32_t EFI_TCG2_EVENT_LOG_BITMAP;
typedef uint32_t EFI_TCG2_EVENT_LOG_FORMAT;
typedef uint32_t EFI_TCG2_EVENT_ALGORITHM_BITMAP;

typedef struct tdTREE_VERSION {
  uint8_t Major;
  uint8_t Minor;
} TREE_VERSION;

typedef struct tdEFI_TCG2_VERSION {
  uint8_t Major;
  uint8_t Minor;
} EFI_TCG2_VERSION;

typedef struct tdTREE_BOOT_SERVICE_CAPABILITY {
  uint8_t Size;
  TREE_VERSION StructureVersion;
  TREE_VERSION ProtocolVersion;
  uint32_t HashAlgorithmBitmap;
  TREE_EVENT_LOG_BITMAP SupportedEventLogs;
  BOOLEAN TrEEPresentFlag;
  uint16_t MaxCommandSize;
  uint16_t MaxResponseSize;
  uint32_t ManufacturerID;
} TREE_BOOT_SERVICE_CAPABILITY;

typedef struct tdEFI_TCG2_BOOT_SERVICE_CAPABILITY {
  uint8_t Size;
  EFI_TCG2_VERSION StructureVersion;
  EFI_TCG2_VERSION ProtocolVersion;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP HashAlgorithmBitmap;
  EFI_TCG2_EVENT_LOG_BITMAP SupportedEventLogs;
  BOOLEAN TPMPresentFlag;
  uint16_t MaxCommandSize;
  uint16_t MaxResponseSize;
  uint32_t ManufacturerID;
  uint32_t NumberOfPcrBanks;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP ActivePcrBanks;
} EFI_TCG2_BOOT_SERVICE_CAPABILITY;

typedef uint32_t TCG_PCRINDEX;
typedef uint32_t TCG_EVENTTYPE;

typedef struct tdEFI_TCG2_EVENT_HEADER {
  uint32_t HeaderSize;
  uint16_t HeaderVersion;
  TCG_PCRINDEX PCRIndex;
  TCG_EVENTTYPE EventType;
} __attribute__ ((packed)) EFI_TCG2_EVENT_HEADER;

typedef struct tdEFI_TCG2_EVENT {
  uint32_t Size;
  EFI_TCG2_EVENT_HEADER Header;
  uint8_t Event[1];
} __attribute__ ((packed)) EFI_TCG2_EVENT;

#define EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2 0x00000001
#define EFI_TCG2_EVENT_LOG_FORMAT_TCG_2   0x00000002

struct efi_tpm2_protocol
{
  EFI_STATUS (EFIAPI *get_capability) (struct efi_tpm2_protocol *this,
				       EFI_TCG2_BOOT_SERVICE_CAPABILITY *ProtocolCapability);
  EFI_STATUS (EFIAPI *get_event_log) (struct efi_tpm2_protocol *this,
				      EFI_TCG2_EVENT_LOG_FORMAT EventLogFormat,
				      EFI_PHYSICAL_ADDRESS *EventLogLocation,
				      EFI_PHYSICAL_ADDRESS *EventLogLastEntry,
				      BOOLEAN *EventLogTruncated);
  EFI_STATUS (EFIAPI *hash_log_extend_event) (struct efi_tpm2_protocol *this,
					      uint64_t Flags,
					      EFI_PHYSICAL_ADDRESS DataToHash,
					      uint64_t DataToHashLen,
					      EFI_TCG2_EVENT *EfiTcgEvent);
  EFI_STATUS (EFIAPI *submit_command) (struct efi_tpm2_protocol *this,
				       uint32_t InputParameterBlockSize,
				       uint8_t *InputParameterBlock,
				       uint32_t OutputParameterBlockSize,
				       uint8_t *OutputParameterBlock);
  EFI_STATUS (EFIAPI *get_active_pcr_blanks) (struct efi_tpm2_protocol *this,
					      uint32_t *ActivePcrBanks);
  EFI_STATUS (EFIAPI *set_active_pcr_banks) (struct efi_tpm2_protocol *this,
					     uint32_t ActivePcrBanks);
  EFI_STATUS (EFIAPI *get_result_of_set_active_pcr_banks) (struct efi_tpm2_protocol *this,
							   uint32_t *OperationPresent,
							   uint32_t *Response);
};

typedef struct efi_tpm2_protocol efi_tpm2_protocol_t;

typedef UINT32                     TCG_EVENTTYPE;

#define EV_EFI_EVENT_BASE                   ((TCG_EVENTTYPE) 0x80000000)
#define EV_EFI_VARIABLE_DRIVER_CONFIG       (EV_EFI_EVENT_BASE + 1)
#define EV_EFI_VARIABLE_BOOT                (EV_EFI_EVENT_BASE + 2)
#define EV_EFI_BOOT_SERVICES_APPLICATION    (EV_EFI_EVENT_BASE + 3)
#define EV_EFI_BOOT_SERVICES_DRIVER         (EV_EFI_EVENT_BASE + 4)
#define EV_EFI_RUNTIME_SERVICES_DRIVER      (EV_EFI_EVENT_BASE + 5)
#define EV_EFI_GPT_EVENT                    (EV_EFI_EVENT_BASE + 6)
#define EV_EFI_ACTION                       (EV_EFI_EVENT_BASE + 7)
#define EV_EFI_PLATFORM_FIRMWARE_BLOB       (EV_EFI_EVENT_BASE + 8)
#define EV_EFI_HANDOFF_TABLES               (EV_EFI_EVENT_BASE + 9)
#define EV_EFI_VARIABLE_AUTHORITY           (EV_EFI_EVENT_BASE + 0xE0)

#define PE_COFF_IMAGE 0x0000000000000010

#endif /* SHIM_TPM_H */
// vim:fenc=utf-8:tw=75
