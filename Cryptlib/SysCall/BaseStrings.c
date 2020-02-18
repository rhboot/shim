#include <CrtLibSupport.h>

#define ASCII_RSIZE_MAX  1000000ul
#define RSIZE_MAX  (ASCII_RSIZE_MAX<<1ul)

#define SAFE_STRING_CONSTRAINT_CHECK(Expression, Status)  \
  do { \
    ASSERT (Expression); \
    if (!(Expression)) { \
      return Status; \
    } \
  } while (FALSE)

/**
  Returns if 2 memory blocks are overlapped.

  @param  Base1  Base address of 1st memory block.
  @param  Size1  Size of 1st memory block.
  @param  Base2  Base address of 2nd memory block.
  @param  Size2  Size of 2nd memory block.

  @retval TRUE  2 memory blocks are overlapped.
  @retval FALSE 2 memory blocks are not overlapped.
**/
BOOLEAN
InternalSafeStringIsOverlap (
  IN VOID    *Base1,
  IN UINTN   Size1,
  IN VOID    *Base2,
  IN UINTN   Size2
  )
{
  if ((((UINTN)Base1 >= (UINTN)Base2) && ((UINTN)Base1 < (UINTN)Base2 + Size2)) ||
      (((UINTN)Base2 >= (UINTN)Base1) && ((UINTN)Base2 < (UINTN)Base1 + Size1))) {
    return TRUE;
  }
  return FALSE;
}

/**
  Returns if 2 Unicode strings are not overlapped.

  @param  Str1   Start address of 1st Unicode string.
  @param  Size1  The number of char in 1st Unicode string,
                 including terminating null char.
  @param  Str2   Start address of 2nd Unicode string.
  @param  Size2  The number of char in 2nd Unicode string,
                 including terminating null char.

  @retval TRUE  2 Unicode strings are NOT overlapped.
  @retval FALSE 2 Unicode strings are overlapped.
**/
BOOLEAN
InternalSafeStringNoStrOverlap (
  IN CHAR16  *Str1,
  IN UINTN   Size1,
  IN CHAR16  *Str2,
  IN UINTN   Size2
  )
{
  return !InternalSafeStringIsOverlap (Str1, Size1 * sizeof(CHAR16), Str2, Size2 * sizeof(CHAR16));
}

/**
  Returns if 2 Ascii strings are not overlapped.

  @param  Str1   Start address of 1st Ascii string.
  @param  Size1  The number of char in 1st Ascii string,
                 including terminating null char.
  @param  Str2   Start address of 2nd Ascii string.
  @param  Size2  The number of char in 2nd Ascii string,
                 including terminating null char.

  @retval TRUE  2 Ascii strings are NOT overlapped.
  @retval FALSE 2 Ascii strings are overlapped.
**/
BOOLEAN
InternalSafeStringNoAsciiStrOverlap (
  IN CHAR8   *Str1,
  IN UINTN   Size1,
  IN CHAR8   *Str2,
  IN UINTN   Size2
  )
{
  return !InternalSafeStringIsOverlap (Str1, Size1, Str2, Size2);
}

/**
  Returns the length of a Null-terminated Ascii string.

  This function is similar as strlen_s defined in C11.

  @param  String   A pointer to a Null-terminated Ascii string.
  @param  MaxSize  The maximum number of Destination Ascii
                   char, including terminating null char.

  @retval 0        If String is NULL.
  @retval MaxSize  If there is no null character in the first MaxSize characters of String.
  @return The number of characters that percede the terminating null character.

**/
UINTN
AsciiStrnLenS (
  IN CONST CHAR8               *String,
  IN UINTN                     MaxSize
  )
{
  UINTN                             Length;

  //
  // If String is a null pointer or MaxSize is 0, then the AsciiStrnLenS function returns zero.
  //
  if ((String == NULL) || (MaxSize == 0)) {
    return 0;
  }

  //
  // Otherwise, the AsciiStrnLenS function returns the number of characters that precede the
  // terminating null character. If there is no null character in the first MaxSize characters of
  // String then AsciiStrnLenS returns MaxSize. At most the first MaxSize characters of String shall
  // be accessed by AsciiStrnLenS.
  //
  Length = 0;
  while (String[Length] != 0) {
    if (Length >= MaxSize - 1) {
      return MaxSize;
    }
    Length++;
  }
  return Length;
}

