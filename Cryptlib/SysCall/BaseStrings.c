#include <OpenSslSupport.h>

CHAR8 *
AsciiStrCat(CHAR8 *Destination, CHAR8 *Source)
{
	UINTN dest_len = strlena(Destination);
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

CHAR8 *
AsciiStrnCpy(CHAR8 *Destination, CHAR8 *Source, UINTN count)
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


