/*++

Adopted from efilib.h, efidevp.h from gnu-efi/inc
Copyright (c) 1998-2000  Intel Corporation

--*/

#pragma once

#include <efi.h>
#include <Protocol/DevicePath.h>

#ifdef __cplusplus
extern "C"
#endif

BOOLEAN
LibMatchDevicePaths (
    IN  EFI_DEVICE_PATH *Multi,
    IN  EFI_DEVICE_PATH *Single
    );

EFI_DEVICE_PATH *
LibDuplicateDevicePathInstance (
    IN EFI_DEVICE_PATH  *DevPath
    );

EFI_DEVICE_PATH *
DevicePathFromHandle (
    IN EFI_HANDLE           Handle
    );

EFI_DEVICE_PATH *
DevicePathInstance (
    IN OUT EFI_DEVICE_PATH  **DevicePath,
    OUT UINTN               *Size
    );

UINTN
DevicePathInstanceCount (
    IN EFI_DEVICE_PATH      *DevicePath
    );

EFI_DEVICE_PATH *
AppendDevicePath (
    IN EFI_DEVICE_PATH      *Src1,
    IN EFI_DEVICE_PATH      *Src2
    );

EFI_DEVICE_PATH *
AppendDevicePathNode (
    IN EFI_DEVICE_PATH      *Src1,
    IN EFI_DEVICE_PATH      *Src2
    );

EFI_DEVICE_PATH*
AppendDevicePathInstance (
    IN EFI_DEVICE_PATH  *Src,
    IN EFI_DEVICE_PATH  *Instance
    );

EFI_DEVICE_PATH *
FileDevicePath (
    IN EFI_HANDLE           Device  OPTIONAL,
    IN CHAR16               *FileName
    );

UINTN
DevicePathSize (
    IN EFI_DEVICE_PATH      *DevPath
    );

EFI_DEVICE_PATH *
DuplicateDevicePath (
    IN EFI_DEVICE_PATH      *DevPath
    );

EFI_DEVICE_PATH *
UnpackDevicePath (
    IN EFI_DEVICE_PATH      *DevPath
    );

EFI_STATUS
LibDevicePathToInterface (
    IN EFI_GUID             *Protocol,
    IN EFI_DEVICE_PATH      *FilePath,
    OUT VOID                **Interface
    );

CHAR16 *
DevicePathToStr (
    EFI_DEVICE_PATH         *DevPath
    );

#define UNKNOWN_DEVICE_GUID \
    { 0xcf31fac5, 0xc24e, 0x11d2,  {0x85, 0xf3, 0x0, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b}  }

typedef struct _UKNOWN_DEVICE_VENDOR_DEVICE_PATH {
    VENDOR_DEVICE_PATH      DevicePath;
    UINT8                   LegacyDriveLetter;
} UNKNOWN_DEVICE_VENDOR_DEVICE_PATH;

#define EFI_DP_TYPE_MASK                    0x7F
#define EFI_DP_TYPE_UNPACKED                0x80
#define END_DEVICE_PATH_LENGTH              (sizeof(EFI_DEVICE_PATH))

#define DP_IS_END_TYPE(a)
#define DP_IS_END_SUBTYPE(a)        ( ((a)->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE )

#define DevicePathType(a)           ( ((a)->Type) & EFI_DP_TYPE_MASK )
#define DevicePathSubType(a)        ( (a)->SubType )
#define DevicePathNodeLength(a)     ( ((a)->Length[0]) | ((a)->Length[1] << 8) )
#define NextDevicePathNode(a)       ( (EFI_DEVICE_PATH *) ( ((UINT8 *) (a)) + DevicePathNodeLength(a)))
//#define IsDevicePathEndType(a)      ( DevicePathType(a) == END_DEVICE_PATH_TYPE_UNPACKED )
#define IsDevicePathEndType(a)      ( DevicePathType(a) == END_DEVICE_PATH_TYPE )
#define IsDevicePathEndSubType(a)   ( (a)->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE )
#define IsDevicePathEnd(a)          ( IsDevicePathEndType(a) && IsDevicePathEndSubType(a) )
#define IsDevicePathUnpacked(a)     ( (a)->Type & EFI_DP_TYPE_UNPACKED )


#define SetDevicePathNodeLength(a,l) {                  \
            (a)->Length[0] = (UINT8) (l);               \
            (a)->Length[1] = (UINT8) ((l) >> 8);        \
            }

#define SetDevicePathEndNode(a)  {                      \
            (a)->Type = END_DEVICE_PATH_TYPE;           \
            (a)->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;     \
            (a)->Length[0] = sizeof(EFI_DEVICE_PATH);   \
            (a)->Length[1] = 0;                         \
            }

extern EFI_GUID gEfiUnknownDeviceGuid;

#ifdef __cplusplus
}
#endif
