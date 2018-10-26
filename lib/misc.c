/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    misc.c

Abstract:




Revision History

--*/

#include <efilib.h>

#define DEBUG(...)

BOOLEAN
GrowBuffer (
    IN OUT EFI_STATUS   *Status,
    IN OUT VOID         **Buffer,
    IN UINTN            BufferSize
    )
/*++

Routine Description:

    Helper function called as part of the code needed
    to allocate the proper sized buffer for various 
    EFI interfaces.

Arguments:

    Status      - Current status

    Buffer      - Current allocated buffer, or NULL

    BufferSize  - Current buffer size needed
    
Returns:
    
    TRUE - if the buffer was reallocated and the caller 
    should try the API again.

--*/
{
    BOOLEAN         TryAgain;

    //
    // If this is an initial request, buffer will be null with a new buffer size
    //

    if (!*Buffer && BufferSize) {
        *Status = EFI_BUFFER_TOO_SMALL;
    }

    //
    // If the status code is "buffer too small", resize the buffer
    //

    TryAgain = FALSE;
    if (*Status == EFI_BUFFER_TOO_SMALL) {

        if (*Buffer) {
            FreePool (*Buffer);
        }

        *Buffer = AllocatePool (BufferSize);

        if (*Buffer) {
            TryAgain = TRUE;
        } else {    
            *Status = EFI_OUT_OF_RESOURCES;
        } 
    }

    //
    // If there's an error, free the buffer
    //

    if (!TryAgain && EFI_ERROR(*Status) && *Buffer) {
        FreePool (*Buffer);
        *Buffer = NULL;
    }

    return TryAgain;
}

EFI_STATUS
LibLocateProtocol (
    IN  EFI_GUID    *ProtocolGuid,
    OUT VOID        **Interface
    )
//
// Find the first instance of this Protocol in the system and return it's interface
//
{
    EFI_STATUS      Status;
    UINTN           NumberHandles, Index;
    EFI_HANDLE      *Handles;

    
    *Interface = NULL;
    Status = LibLocateHandle (ByProtocol, ProtocolGuid, NULL, &NumberHandles, &Handles);
    if (EFI_ERROR(Status)) {
        DEBUG((D_INFO, "LibLocateProtocol: Handle not found\n"));
        return Status;
    }

    for (Index=0; Index < NumberHandles; Index++) {
        Status = gBS->HandleProtocol(Handles[Index], ProtocolGuid, Interface);
        if (!EFI_ERROR(Status)) {
            break;
        }
    }

    if (Handles) {
        FreePool (Handles);
    }

    return Status;
}

EFI_STATUS
LibLocateHandle (
    IN EFI_LOCATE_SEARCH_TYPE       SearchType,
    IN EFI_GUID                     *Protocol OPTIONAL,
    IN VOID                         *SearchKey OPTIONAL,
    IN OUT UINTN                    *NoHandles,
    OUT EFI_HANDLE                  **Buffer
    )

{
    EFI_STATUS          Status;
    UINTN               BufferSize;

    //
    // Initialize for GrowBuffer loop
    //

    Status = EFI_SUCCESS;
    *Buffer = NULL;
    BufferSize = 50 * sizeof(EFI_HANDLE);

    //
    // Call the real function
    //

    while (GrowBuffer (&Status, (VOID **) Buffer, BufferSize)) {

        Status = gBS->LocateHandle(SearchType,
                        Protocol,
                        SearchKey,
                        &BufferSize,
                        *Buffer
                        );

    }

    *NoHandles = BufferSize / sizeof (EFI_HANDLE);
    if (EFI_ERROR(Status)) {
        *NoHandles = 0;
    }

    return Status;
}

EFI_STATUS
WaitForSingleEvent (
    IN EFI_EVENT        Event,
    IN UINT64           Timeout OPTIONAL
    )
{
    EFI_STATUS          Status;
    UINTN               Index;
    EFI_EVENT           TimerEvent;
    EFI_EVENT           WaitList[2];

    if (Timeout) {
        //
        // Create a timer event
        //

        Status = gBS->CreateEvent(EVT_TIMER, 0, NULL, NULL, &TimerEvent);
        if (!EFI_ERROR(Status)) {

            //
            // Set the timer event
            //

            gBS->SetTimer(TimerEvent, TimerRelative, Timeout);

            //
            // Wait for the original event or the timer
            //

            WaitList[0] = Event;
            WaitList[1] = TimerEvent;
            Status = gBS->WaitForEvent(2, WaitList, &Index);
            gBS->CloseEvent(TimerEvent);

            //
            // If the timer expired, change the return to timed out
            //

            if (!EFI_ERROR(Status)  &&  Index == 1) {
                Status = EFI_TIMEOUT;
            }
        }

    } else {

        //
        // No timeout... just wait on the event
        //

        Status = gBS->WaitForEvent(1, &Event, &Index);
        ASSERT (!EFI_ERROR(Status));
        ASSERT (Index == 0);
    }

    return Status;
}