/**
  Returns the size of a Null-terminated Ascii string in bytes, including the
  Null terminator.

  This function returns the size of the Null-terminated Ascii string specified
  by String in bytes, including the Null terminator.

  @param  String   A pointer to a Null-terminated Ascii string.
  @param  MaxSize  The maximum number of Destination Ascii
                   char, including the Null terminator.

  @retval 0  If String is NULL.
  @retval (sizeof (CHAR8) * (MaxSize + 1))
             If there is no Null terminator in the first MaxSize characters of
             String.
  @return The size of the Null-terminated Ascii string in bytes, including the
          Null terminator.

**/
UINTN
AsciiStrnSizeS (
  IN CONST CHAR8               *String,
  IN UINTN                     MaxSize
  )
{
  //
  // If String is a null pointer, then the AsciiStrnSizeS function returns
  // zero.
  //
  if (String == NULL) {
    return 0;
  }

  //
  // Otherwise, the AsciiStrnSizeS function returns the size of the
  // Null-terminated Ascii string in bytes, including the Null terminator. If
  // there is no Null terminator in the first MaxSize characters of String,
  // then AsciiStrnSizeS returns (sizeof (CHAR8) * (MaxSize + 1)) to keep a
  // consistent map with the AsciiStrnLenS function.
  //
  return (AsciiStrnLenS (String, MaxSize) + 1) * sizeof (*String);
}

