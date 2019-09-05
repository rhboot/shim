#ifndef __BASE_LIB_H__
#define __BASE_LIB_H__

#include <efi.h>
#include <efilib.h>

UINT32 WriteUnaligned32 (UINT32 *Buffer, UINT32 Value);
UINTN AsciiStrSize (CONST CHAR8 *string);
RETURN_STATUS AsciiStrnCpyS (CHAR8 *Destination, UINTN DestMax, CONST CHAR8 *Source, UINTN Length);
RETURN_STATUS AsciiStrnSizeS(CONST CHAR8 *String, UINTN MaxSize);
RETURN_STATUS AsciiStrnLenS (CONST CHAR8 *String, UINTN MaxSize);
CHAR8 *AsciiStrnCpy(CHAR8 *Destination, CONST CHAR8 *Source, UINTN count);
RETURN_STATUS AsciiStrCpyS (CHAR8 *Destination, UINTN DestMax, CONST CHAR8 *Source);
CHAR8 *AsciiStrCat(CHAR8 *Destination, CONST CHAR8 *Source);
RETURN_STATUS AsciiStrCatS(CHAR8 *Destination, UINTN DestMax, CONST CHAR8 *Source);
CHAR8 *AsciiStrCpy(CHAR8 *Destination, CONST CHAR8 *Source);
UINTN AsciiStrDecimalToUintn(CONST CHAR8 *String);

#endif /* __BASE_LIB_H__ */
