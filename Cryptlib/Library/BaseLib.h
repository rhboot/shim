#include <efi.h>
#include <efilib.h>

UINT32 WriteUnaligned32 (UINT32 *Buffer, UINT32 Value);
UINTN AsciiStrSize (CHAR8 *string);
char *AsciiStrnCpy(char *Destination, char *Source, UINTN count);
char *AsciiStrCat(char *Destination, char *Source);
CHAR8 *AsciiStrCpy(CHAR8 *Destination, CHAR8 *Source);
UINTN AsciiStrDecimalToUintn(const char *String);
