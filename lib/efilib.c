/*
 * Copyright 2018 Ruslan Nikolaev <nruslandev@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <efilib.h>
#include <Protocol/LoadedImage.h>

static EFI_MEMORY_TYPE PoolAllocationType = EfiBootServicesData;

EFI_SYSTEM_TABLE *gST;
EFI_BOOT_SERVICES *gBS;
EFI_RUNTIME_SERVICES *gRT;

UINTN StrLen (
	IN CONST CHAR16	*str
	)
{
	CONST CHAR16 *cur = str;
	while (*cur != '\0')
		cur++;
	return (UINTN) (cur - str);
}

UINTN StrSize (
	IN CONST CHAR16	*str
	)
{
	CONST CHAR16 *cur = str;
	while (*cur++ != '\0')
		;
	return (UINTN) ((CONST UINT8 *) cur - (CONST UINT8 *) str);
}

VOID StrCpy (
	IN CHAR16	*dst,
	IN CONST CHAR16	*src
	)
{
	while ((*dst++ = *src++) != '\0')
		;
}

VOID StrCat (
	IN CHAR16	*dst,
	IN CONST CHAR16	*src
	)
{
	while (*dst != '\0')
		dst++;
	while ((*dst++ = *src++) != '\0')
		;
}

INTN StrCmp (
	IN CONST CHAR16	*src1,
	IN CONST CHAR16	*src2
	)
{
	CONST UINT16 *s1 = (CONST UINT16 *) src1;
	CONST UINT16 *s2 = (CONST UINT16 *) src2;
	for (; *s1 != 0 && *s1 == *s2; s1++, s2++)
		;
	return (INTN) *s1 - (INTN) *s2;
}

INTN StrnCmp (
	IN CONST CHAR16	*src1,
	IN CONST CHAR16	*src2,
	IN UINTN	len
	)
{
	CONST UINT16 *s1 = (CONST UINT16 *) src1;
	CONST UINT16 *s2 = (CONST UINT16 *) src2;
	CONST UINT16 *s1e = s1 + len;
	for (; s1 != s1e; s1++, s2++) {
		if (*s1 == 0 || *s1 != *s2)
			return (INTN) *s1 - (INTN) *s2;
	}
	return 0;
}

CHAR16 *StrDuplicate (
	IN CONST CHAR16	*src
	)
{
	UINTN size = StrSize(src);
	CHAR16 *dup = (CHAR16 *) AllocatePool(size);
	if (dup != NULL)
		memcpy(dup, src, size);
	return dup;
}

UINTN strlena (
	IN CONST CHAR8	*str
	)
{
	CONST CHAR8 *cur = str;
	while (*cur != '\0')
		cur++;
	return (UINTN) (cur - str);
}

INTN strcmpa (
	IN CONST CHAR8	*src1,
	IN CONST CHAR8	*src2
	)
{
	CONST UINT8 *s1 = (CONST UINT8 *) src1;
	CONST UINT8 *s2 = (CONST UINT8 *) src2;
	for (; *s1 != 0 && *s1 == *s2; s1++, s2++)
		;
	return (INTN) *s1 - (INTN) *s2;
}

INTN strncmpa (
	IN CONST CHAR8	*src1,
	IN CONST CHAR8	*src2,
	IN UINTN	len
	)
{
	CONST UINT8 *s1 = (CONST UINT8 *) src1;
	CONST UINT8 *s2 = (CONST UINT8 *) src2;
	CONST UINT8 *s1e = s1 + len;
	for (; s1 != s1e; s1++, s2++) {
		if (*s1 == 0 || *s1 != *s2)
			return (INTN) *s1 - (INTN) *s2;
	}
	return 0;
}

VOID *AllocatePool (
	IN UINTN	size
	)
{
	VOID *ptr;
	EFI_STATUS ret = gBS->AllocatePool(PoolAllocationType, size, &ptr);
	if (EFI_ERROR(ret))
		return NULL;
	return ptr;
}

VOID *AllocateZeroPool (
	IN UINTN	size
	)
{
	VOID *ptr = AllocatePool(size);
	if (ptr != NULL)
		memset(ptr, 0, size);
	return ptr;
}

VOID FreePool (
	IN VOID		*buf
	)
{
	gBS->FreePool(buf);
}

VOID *ReallocatePool (
	IN VOID		*ptr,
	IN UINTN	psize,
	IN UINTN	nsize
	)
{
	VOID *nptr = AllocatePool(nsize);

	if (ptr != NULL && nptr != NULL) {
		memcpy(nptr, ptr, (nsize >= psize) ? psize : nsize);
		FreePool(ptr);
	}

	return nptr;
}

VOID InitializeLib (
	IN EFI_HANDLE		image,
	IN EFI_SYSTEM_TABLE	*st
	)
{
	/* This function is called in efi_main right away, so
	   clear the DF flag just in case, as a compiler can
	   potentially generate code which uses MOVS, STOS, etc */
#if defined(__i386__) || defined(__x86_64__)
	__asm__ __volatile__ ( "cld" : : : "cc" );
#endif

	gST = st;
	gBS = st->BootServices;
	gRT = st->RuntimeServices;

	/* Get PoolAllocationType from the image */
	if (image) {
		EFI_LOADED_IMAGE *li;
		EFI_STATUS ret = gBS->HandleProtocol(image,
					&gEfiLoadedImageProtocolGuid,
					(void **) &li);
		if (!EFI_ERROR(ret))
			PoolAllocationType = li->ImageDataType;
	}
}
