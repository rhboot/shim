/*
 * loader - UEFI shim transparent LoadImage()/StartImage() support
 *
 * Copyright (C) 2018 Michael Brown <mbrown@fensystems.co.uk>
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
#ifndef _LOADER_H
#define _LOADER_H

#include <efi.h>
#include <efiapi.h>

typedef struct _LOADER_PROTOCOL LOADER_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *LOADER_START_IMAGE)(
  IN  LOADER_PROTOCOL		*This,
  OUT UINTN			*ExitDataSize,
  OUT CHAR16			**ExitData	OPTIONAL
  );

typedef EFI_STATUS
(EFIAPI *LOADER_UNLOAD_IMAGE)(
  IN  LOADER_PROTOCOL *This
  );

typedef EFI_STATUS
(EFIAPI *LOADER_EXIT)(
  IN  LOADER_PROTOCOL		*This,
  IN  EFI_STATUS		ExitStatus,
  OUT UINTN			ExitDataSize,
  OUT CHAR16			*ExitData	OPTIONAL
  );

struct _LOADER_PROTOCOL {
  LOADER_START_IMAGE		StartImage;
  LOADER_UNLOAD_IMAGE		UnloadImage;
  LOADER_EXIT			Exit;
};

extern LOADER_PROTOCOL *loader_protocol(EFI_HANDLE image);
extern EFI_STATUS loader_install(EFI_SYSTEM_TABLE *systab,
				 EFI_HANDLE parent_image,
				 EFI_DEVICE_PATH *path,
				 VOID *buffer, UINTN size,
				 EFI_HANDLE *handle);

#endif /* _LOADER_H */
