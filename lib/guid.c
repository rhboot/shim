/*
 * Copyright 2018 Ruslan Nikolaev <nruslandev@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "shim.h"

#include <efilib.h>
#include <dpath.h>
#include <console.h>

#include <Guid/PcAnsi.h>
#include <Guid/Gpt.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/GlobalVariable.h>
#include <Guid/WinCertificate.h>

#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/LoadFile.h>
#include <Protocol/DeviceIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/PxeBaseCode.h>
#include <Protocol/PxeBaseCodeCallBack.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/Ip6Config.h>
#include <Protocol/Http.h>
#include <Protocol/TcgService.h>
#include <Protocol/Tcg2Protocol.h>
#include <Protocol/Security.h>
#include <Protocol/Security2.h>
#include <Protocol/SerialIo.h>
#include <Protocol/UnicodeCollation.h>

EFI_GUID gShimLockGuid = { 0x605dab50, 0xe046, 0x4300, { 0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23 } };
EFI_GUID gEfiBdsGuid = { 0x8108ac4e, 0x9f11, 0x4d59, { 0x85, 0x0e, 0xe2, 0x1a, 0x52, 0x2c, 0x59, 0xb2 } };
EFI_GUID gEfiConsoleControlGuid = { 0xf42f7782, 0x12e, 0x4c12, {0x99, 0x56, 0x49, 0xf9, 0x43, 0x4, 0xf7, 0x21} };

EFI_GUID gEfiHttpProtocolGuid = EFI_HTTP_PROTOCOL_GUID;
EFI_GUID gEfiHttpServiceBindingProtocolGuid = EFI_HTTP_SERVICE_BINDING_PROTOCOL_GUID;
EFI_GUID gEfiIp4Config2ProtocolGuid = EFI_IP4_CONFIG2_PROTOCOL_GUID;
EFI_GUID gEfiIp6ConfigProtocolGuid = EFI_IP6_CONFIG_PROTOCOL_GUID;
EFI_GUID gEfiGlobalVariableGuid = EFI_GLOBAL_VARIABLE;
EFI_GUID gEfiCertX509Guid = EFI_CERT_X509_GUID;
EFI_GUID gEfiCertPkcs7Guid = EFI_CERT_TYPE_PKCS7_GUID;
EFI_GUID gEfiFileInfoGuid = EFI_FILE_INFO_ID;
EFI_GUID gEfiFileSystemInfoGuid = EFI_FILE_SYSTEM_INFO_ID;
EFI_GUID gEfiCertSha1Guid = EFI_CERT_SHA1_GUID;
EFI_GUID gEfiCertSha256Guid = EFI_CERT_SHA256_GUID;
EFI_GUID gEfiCertSha224Guid = EFI_CERT_SHA224_GUID;
EFI_GUID gEfiCertSha384Guid = EFI_CERT_SHA384_GUID;
EFI_GUID gEfiCertSha512Guid = EFI_CERT_SHA512_GUID;
EFI_GUID gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiTcgProtocolGuid = EFI_TCG_PROTOCOL_GUID;
EFI_GUID gEfiTcg2ProtocolGuid = EFI_TCG2_PROTOCOL_GUID;
EFI_GUID gEfiImageSecurityDatabaseGuid = EFI_IMAGE_SECURITY_DATABASE_GUID;
EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiPxeBaseCodeProtocolGuid = EFI_PXE_BASE_CODE_PROTOCOL_GUID;
EFI_GUID gEfiSecurityArchProtocolGuid = EFI_SECURITY_ARCH_PROTOCOL_GUID;
EFI_GUID gEfiSecurity2ArchProtocolGuid = EFI_SECURITY2_ARCH_PROTOCOL_GUID;
EFI_GUID gEfiDevicePathProtocolGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
EFI_GUID gEfiUnknownDeviceGuid = UNKNOWN_DEVICE_GUID;

EFI_GUID gEfiPartTypeSystemPartGuid = EFI_PART_TYPE_EFI_SYSTEM_PART_GUID;
EFI_GUID gEfiPartTypeLegacyMbrGuid = EFI_PART_TYPE_LEGACY_MBR_GUID;

EFI_GUID gEfiSimpleTextInProtocolGuid = EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID;
EFI_GUID gEfiSimpleTextOutProtocolGuid = EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiBlockIoProtocolGuid = EFI_BLOCK_IO_PROTOCOL_GUID;
EFI_GUID gEfiBlockIo2ProtocolGuid = EFI_BLOCK_IO2_PROTOCOL_GUID;
EFI_GUID gEfiDiskIoProtocolGuid = EFI_DISK_IO_PROTOCOL_GUID;
EFI_GUID gEfiDiskIo2ProtocolGuid = EFI_DISK_IO2_PROTOCOL_GUID;
EFI_GUID gEfiLoadFileProtocolGuid = EFI_LOAD_FILE_PROTOCOL_GUID;
EFI_GUID gEfiDeviceIoProtocolGuid = EFI_DEVICE_IO_PROTOCOL_GUID;
EFI_GUID gEfiComponentNameProtocolGuid = EFI_COMPONENT_NAME_PROTOCOL_GUID;
EFI_GUID gEfiComponentName2ProtocolGuid = EFI_COMPONENT_NAME2_PROTOCOL_GUID;
EFI_GUID gEfiFileSystemVolumeLabelInfoIdGuid = EFI_FILE_SYSTEM_VOLUME_LABEL_ID;
EFI_GUID gEfiUnicodeCollationProtocolGuid = EFI_UNICODE_COLLATION_PROTOCOL_GUID;
EFI_GUID gEfiUnicodeCollation2ProtocolGuid = EFI_UNICODE_COLLATION_PROTOCOL2_GUID;
EFI_GUID gEfiSerialIoProtocolGuid = EFI_SERIAL_IO_PROTOCOL_GUID;
EFI_GUID gEfiSimpleNetworkProtocolGuid = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;
EFI_GUID gEfiNetworkInterfaceIdentifierProtocolGuid = EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID;
EFI_GUID gEfiNetworkInterfaceIdentifierProtocolGuid_31 = EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID_31;
EFI_GUID gEfiPxeBaseCodeCallbackProtocolGuid = EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_GUID;
EFI_GUID gEfiPcAnsiGuid = EFI_PC_ANSI_GUID;
EFI_GUID gEfiVT100Guid = EFI_VT_100_GUID;
EFI_GUID gEfiVT100PlusGuid = EFI_VT_100_PLUS_GUID;
EFI_GUID gEfiVTUTF8Guid = EFI_VT_UTF8_GUID;


/*
   The below code, i.e. GuidToString() was borrowed from gnu-efi
 */

