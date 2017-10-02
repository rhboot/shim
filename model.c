/*
 * model.c - modeling file for coverity
 * Copyright 2017 Peter Jones <pjones@redhat.com>
 *
 */

#ifndef __COVERITY__
/* This is so vim's Syntastic checker won't yell about all these. */
extern void __coverity_string_size_sanitize__(int);
extern void __coverity_negative_sink__(int);
extern void __coverity_alloc_nosize__(void);
extern void *__coverity_alloc__(int);
extern void __coverity_sleep__();
extern void __coverity_tainted_data_sanitize__(void *);
#endif

void *
OBJ_dup(void *o)
{
	__coverity_alloc_nosize__();
}

int
UTF8_getc(const unsigned char *str, int len, unsigned long *val)
{
	/* You can't quite express the right thing here, so instead we're
	 * telling covscan that if len is a certain value, the string has
	 * been checked for having a NUL at the right place.  Ideally what
	 * we'd tell it is it's never allowed to give us a string shorter
	 * than a certain length if certain bits (i.e. the UTF-8 surrogate
	 * length bits) are set. */
	if (len <= 0) {
		__coverity_string_size_sanitize__(0);
		return 0;
	} else if (len <= 6) {
		__coverity_string_size_sanitize__(0);
		return len;
	}
	return -2;
}

typedef unsigned long long u64;
typedef struct {
	unsigned long long hi;
	unsigned long long lo;
} u128;

void
gcm_gmult_4bit(u64 Xi[2], u128 Htable[16])
{
	__coverity_tainted_data_sanitize__(Htable);
}

void
msleep(int n)
{
	__coverity_sleep__();
}

/* From MdePkg/Include/Base.h or so */
typedef unsigned long long UINT64;
typedef unsigned long UINTN;
typedef long INTN;
typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINTN RETURN_STATUS;
typedef RETURN_STATUS EFI_STATUS;

#define MAX_BIT     (1ULL << (sizeof (INTN) * 8 - 1))
#define MAX_INTN   ((INTN)~MAX_BIT)

#define ENCODE_ERROR(StatusCode)     ((RETURN_STATUS)(MAX_BIT | (StatusCode)))
#define ENCODE_WARNING(StatusCode)   ((RETURN_STATUS)(StatusCode))
#define RETURN_ERROR(StatusCode)     (((INTN)(RETURN_STATUS)(StatusCode)) < 0)
#define RETURN_SUCCESS               0
#define RETURN_INVALID_PARAMETER     ENCODE_ERROR (2)
#define RETURN_OUT_OF_RESOURCES      ENCODE_ERROR (9)

/* From MdePkg/Include/Uefi/UefiBaseType.h */
#define EFI_SUCCESS               RETURN_SUCCESS
#define EFI_INVALID_PARAMETER     RETURN_INVALID_PARAMETER
#define EFI_OUT_OF_RESOURCES      RETURN_OUT_OF_RESOURCES

#define EFI_PAGE_MASK 0xFFF
#define EFI_PAGE_SHIFT 12
#define EFI_SIZE_TO_PAGES(a) (((a) >> EFI_PAGE_SHIFT) + (((a) & EFI_PAGE_MASK) ? 1 : 0))
#define EFI_PAGES_TO_SIZE(a) ((a) << EFI_PAGE_SHIFT)

/* From MdePkg/Include/Uefi/UefiMultiPhase.h */
typedef enum {
  EfiReservedMemoryType,
  EfiLoaderCode,
  EfiLoaderData,
  EfiBootServicesCode,
  EfiBootServicesData,
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiConventionalMemory,
  EfiUnusableMemory,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS,
  EfiMemoryMappedIO,
  EfiMemoryMappedIOPortSpace,
  EfiPalCode,
  EfiPersistentMemory,
  EfiMaxMemoryType
} EFI_MEMORY_TYPE;

/* From MdePkg/Include/Uefi/UefiSpec.h */
typedef enum {
	AllocateAnyPages,
	AllocateMaxAddress,
	AllocateAddress,
	MaxAllocateType
} EFI_ALLOCATE_TYPE;

EFI_STATUS
AllocatePages(EFI_ALLOCATE_TYPE Type,
	      EFI_MEMORY_TYPE MemoryType,
	      unsigned long Pages,
	      EFI_PHYSICAL_ADDRESS *Memory)
{
	int has_memory;
	unsigned long bytes = EFI_PAGES_TO_SIZE(Pages);

	if (Pages >= (unsigned long)((-1L) >> EFI_PAGE_SHIFT))
		return EFI_INVALID_PARAMETER;

	__coverity_negative_sink__(bytes);
	if (has_memory) {
		*Memory = (EFI_PHYSICAL_ADDRESS)__coverity_alloc__(bytes);
		return EFI_SUCCESS;
	}
	return EFI_OUT_OF_RESOURCES;
}

// vim:fenc=utf-8:tw=75
