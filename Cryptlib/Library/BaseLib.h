#include <efi.h>
#include <efilib.h>

UINT32 WriteUnaligned32 (UINT32 *Buffer, UINT32 Value);
UINTN AsciiStrSize (const CHAR8 *string);
CHAR8 *AsciiStrnCpy(CHAR8 *Destination, const CHAR8 *Source, UINTN count);
CHAR8 *AsciiStrCat(CHAR8 *Destination, const CHAR8 *Source);
CHAR8 *AsciiStrCpy(CHAR8 *Destination, const CHAR8 *Source);
UINTN AsciiStrDecimalToUintn(const CHAR8 *String);