/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    misc.c

Abstract:

    Misc EFI support functions



Revision History

--*/

//
// Additional Known guids
//

#define VARIABLE_STORE_PROTOCOL \
	{ 0xf088cd91, 0xa046, 0x11d2, {0x8e, 0x42, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define LEGACY_BOOT_PROTOCOL \
	{ 0x376e5eb2, 0x30e4, 0x11d3, { 0xba, 0xe5, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }

#define VGA_CLASS_DRIVER_PROTOCOL \
	{ 0xe3d6310, 0x6fe4, 0x11d3, {0xbb, 0x81, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81} }

#define TEXT_OUT_SPLITER_PROTOCOL \
	{ 0x56d830a0, 0x7e7a, 0x11d3, {0xbb, 0xa0, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define ERROR_OUT_SPLITER_PROTOCOL \
	{ 0xf0ba9039, 0x68f1, 0x425e, {0xaa, 0x7f, 0xd9, 0xaa, 0xf9, 0x1b, 0x82, 0xa1} }

#define TEXT_IN_SPLITER_PROTOCOL \
	{ 0xf9a3c550, 0x7fb5, 0x11d3, {0xbb, 0xa0, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define SHELL_INTERFACE_PROTOCOL \
	{ 0x47c7b223, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define ENVIRONMENT_VARIABLE_ID  \
	{ 0x47c7b224, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define DEVICE_PATH_MAPPING_ID  \
	{ 0x47c7b225, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define PROTOCOL_ID_ID  \
	{ 0x47c7b226, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

#define ALIAS_ID  \
	{ 0x47c7b227, 0xc42a, 0x11d2, {0x8e, 0x57, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} }

static EFI_GUID NullGuid               = { 0,0,0,{0,0,0,0,0,0,0,0} };
static EFI_GUID VariableStoreProtocol  = VARIABLE_STORE_PROTOCOL;
static EFI_GUID LegacyBootProtocol     = LEGACY_BOOT_PROTOCOL;
static EFI_GUID VgaClassProtocol       = VGA_CLASS_DRIVER_PROTOCOL;
static EFI_GUID TextOutSpliterProtocol = TEXT_OUT_SPLITER_PROTOCOL;
static EFI_GUID ErrorOutSpliterProtocol = ERROR_OUT_SPLITER_PROTOCOL;
static EFI_GUID TextInSpliterProtocol  = TEXT_IN_SPLITER_PROTOCOL;

static EFI_GUID ShellInterfaceProtocol = SHELL_INTERFACE_PROTOCOL;
static EFI_GUID SEnvId                 = ENVIRONMENT_VARIABLE_ID;
static EFI_GUID SMapId                 = DEVICE_PATH_MAPPING_ID;
static EFI_GUID SProtId                = PROTOCOL_ID_ID;
static EFI_GUID SAliasId               = ALIAS_ID;

static struct {
    EFI_GUID        *Guid;
    CONST CHAR16    *GuidName;
} KnownGuids[] = {
	{  &NullGuid,                                       L"G0" },
	{  &gEfiGlobalVariableGuid,                         L"EfiVar" },

	{  &VariableStoreProtocol,                          L"VarStore" },
	{  &gEfiDevicePathProtocolGuid,                     L"DevPath" },
	{  &gEfiLoadedImageProtocolGuid,                    L"LdImg" },
	{  &gEfiSimpleTextInProtocolGuid,                   L"TxtIn" },
	{  &gEfiSimpleTextOutProtocolGuid,                  L"TxtOut" },
	{  &gEfiGraphicsOutputProtocolGuid,                 L"GOP" },
	{  &gEfiBlockIoProtocolGuid,                        L"BlkIo" },
	{  &gEfiBlockIo2ProtocolGuid,                       L"BlkIo2" },
	{  &gEfiDiskIoProtocolGuid,                         L"DskIo" },
	{  &gEfiDiskIo2ProtocolGuid,                        L"DskIo2" },
	{  &gEfiSimpleFileSystemProtocolGuid,               L"Fs" },
	{  &gEfiLoadFileProtocolGuid,                       L"LdFile" },
	{  &gEfiDeviceIoProtocolGuid,                       L"DevIo" },
	{  &gEfiComponentNameProtocolGuid,                  L"CName" },
	{  &gEfiComponentName2ProtocolGuid,                 L"CName2" },

	{  &gEfiFileInfoGuid,                               L"FileInfo" },
	{  &gEfiFileSystemInfoGuid,                         L"FsInfo" },
	{  &gEfiFileSystemVolumeLabelInfoIdGuid,            L"FsVolInfo" },

	{  &gEfiUnicodeCollationProtocolGuid,               L"Unicode" },
	{  &gEfiUnicodeCollation2ProtocolGuid,              L"Unicode2" },
	{  &LegacyBootProtocol,                             L"LegacyBoot" },
	{  &gEfiSerialIoProtocolGuid,                       L"SerIo" },
	{  &VgaClassProtocol,                               L"VgaClass"},
	{  &gEfiSimpleNetworkProtocolGuid,                  L"Net" },
	{  &gEfiNetworkInterfaceIdentifierProtocolGuid,     L"Nii" },
	{  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,  L"Nii31" },
	{  &gEfiPxeBaseCodeProtocolGuid,                    L"Pxe" },
	{  &gEfiPxeBaseCodeCallbackProtocolGuid,            L"PxeCb" },

	{  &TextOutSpliterProtocol,                         L"TxtOutSplit" },
	{  &ErrorOutSpliterProtocol,                        L"ErrOutSplit" },
	{  &TextInSpliterProtocol,                          L"TxtInSplit" },
	{  &gEfiPcAnsiGuid,                                 L"PcAnsi" },
	{  &gEfiVT100Guid,                                  L"Vt100" },
	{  &gEfiVT100PlusGuid,                              L"Vt100Plus" },
	{  &gEfiVTUTF8Guid,                                 L"VtUtf8" },
	{  &gEfiUnknownDeviceGuid,                          L"UnknownDev" },

	{  &gEfiPartTypeSystemPartGuid,                     L"ESP" },
	{  &gEfiPartTypeLegacyMbrGuid,                      L"GPT MBR" },

	{  &ShellInterfaceProtocol,                         L"ShellInt" },
	{  &SEnvId,                                         L"SEnv" },
	{  &SProtId,                                        L"ShellProtId" },
	{  &SMapId,                                         L"ShellDevPathMap" },
	{  &SAliasId,                                       L"ShellAlias" },

	{  NULL, L"" }
};

CONST CHAR16 *
GuidToString (
    OUT CHAR16      *Buffer,
    IN EFI_GUID     *Guid
    )
{

    UINTN           Index;

    //
    // Else, (for now) use additional internal function for mapping guids
    //

    for (Index=0; KnownGuids[Index].Guid; Index++) {
        if (CompareGuid(Guid, KnownGuids[Index].Guid) == 0) {
            return KnownGuids[Index].GuidName;
        }
    }

    //
    // Else dump it
    //

    SPrint (Buffer, 0, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        Guid->Data1,
        Guid->Data2,
        Guid->Data3,
        Guid->Data4[0],
        Guid->Data4[1],
        Guid->Data4[2],
        Guid->Data4[3],
        Guid->Data4[4],
        Guid->Data4[5],
        Guid->Data4[6],
        Guid->Data4[7]
        );

    return Buffer;
}
