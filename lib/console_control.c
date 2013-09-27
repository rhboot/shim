#include <efi.h>
#include <efilib.h>

#include "console_control.h"

VOID setup_console (int text)
{
	EFI_STATUS status;
	EFI_GUID console_control_guid = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
	EFI_CONSOLE_CONTROL_PROTOCOL *concon;
	static EFI_CONSOLE_CONTROL_SCREEN_MODE mode =
					EfiConsoleControlScreenGraphics;
	EFI_CONSOLE_CONTROL_SCREEN_MODE new_mode;

	status = LibLocateProtocol(&console_control_guid, (VOID **)&concon);
	if (status != EFI_SUCCESS)
		return;

	if (text) {
		new_mode = EfiConsoleControlScreenText;

		status = uefi_call_wrapper(concon->GetMode, 4, concon, &mode,
						0, 0);
		/* If that didn't work, assume it's graphics */
		if (status != EFI_SUCCESS)
			mode = EfiConsoleControlScreenGraphics;
	} else {
		new_mode = mode;
	}

	uefi_call_wrapper(concon->SetMode, 2, concon, new_mode);
}
