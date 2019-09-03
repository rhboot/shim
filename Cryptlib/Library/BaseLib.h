#include <efi.h>
#include <efilib.h>

UINT32 WriteUnaligned32 (UINT32 *Buffer, UINT32 Value);
UINTN AsciiStrSize (const CHAR8 *string);
char *AsciiStrnCpy(char *Destination, const char *Source, UINTN count);
char *AsciiStrCat(char *Destination, const char *Source);
CHAR8 *AsciiStrCpy(CHAR8 *Destination, const CHAR8 *Source);
UINTN AsciiStrDecimalToUintn(const char *String);
