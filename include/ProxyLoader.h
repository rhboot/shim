/** @file
  Proxy loader protocol definition.

**/

#ifndef __PROXY_LOADER_PROTOCOL_H__
#define __PROXY_LOADER_PROTOCOL_H__

#define PROXY_LOADER_PROTOCOL_GUID \
  { \
  0x6766FF22, 0xE98C, 0x47D2, {0x9B, 0x7A, 0xD3, 0x27, 0x73, 0xD9, 0x80, 0xBB} \
  }

typedef struct _PROXY_LOADER_PROTOCOL PROXY_LOADER_PROTOCOL;

/**
  Image entry point.

  @param  ImageHandle		The firmware allocated handle for the EFI image.
  @param  SystemTable		A pointer to the EFI System Table.

  @retval Status		Function execution status.
**/
typedef
EFI_STATUS
(EFIAPI *PROXY_LOADER_ENTRY)(
  IN EFI_HANDLE			ImageHandle,
  IN EFI_SYSTEM_TABLE		*SystemTable
  );

struct _PROXY_LOADER_PROTOCOL {
  PROXY_LOADER_ENTRY		Entry;
};

extern EFI_GUID gProxyLoaderProtocolGuid;

#endif
