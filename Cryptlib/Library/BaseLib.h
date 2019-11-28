#include <efi.h>
#include <efilib.h>

UINT32 WriteUnaligned32 (UINT32 *Buffer, UINT32 Value);
UINTN AsciiStrSize (const char *string);
char *AsciiStrnCpy(char *Destination, const char *Source, UINTN count);
char *AsciiStrCat(char *Destination, const char *Source);
char *AsciiStrCpy(char *Destination, const char *Source);
UINTN AsciiStrDecimalToUintn(const char *String);
