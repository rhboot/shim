#ifndef __BASE_LIB_H__
#define __BASE_LIB_H__

#include "Base.h"

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

UINT64 EFIAPI AsmReadTsc (VOID);
UINT64 EFIAPI AsmReadMsr64 (UINT32 Index);
VOID EFIAPI CpuPause (VOID);
UINT64 EFIAPI MultU64x64 (UINT64 Multiplicand, UINT64 Multiplier);

#pragma pack (1)
typedef struct {
  UINT16  Limit;
  UINTN   Base;
} IA32_DESCRIPTOR;
#pragma pack ()

///
/// Byte packed structure for an FP/SSE/SSE2 context.
///
typedef struct {
  UINT8  Buffer[512];
} IA32_FX_BUFFER;

#endif /* __BASE_LIB_H__ */
