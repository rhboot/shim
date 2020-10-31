#pragma once

#include <efi.h>

extern EFI_SYSTEM_TABLE *gST;
extern EFI_BOOT_SERVICES *gBS;
extern EFI_RUNTIME_SERVICES *gRT;

void *memset(void *dst, int ch, UINTN len);
void *memcpy(void *dst, const void *src, UINTN len);
void *memmove(void *dst, const void *src, UINTN len);
int memcmp(const void *src1, const void *src2, UINTN len);

static inline INTN CompareMem (
	IN CONST VOID	*src1,
	IN CONST VOID	*src2,
	IN UINTN	len
	)
{
	return memcmp(src1, src2, len);
}

static inline INTN CompareGuid (
	IN CONST EFI_GUID	*guid1,
	IN CONST EFI_GUID	*guid2
	)
{
	return memcmp(guid1, guid2, sizeof(EFI_GUID));
}

static inline VOID SetMem (
	IN VOID		*dst,
	IN UINTN	len,
	IN UINT8	ch
	)
{
	memset(dst, ch, len);
}

static inline VOID ZeroMem (
	IN VOID		*buffer,
	IN UINTN	size
	)
{
	memset(buffer, 0, size);
}

static inline VOID CopyMem (
	IN VOID		*dst,
	IN CONST VOID	*src,
	IN UINTN	len
	)
{
	/* dst and src may overlap */
	memmove(dst, src, len);
}

UINTN StrLen (
	IN CONST CHAR16	*str
	);

UINTN StrSize (
	IN CONST CHAR16	*str
	);

VOID StrCpy (
	IN CHAR16	*dst,
	IN CONST CHAR16	*src
	);

VOID StrCat (
	IN CHAR16	*dst,
	IN CONST CHAR16	*src
	);

INTN StrCmp (
	IN CONST CHAR16	*src1,
	IN CONST CHAR16	*src2
	);

INTN StrnCmp (
	IN CONST CHAR16	*src1,
	IN CONST CHAR16	*src2,
	IN UINTN	len
	);

CHAR16 *StrDuplicate (
	IN CONST CHAR16	*src
	);

UINTN strlena (
	IN CONST CHAR8	*str
	);

INTN strcmpa (
	IN CONST CHAR8	*src1,
	IN CONST CHAR8	*src2
	);

INTN strncmpa (
	IN CONST CHAR8 *src1,
	IN CONST CHAR8 *src2,
	IN UINTN len
	);

VOID *AllocatePool (
	IN UINTN	size
	);

VOID FreePool (
	IN VOID		*buf
	);

VOID *AllocateZeroPool (
	IN UINTN	size
	);

VOID *ReallocatePool (
	IN VOID		*ptr,
	IN UINTN	psize,
	IN UINTN	nsize
	);

VOID InitializeLib (
	IN EFI_HANDLE		image,
	IN EFI_SYSTEM_TABLE	*st
	);

typedef struct _POOL_PRINT {
	CHAR16	*str;
	UINTN	max;
	UINTN	len;
} POOL_PRINT;

UINTN
VSPrint (OUT CHAR16	*buf,
	IN UINTN	bufSize,
	IN CONST CHAR16	*fmt,
	VA_LIST args);

UINTN
SPrint (OUT CHAR16	*buf,
	IN UINTN	bufSize,
	IN CONST CHAR16	*fmt,
	...);

CHAR16 *CatPrint (
	IN OUT POOL_PRINT	*buf,
	IN CONST CHAR16		*fmt,
	...);

CHAR16 *VPoolPrint (
	IN CONST CHAR16	*fmt,
	VA_LIST		args
	);

CHAR16 *PoolPrint (
	IN CONST CHAR16	*fmt,
	...
	);

UINTN
VPrint (IN CONST CHAR16 *fmt,
	VA_LIST args
	);

BOOLEAN
GrowBuffer (
	IN OUT EFI_STATUS	*status,
	IN OUT VOID		**buffer,
	IN UINTN		bufferSize
	);

EFI_STATUS
LibLocateProtocol (
	IN  EFI_GUID	*protocolGuid,
	OUT VOID	**interface
	);

EFI_STATUS
LibLocateHandle (
	IN EFI_LOCATE_SEARCH_TYPE	searchType,
	IN EFI_GUID			*protocol OPTIONAL,
	IN VOID				*searchKey OPTIONAL,
	IN OUT UINTN			*noHandles,
	OUT EFI_HANDLE			**buffer
	);

EFI_STATUS
WaitForSingleEvent (
	IN EFI_EVENT        event,
	IN UINT64           timeout OPTIONAL
	);

CONST CHAR16 *
GuidToString (
	OUT CHAR16	*buffer,
	IN EFI_GUID	*guid
	);

CONST CHAR16 *
StatusToString (
	OUT CHAR16	*buffer,
	IN EFI_STATUS	efi_status
	);

/* Used by Cryptlib. */
UINTN
console_print(const CHAR16 *fmt, ...);
UINTN
console_print_at(UINTN col, UINTN row, const CHAR16 *fmt, ...);
#define Print console_print
#define PrintAt console_print_at
