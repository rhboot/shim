#include <OpenSslSupport.h>

char *
AsciiStrCat(char *Destination, char *Source)
{
	UINTN dest_len = strlena((CHAR8 *)Destination);
	UINTN i;

	for (i = 0; Source[i] != '\0'; i++)
		Destination[dest_len + i] = Source[i];
	Destination[dest_len + i] = '\0';

	return Destination;
}

CHAR8 *
AsciiStrCpy(CHAR8 *Destination, CHAR8 *Source)
{
	UINTN i;

	for (i=0; Source[i] != '\0'; i++)
		Destination[i] = Source[i];
	Destination[i] = '\0';

	return Destination;
}

char *
AsciiStrnCpy(char *Destination, char *Source, UINTN count)
{
	UINTN i;

	for (i=0; i < count && Source[i] != '\0'; i++)
		Destination[i] = Source[i];
	for ( ; i < count; i++) 
		Destination[i] = '\0';

	return Destination;
}

CHAR8 *
ScanMem8(CHAR8 *str, UINTN count, CHAR8 ch)
{
	UINTN i;

	for (i = 0; i < count; i++) {
		if (str[i] == ch)
			return str + i;
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
AsciiStrSize(CHAR8 *string)
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
AsciiStrDecimalToUintn(const char *String)
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