/**
  Copies the string pointed to by Source (including the terminating null char)
  to the array pointed to by Destination.

  This function is similar as strcpy_s defined in C11.

  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
AsciiStrCpyS (
  OUT CHAR8        *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR8  *Source
  )
{
  UINTN            SourceLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than ASCII_RSIZE_MAX.
  //
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than AsciiStrnLenS(Source, DestMax).
  //
  SourceLen = AsciiStrnLenS (Source, DestMax);
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrCpyS function copies the string pointed to by Source (including the terminating
  // null character) into the array pointed to by Destination.
  //
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Copies not more than Length successive char from the string pointed to by
  Source to the array pointed to by Destination. If no null char is copied from
  Source, then Destination[Length] is always set to null.

  This function is similar as strncpy_s defined in C11.

  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.
  @param  Length                   The maximum number of Ascii characters to copy.

  @retval RETURN_SUCCESS           String is copied.
  @retval RETURN_BUFFER_TOO_SMALL  If DestMax is NOT greater than
                                   MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
AsciiStrnCpyS (
  OUT CHAR8        *Destination,
  IN  UINTN        DestMax,
  IN  CONST CHAR8  *Source,
  IN  UINTN        Length
  )
{
  UINTN            SourceLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than ASCII_RSIZE_MAX
  //
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. If Length is not less than DestMax, then DestMax shall be greater than AsciiStrnLenS(Source, DestMax).
  //
  SourceLen = AsciiStrnLenS (Source, MIN (DestMax, Length));
  if (Length >= DestMax) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);
  }

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrnCpyS function copies not more than Length successive characters (characters that
  // follow a null character are not copied) from the array pointed to by Source to the array
  // pointed to by Destination. If no null character was copied from Source, then Destination[Length] is set to a null
  // character.
  //
  while ((SourceLen > 0) && (*Source != 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends a copy of the string pointed to by Source (including the terminating
  null char) to the end of the string pointed to by Destination.

  This function is similar as strcat_s defined in C11.

  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than StrLen(Source).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
AsciiStrCatS (
  IN OUT CHAR8        *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR8  *Source
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;

  //
  // Let CopyLen denote the value DestMax - AsciiStrnLenS(Destination, DestMax) upon entry to AsciiStrCatS.
  //
  DestLen = AsciiStrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than ASCII_RSIZE_MAX.
  //
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. CopyLen shall be greater than AsciiStrnLenS(Source, CopyLen).
  //
  SourceLen = AsciiStrnLenS (Source, CopyLen);
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 6. Copying shall not take place between objects that overlap.
  //
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrCatS function appends a copy of the string pointed to by Source (including the
  // terminating null character) to the end of the string pointed to by Destination. The initial character
  // from Source overwrites the null character at the end of Destination.
  //
  Destination = Destination + DestLen;
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

/**
  Appends not more than Length successive char from the string pointed to by
  Source to the end of the string pointed to by Destination. If no null char is
  copied from Source, then Destination[StrLen(Destination) + Length] is always
  set to null.

  This function is similar as strncat_s defined in C11.

  If an error would be returned, then the function will also ASSERT().

  If an error is returned, then the Destination is unmodified.

  @param  Destination              A pointer to a Null-terminated Ascii string.
  @param  DestMax                  The maximum number of Destination Ascii
                                   char, including terminating null char.
  @param  Source                   A pointer to a Null-terminated Ascii string.
  @param  Length                   The maximum number of Ascii characters to copy.

  @retval RETURN_SUCCESS           String is appended.
  @retval RETURN_BAD_BUFFER_SIZE   If DestMax is NOT greater than
                                   StrLen(Destination).
  @retval RETURN_BUFFER_TOO_SMALL  If (DestMax - StrLen(Destination)) is NOT
                                   greater than MIN(StrLen(Source), Length).
  @retval RETURN_INVALID_PARAMETER If Destination is NULL.
                                   If Source is NULL.
                                   If PcdMaximumAsciiStringLength is not zero,
                                    and DestMax is greater than
                                    PcdMaximumAsciiStringLength.
                                   If DestMax is 0.
  @retval RETURN_ACCESS_DENIED     If Source and Destination overlap.
**/
RETURN_STATUS
AsciiStrnCatS (
  IN OUT CHAR8        *Destination,
  IN     UINTN        DestMax,
  IN     CONST CHAR8  *Source,
  IN     UINTN        Length
  )
{
  UINTN               DestLen;
  UINTN               CopyLen;
  UINTN               SourceLen;

  //
  // Let CopyLen denote the value DestMax - AsciiStrnLenS(Destination, DestMax) upon entry to AsciiStrnCatS.
  //
  DestLen = AsciiStrnLenS (Destination, DestMax);
  CopyLen = DestMax - DestLen;

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Neither DestMax nor Length shall be greater than ASCII_RSIZE_MAX.
  //
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. CopyLen shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((CopyLen != 0), RETURN_BAD_BUFFER_SIZE);

  //
  // 5. If Length is not less than CopyLen, then CopyLen shall be greater than AsciiStrnLenS(Source, CopyLen).
  //
  SourceLen = AsciiStrnLenS (Source, MIN (CopyLen, Length));
  if (Length >= CopyLen) {
    SAFE_STRING_CONSTRAINT_CHECK ((CopyLen > SourceLen), RETURN_BUFFER_TOO_SMALL);
  }

  //
  // 6. Copying shall not take place between objects that overlap.
  //
  if (SourceLen > Length) {
    SourceLen = Length;
  }
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoAsciiStrOverlap (Destination, DestMax, (CHAR8 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The AsciiStrnCatS function appends not more than Length successive characters (characters
  // that follow a null character are not copied) from the array pointed to by Source to the end of
  // the string pointed to by Destination. The initial character from Source overwrites the null character at
  // the end of Destination. If no null character was copied from Source, then Destination[DestMax-CopyLen+Length] is set to
  // a null character.
  //
  Destination = Destination + DestLen;
  while ((SourceLen > 0) && (*Source != 0)) {
    *(Destination++) = *(Source++);
    SourceLen--;
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

CHAR8 *
AsciiStrCat(CHAR8 *Destination, const CHAR8 *Source)
{
	UINTN dest_len = strlena((CHAR8 *)Destination);
	UINTN i;

	for (i = 0; Source[i] != '\0'; i++)
		Destination[dest_len + i] = Source[i];
	Destination[dest_len + i] = '\0';

	return Destination;
}

CHAR8 *
AsciiStrCpy(CHAR8 *Destination, const CHAR8 *Source)
{
	UINTN i;

	for (i=0; Source[i] != '\0'; i++)
		Destination[i] = Source[i];
	Destination[i] = '\0';

	return Destination;
}

CHAR8 *
AsciiStrnCpy(CHAR8 *Destination, const CHAR8 *Source, UINTN count)
{
	UINTN i;

	for (i=0; i < count && Source[i] != '\0'; i++)
		Destination[i] = Source[i];
	for ( ; i < count; i++) 
		Destination[i] = '\0';

	return Destination;
}

CHAR8 *
AsciiStpCpy(CHAR8 *Destination, CONST CHAR8 *Source)
{
        size_t len = AsciiStrLen (Source);
        CopyMem (Destination, Source, len + 1);
        return Destination + len;
}

CHAR8 *
ScanMem8(const CHAR8 *str, UINTN count, CHAR8 ch)
{
	UINTN i;

	for (i = 0; i < count; i++) {
		if (str[i] == ch)
			return (CHAR8 *)str + i;
	}
	return NULL;
}

UINT32
WriteUnaligned32(UINT32 *Buffer, UINT32 Value)
{
	*Buffer = Value;

	return Value;
}

UINTN
AsciiStrSize(const CHAR8 *string)
{
	return strlena(string) + 1;
}

int
strcmp (const char *str1, const char *str2)
{
	return strcmpa((CHAR8 *)str1,(CHAR8 *)str2);
}

inline static char
toupper (char c)
{
	return ((c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c);
}

/* Based on AsciiStriCmp() in edk2 MdePkg/Library/BaseLib/String.c */
int
strcasecmp (const char *str1, const char *str2)
{
	char c1, c2;

	c1 = toupper (*str1);
	c2 = toupper (*str2);
	while ((*str1 != '\0') && (c1 == c2)) {
		str1++;
		str2++;
		c1 = toupper (*str1);
		c2 = toupper (*str2);
	}

	return c1 - c2;
}

/* Based on AsciiStrDecimalToUintnS() in edk2
 * MdePkg/Library/BaseLib/SafeString.c */
UINTN
AsciiStrDecimalToUintn(const CHAR8 *String)
{
	UINTN     Result;

	if (String == NULL)
		return 0;

	/* Ignore the pad spaces (space or tab) */
	while ((*String == ' ') || (*String == '\t')) {
		String++;
	}

	/* Ignore leading Zeros after the spaces */
	while (*String == '0') {
		String++;
	}

	Result = 0;

	while (*String >= '0' && *String <= '9') {
		Result = Result * 10 + (*String - '0');
		String++;
	}

	return Result;
}
