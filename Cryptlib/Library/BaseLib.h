#include <efi.h>
#include <efilib.h>

UINT32 WriteUnaligned32 (UINT32 *Buffer, UINT32 Value);
UINTN AsciiStrSize (CHAR8 *string);
char *AsciiStrnCpy(char *Destination, const char *Source, UINTN count);
char *AsciiStrCat(char *Destination, char *Source);
char *AsciiStrCpy(char *Destination, const char *Source);
UINTN AsciiStrDecimalToUintn(const char *String);
