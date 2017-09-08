#ifndef SHIM_H_
#define SHIM_H_

#include <efi.h>
#include <efilib.h>

#include "PeImage.h"

extern EFI_GUID SHIM_LOCK_GUID;

INTERFACE_DECL(_SHIM_LOCK);

typedef
EFI_STATUS
(*EFI_SHIM_LOCK_VERIFY) (
	IN VOID *buffer,
	IN UINT32 size
	);

typedef
EFI_STATUS
(*EFI_SHIM_LOCK_HASH) (
	IN char *data,
	IN int datasize,
	PE_COFF_LOADER_IMAGE_CONTEXT *context,
	UINT8 *sha256hash,
	UINT8 *sha1hash
	);

typedef
EFI_STATUS
(*EFI_SHIM_LOCK_CONTEXT) (
	IN VOID *data,
	IN unsigned int datasize,
	PE_COFF_LOADER_IMAGE_CONTEXT *context
	);

typedef struct _SHIM_LOCK {
	EFI_SHIM_LOCK_VERIFY Verify;
	EFI_SHIM_LOCK_HASH Hash;
	EFI_SHIM_LOCK_CONTEXT Context;
} SHIM_LOCK;

extern EFI_STATUS shim_init(void);
extern void shim_fini(void);
extern EFI_STATUS LogError(const char *file, int line, const char *func, CHAR16 *fmt, ...);
extern EFI_STATUS VLogError(const char *file, int line, const char *func, CHAR16 *fmt, va_list args);
extern VOID PrintErrors(VOID);
extern VOID ClearErrors(VOID);

#ifdef __x86_64__
#ifndef DEFAULT_LOADER
#define DEFAULT_LOADER L"\\grubx64.efi"
#endif
#ifndef DEFAULT_LOADER_CHAR
#define DEFAULT_LOADER_CHAR "\\grubx64.efi"
#endif
#ifndef EFI_ARCH
#define EFI_ARCH L"x64"
#endif
#ifndef DEBUGDIR
#define DEBUGDIR L"/usr/lub/debug/usr/share/shim/x64/"
#endif
#endif

#if defined(__i686__) || defined(__i386__)
#ifndef DEFAULT_LOADER
#define DEFAULT_LOADER L"\\grubia32.efi"
#endif
#ifndef DEFAULT_LOADER_CHAR
#define DEFAULT_LOADER_CHAR "\\grubia32.efi"
#endif
#ifndef EFI_ARCH
#define EFI_ARCH L"ia32"
#endif
#ifndef DEBUGDIR
#define DEBUGDIR L"/usr/lub/debug/usr/share/shim/ia32/"
#endif
#endif

#if defined(__aarch64__)
#ifndef DEFAULT_LOADER
#define DEFAULT_LOADER L"\\grubaa64.efi"
#endif
#ifndef DEFAULT_LOADER_CHAR
#define DEFAULT_LOADER_CHAR "\\grubaa64.efi"
#endif
#ifndef EFI_ARCH
#define EFI_ARCH L"aa64"
#endif
#ifndef DEBUGDIR
#define DEBUGDIR L"/usr/lub/debug/usr/share/shim/aa64/"
#endif
#endif

#if defined(__arm__)
#ifndef DEFAULT_LOADER
#define DEFAULT_LOADER L"\\grubarm.efi"
#endif
#ifndef DEFAULT_LOADER_CHAR
#define DEFAULT_LOADER_CHAR "\\grubarm.efi"
#endif
#ifndef EFI_ARCH
#define EFI_ARCH L"arm"
#endif
#ifndef DEBUGDIR
#define DEBUGDIR L"/usr/lub/debug/usr/share/shim/arm/"
#endif
#endif

#include "netboot.h"
#include "httpboot.h"
#include "replacements.h"
#include "tpm.h"
#include "ucs2.h"

#include "guid.h"
#include "variables.h"
#include "efiauthenticated.h"
#include "security_policy.h"
#include "console.h"
#include "version.h"

#ifdef ENABLE_SHIM_CERT
#include "shim_cert.h"
#endif

#define LogError(fmt, ...) LogError(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)

#endif /* SHIM_H_ */
