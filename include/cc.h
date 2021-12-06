// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SHIM_CC_H
#define SHIM_CC_H

typedef struct {
	uint8_t Major;
	uint8_t Minor;
} EFI_CC_VERSION;

/*
 * EFI_CC Type/SubType definition
 */
#define EFI_CC_TYPE_NONE 0
#define EFI_CC_TYPE_SEV  1
#define EFI_CC_TYPE_TDX  2

typedef struct {
	uint8_t Type;
	uint8_t SubType;
} EFI_CC_TYPE;

typedef uint32_t EFI_CC_EVENT_LOG_BITMAP;
typedef uint32_t EFI_CC_EVENT_LOG_FORMAT;
typedef uint32_t EFI_CC_EVENT_ALGORITHM_BITMAP;
typedef uint32_t EFI_CC_MR_INDEX;

/*
 * Intel TDX measure register index
 */
#define TDX_MR_INDEX_MRTD  0
#define TDX_MR_INDEX_RTMR0 1
#define TDX_MR_INDEX_RTMR1 2
#define TDX_MR_INDEX_RTMR2 3
#define TDX_MR_INDEX_RTMR3 4

#define EFI_CC_EVENT_LOG_FORMAT_TCG_2 0x00000002
#define EFI_CC_BOOT_HASH_ALG_SHA384   0x00000004
#define EFI_CC_EVENT_HEADER_VERSION   1

typedef struct tdEFI_CC_EVENT_HEADER {
	/*
	 * Size of the event header itself (sizeof(EFI_CC_EVENT_HEADER))
	 */
	uint32_t HeaderSize;

	/*
	 * Header version. For this version of this specification,
	 * the value shall be 1.
	 */
	uint16_t HeaderVersion;

	/*
	 * Index of the MR that shall be extended
	 */
	EFI_CC_MR_INDEX MrIndex;

	/*
	 * Type of the event that shall be extended (and optionally logged)
	 */
	uint32_t EventType;
} __attribute__((packed)) EFI_CC_EVENT_HEADER;

typedef struct tdEFI_CC_EVENT {
	/*
	 * Total size of the event including the Size component, the header and the
	 * Event data.
	 */
	uint32_t Size;
	EFI_CC_EVENT_HEADER Header;
	uint8_t Event[1];
} __attribute__((packed)) EFI_CC_EVENT;

typedef struct {
	/* Allocated size of the structure */
	uint8_t Size;

	/*
	 * Version of the EFI_CC_BOOT_SERVICE_CAPABILITY structure itself.
	 * For this version of the protocol, the Major version shall be set to 1
	 * and the Minor version shall be set to 1.
	 */
	EFI_CC_VERSION StructureVersion;

	/*
	 * Version of the EFI TD protocol.
	 * For this version of the protocol, the Major version shall be set to 1
	 * and the Minor version shall be set to 1.
	 */
	EFI_CC_VERSION ProtocolVersion;

	/*
	 * Supported hash algorithms
	 */
	EFI_CC_EVENT_ALGORITHM_BITMAP HashAlgorithmBitmap;

	/*
	 * Bitmap of supported event log formats
	 */
	EFI_CC_EVENT_LOG_BITMAP SupportedEventLogs;

	/*
	 * Indicates the CC type
	 */
	EFI_CC_TYPE CcType;
} EFI_CC_BOOT_SERVICE_CAPABILITY;

struct efi_cc_protocol {
	EFI_STATUS(EFIAPI *get_capability)
	(struct efi_cc_protocol *this,
	 EFI_CC_BOOT_SERVICE_CAPABILITY *ProtocolCapability);
	EFI_STATUS(EFIAPI *get_event_log)
	(struct efi_cc_protocol *this,
	 EFI_CC_EVENT_LOG_FORMAT EventLogFormat,
	 EFI_PHYSICAL_ADDRESS *EventLogLocation,
	 EFI_PHYSICAL_ADDRESS *EventLogLastEntry, BOOLEAN *EventLogTruncated);
	EFI_STATUS(EFIAPI *hash_log_extend_event)
	(struct efi_cc_protocol *this, uint64_t Flags,
	 EFI_PHYSICAL_ADDRESS DataToHash, uint64_t DataToHashLen,
	 EFI_CC_EVENT *EfiCcEvent);
	EFI_STATUS(EFIAPI *map_pcr_to_mr_index)
	(struct efi_cc_protocol *this, uint32_t PcrIndex,
	 EFI_CC_MR_INDEX *MrIndex);
};

typedef struct efi_cc_protocol efi_cc_protocol_t;

EFI_STATUS cc_log_event_raw(EFI_PHYSICAL_ADDRESS buf, UINTN size, UINT8 pcr,
                            const CHAR8 *log, UINTN logsize, UINT32 type,
                            CHAR8 *hash);

#endif /* SHIM_CC_H */
// vim:fenc=utf-8:tw=75
