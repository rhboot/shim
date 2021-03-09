#if defined(__x86_64__)
/* shim.h will check if the compiler is new enough in some other CU */

#if !defined(GNU_EFI_USE_EXTERNAL_STDARG)
#define GNU_EFI_USE_EXTERNAL_STDARG
#endif

#if !defined(GNU_EFI_USE_MS_ABI)
#define GNU_EFI_USE_MS_ABI
#endif

#ifdef NO_BUILTIN_VA_FUNCS
#undef NO_BUILTIN_VA_FUNCS
#endif
#endif

#include <efi.h>
#include <efilib.h>

UINT32 WriteUnaligned32 (UINT32 *Buffer, UINT32 Value);
UINTN AsciiStrSize (const CHAR8 *string);
CHAR8 *AsciiStrnCpy(CHAR8 *Destination, const CHAR8 *Source, UINTN count);
CHAR8 *AsciiStrCat(CHAR8 *Destination, const CHAR8 *Source);
CHAR8 *AsciiStrCpy(CHAR8 *Destination, const CHAR8 *Source);
UINTN AsciiStrDecimalToUintn(const CHAR8 *String);
