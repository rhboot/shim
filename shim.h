#define SHIM_LOCK_GUID \
	{ 0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23} }

INTERFACE_DECL(_SHIM_LOCK);

typedef
EFI_STATUS
(EFIAPI *EFI_SHIM_LOCK_VERIFY) (
	IN VOID *buffer,
	IN UINT32 size
	);

typedef struct _SHIM_LOCK {
	EFI_SHIM_LOCK_VERIFY Verify;
} SHIM_LOCK;
