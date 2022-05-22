// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SHIM_CC_H
#define SHIM_CC_H

typedef struct {
	uint8_t Major;
	uint8_t Minor;
} EFI_CC_VERSION;

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

#define TDX_MR_INDEX_MRTD  0
#define TDX_MR_INDEX_RTMR0 1
#define TDX_MR_INDEX_RTMR1 2
#define TDX_MR_INDEX_RTMR2 3
#define TDX_MR_INDEX_RTMR3 4

#define EFI_CC_EVENT_LOG_FORMAT_TCG_2 0x00000002
#define EFI_CC_BOOT_HASH_ALG_SHA384   0x00000004
#define EFI_CC_EVENT_HEADER_VERSION   1

typedef struct tdEFI_CC_EVENT_HEADER {
	uint32_t HeaderSize;
	uint16_t HeaderVersion;
	EFI_CC_MR_INDEX MrIndex;
	uint32_t EventType;
} __attribute__((packed)) EFI_CC_EVENT_HEADER;

typedef struct tdEFI_CC_EVENT {
	uint32_t Size;
	EFI_CC_EVENT_HEADER Header;
	uint8_t Event[1];
} __attribute__((packed)) EFI_CC_EVENT;

typedef struct tdEFI_CC_BOOT_SERVICE_CAPABILITY {
	uint8_t Size;
	EFI_CC_VERSION StructureVersion;
	EFI_CC_VERSION ProtocolVersion;
	EFI_CC_EVENT_ALGORITHM_BITMAP HashAlgorithmBitmap;
	EFI_CC_EVENT_LOG_BITMAP SupportedEventLogs;
	EFI_CC_TYPE CcType;
} EFI_CC_BOOT_SERVICE_CAPABILITY;

struct efi_cc_protocol
{
	EFI_STATUS (EFIAPI *get_capability) (
		struct efi_cc_protocol *this,
		EFI_CC_BOOT_SERVICE_CAPABILITY *ProtocolCapability);
	EFI_STATUS (EFIAPI *get_event_log) (
		struct efi_cc_protocol *this,
		EFI_CC_EVENT_LOG_FORMAT EventLogFormat,
		EFI_PHYSICAL_ADDRESS *EventLogLocation,
		EFI_PHYSICAL_ADDRESS *EventLogLastEntry,
		BOOLEAN *EventLogTruncated);
	EFI_STATUS (EFIAPI *hash_log_extend_event) (
		struct efi_cc_protocol *this,
		uint64_t Flags,
		EFI_PHYSICAL_ADDRESS DataToHash,
		uint64_t DataToHashLen,
		EFI_CC_EVENT *EfiCcEvent);
	EFI_STATUS (EFIAPI *map_pcr_to_mr_index) (
		struct efi_cc_protocol *this,
		uint32_t PcrIndex,
		EFI_CC_MR_INDEX *MrIndex);
};

typedef struct efi_cc_protocol efi_cc_protocol_t;

#define EFI_CC_FLAG_PE_COFF_IMAGE 0x0000000000000010

#endif /* SHIM_CC_H */
// vim:fenc=utf-8:tw=75
