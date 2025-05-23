// SPDX-License-Identifier: BSD-2-Clause-Patent

/*
 * netboot - trivial UEFI first-stage bootloader netboot support
 *
 * Copyright Red Hat, Inc
 * Author: Matthew Garrett
 *
 * Significant portions of this code are derived from Tianocore
 * (http://tianocore.sf.net) and are Copyright 2009-2012 Intel
 * Corporation.
 */

#include "shim.h"

#define ntohs(x) __builtin_bswap16(x)	/* supported both by GCC and clang */
#define htons(x) ntohs(x)

/* TFTP error codes from RFC 1350 */
#define TFTP_ERROR_NOT_DEFINED  0  /* Not defined, see error message (if any). */
#define TFTP_ERROR_NOT_FOUND    1  /* File not found. */
#define TFTP_ERROR_ACCESS       2  /* Access violation. */
#define TFTP_ERROR_NO_SPACE     3  /* Disk full or allocation exceeded. */
#define TFTP_ERROR_ILLEGAL_OP   4  /* Illegal TFTP operation. */
#define TFTP_ERROR_UNKNOWN_ID   5  /* Unknown transfer ID. */
#define TFTP_ERROR_EXISTS       6  /* File already exists. */
#define TFTP_ERROR_NO_USER      7  /* No such user. */

static EFI_PXE_BASE_CODE *pxe;
static EFI_IP_ADDRESS tftp_addr;
static CHAR8 *full_path;


typedef struct {
	UINT16 OpCode;
	UINT16 Length;
	UINT8 Data[1];
} EFI_DHCP6_PACKET_OPTION;

/*
 * usingNetboot
 * Returns TRUE if we identify a protocol that is enabled and Providing us with
 * the needed information to fetch a grubx64.efi image
 */
BOOLEAN findNetboot(EFI_HANDLE device)
{
	EFI_STATUS efi_status;

	efi_status = BS->HandleProtocol(device, &PxeBaseCodeProtocol,
					(VOID **) &pxe);
	if (EFI_ERROR(efi_status)) {
		pxe = NULL;
		return FALSE;
	}

	if (!pxe || !pxe->Mode) {
		pxe = NULL;
		return FALSE;
	}

	if (!pxe->Mode->Started || !pxe->Mode->DhcpAckReceived) {
		pxe = NULL;
		return FALSE;
	}

	/*
	 * We've located a pxe protocol handle thats been started and has
	 * received an ACK, meaning its something we'll be able to get
	 * tftp server info out of
	 */
	return TRUE;
}

static CHAR8 *get_v6_bootfile_url(EFI_PXE_BASE_CODE_DHCPV6_PACKET *pkt)
{
	void *optr = NULL, *end = NULL;
	EFI_DHCP6_PACKET_OPTION *option = NULL;
	CHAR8 *url = NULL;
	UINT32 urllen = 0;

	optr = pkt->DhcpOptions;
	end = optr + sizeof(pkt->DhcpOptions);

	for (;;) {
		option = (EFI_DHCP6_PACKET_OPTION *)optr;

		if (ntohs(option->OpCode) == 0)
			break;

		if (ntohs(option->OpCode) == 59) {
			/* This is the bootfile url option */
			urllen = ntohs(option->Length);
			if ((void *)(option->Data + urllen) > end)
				break;
			url = AllocateZeroPool(urllen + 1);
			if (!url)
				break;
			memcpy(url, option->Data, urllen);
			return url;
		}
		optr += 4 + ntohs(option->Length);
		if (optr + sizeof(EFI_DHCP6_PACKET_OPTION) > end)
			break;
	}

	return NULL;
}

static CHAR16 str2ns(CHAR8 *str)
{
        CHAR16 ret = 0;
        CHAR8 v;
        for(;*str;str++) {
                if ('0' <= *str && *str <= '9')
                        v = *str - '0';
                else if ('A' <= *str && *str <= 'F')
                        v = *str - 'A' + 10;
                else if ('a' <= *str && *str <= 'f')
                        v = *str - 'a' + 10;
                else
                        v = 0;
                ret = (ret << 4) + v;
        }
        return htons(ret);
}

static CHAR8 *str2ip6(CHAR8 *str)
{
	bool double_colon_found = false;
	UINT8 i = 0, j = 0, p = 0;
	size_t len = 0, dotcount = 0;
	enum { MAX_IP6_DOTS = 7 };
	CHAR8 *a = NULL, *b = NULL, t = 0;
	static UINT16 ip[8];

	memset(ip, 0, sizeof(ip));

	/* Count amount of ':' to prevent overflows.
	 * max. count = 7. Returns an invalid ip6 that
	 * can be checked against
	 */
	for (a = str; *a != 0; ++a) {
		if (*a == ':')
			++dotcount;
	}
	if (dotcount > MAX_IP6_DOTS)
		return (CHAR8 *)ip;

	len = strlen(str);
	a = b = str;

	for (i = 0; i < len; i++) {
                if (str[i] == ':') {
                        if (i+1 < (UINT8)len && str[i+1] == ':') {
                                if (double_colon_found)
                                        return (CHAR8 *)ip;
                                double_colon_found = true;
                        }
                }
                else {
                        if (str[i] < '0' || (str[i] > '9' && str[i] < 'A') ||
                           (str[i] > 'F' && str[i] < 'a') || str[i] > 'f')
                                return (CHAR8 *)ip;
                }
        }


	for (i = p = 0; i < len; i++, b++) {
		if (*b != ':')
			continue;
		*b = '\0';
		ip[p++] = str2ns(a);
		*b = ':';
		a = b + 1;
		if (b[1] == ':' )
			break;
	}

	if (i == len) {
		ip[p++] = str2ns(a);
	} else {
		a = b = (str + len);
		for (j = len, p = 7; j > i; j--, a--) {
			if (*a != ':')
				continue;
			t = *b;
			*b = '\0';
			ip[p--] = str2ns(a+1);
			*b = t;
			b = a;
		}
	}
	return (CHAR8 *)ip;
}

static BOOLEAN extract_tftp_info(CHAR8 *url, CHAR8 *name)
{
	CHAR8 *start, *end;
	CHAR8 ip6str[40];
	CHAR8 ip6inv[16];
	int template_len = 0;
	CHAR8 *template;

	while (name[template_len++] != '\0');
	template = (CHAR8 *)AllocatePool((template_len + 1) * sizeof (CHAR8));
	translate_slashes(template, name);

	// to check against str2ip6() errors
	memset(ip6inv, 0, sizeof(ip6inv));

	if (strncmp((const char *)url, (const char *)"tftp://", 7)) {
		console_print(L"URLS MUST START WITH tftp://\n");
		FreePool(template);
		return FALSE;
	}
	start = url + 7;
	if (*start != '[') {
		console_print(L"TFTP SERVER MUST BE ENCLOSED IN [..]\n");
		FreePool(template);
		return FALSE;
	}

	start++;
	end = start;
	while ((*end != '\0') && (*end != ']')) {
		end++;
		if (end - start >= (int)sizeof(ip6str)) {
			console_print(L"TFTP URL includes malformed IPv6 address\n");
			FreePool(template);
			return FALSE;
		}
	}
	if (*end == '\0') {
		console_print(L"TFTP SERVER MUST BE ENCLOSED IN [..]\n");
		FreePool(template);
		return FALSE;
	}
	memset(ip6str, 0, sizeof(ip6str));
	memcpy(ip6str, start, end - start);
	end++;
	memcpy(&tftp_addr.v6, str2ip6(ip6str), 16);
	if (memcmp(&tftp_addr.v6, ip6inv, sizeof(ip6inv)) == 0) {
		FreePool(template);
		return FALSE;
	}
	full_path = AllocateZeroPool(strlen(end)+strlen(template)+1);
	if (!full_path) {
		FreePool(template);
		return FALSE;
	}
	memcpy(full_path, end, strlen(end));
	end = (CHAR8 *)strrchr((char *)full_path, '/');
	if (!end)
		end = (CHAR8 *)full_path;
	memcpy(end, template, strlen(template));
	end[strlen(template)] = '\0';

	FreePool(template);
	return TRUE;
}

static EFI_STATUS parseDhcp6(CHAR8 *name)
{
	EFI_PXE_BASE_CODE_DHCPV6_PACKET *packet = (EFI_PXE_BASE_CODE_DHCPV6_PACKET *)&pxe->Mode->DhcpAck.Raw;
	CHAR8 *bootfile_url;

	bootfile_url = get_v6_bootfile_url(packet);
	if (!bootfile_url)
		return EFI_NOT_FOUND;
	if (extract_tftp_info(bootfile_url, name) == FALSE) {
		FreePool(bootfile_url);
		return EFI_NOT_FOUND;
	}
	FreePool(bootfile_url);
	return EFI_SUCCESS;
}

static EFI_STATUS parseDhcp4(CHAR8 *name)
{
	CHAR8 *template;
	INTN template_len = 0;
	UINTN template_ofs = 0;
	EFI_PXE_BASE_CODE_DHCPV4_PACKET* pkt_v4 = (EFI_PXE_BASE_CODE_DHCPV4_PACKET *)&pxe->Mode->DhcpAck.Dhcpv4;

	while (name[template_len++] != '\0');
	template = (CHAR8 *)AllocatePool((template_len + 1) * sizeof (CHAR8));
	translate_slashes(template, name);
	template_len = strlen(template) + 1;

	if(pxe->Mode->ProxyOfferReceived) {
		/*
		 * Proxy should not have precedence.  Check if DhcpAck
		 * contained boot info.
		 */
		if(pxe->Mode->DhcpAck.Dhcpv4.BootpBootFile[0] == '\0')
			pkt_v4 = &pxe->Mode->ProxyOffer.Dhcpv4;
	}

	if(pxe->Mode->PxeReplyReceived) {
		/*
		 * If we have no bootinfo yet search for it in the PxeReply.
		 * Some mainboards run into this when the server uses boot menus
		 */
		if(pkt_v4->BootpBootFile[0] == '\0' && pxe->Mode->PxeReply.Dhcpv4.BootpBootFile[0] != '\0')
			pkt_v4 = &pxe->Mode->PxeReply.Dhcpv4;
	}

	INTN dir_len = strnlen((CHAR8 *)pkt_v4->BootpBootFile, 127);
	INTN i;
	UINT8 *dir = pkt_v4->BootpBootFile;

	for (i = dir_len; i >= 0; i--) {
		if ((dir[i] == '/') || (dir[i] == '\\'))
			break;
	}
	dir_len = (i >= 0) ? i + 1 : 0;

	full_path = AllocateZeroPool(dir_len + template_len);

	if (!full_path) {
		FreePool(template);
		return EFI_OUT_OF_RESOURCES;
	}

	if (dir_len > 0) {
		strncpy(full_path, (CHAR8 *)dir, dir_len);
		if (full_path[dir_len-1] == '/' && template[0] == '/')
			full_path[dir_len-1] = '\0';
		/*
		 * If the path from DHCP is using backslash instead of slash,
		 * accept that and use it in the template in the same position
		 * as well.
		 */
		if (full_path[dir_len-1] == '\\' && template[0] == '/') {
			full_path[dir_len-1] = '\0';
			template[0] = '\\';
		}
	}
	if (dir_len == 0 && dir[0] != '/' && template[0] == '/')
		template_ofs++;
	strcat(full_path, template + template_ofs);
	memcpy(&tftp_addr.v4, pkt_v4->BootpSiAddr, 4);

	FreePool(template);
	return EFI_SUCCESS;
}

EFI_STATUS parseNetbootinfo(EFI_HANDLE image_handle UNUSED, CHAR8 *netbootname)
{

	EFI_STATUS efi_status;

	if (!pxe)
		return EFI_NOT_READY;

	memset((UINT8 *)&tftp_addr, 0, sizeof(tftp_addr));

	/*
	 * If we've discovered an active pxe protocol figure out
	 * if its ipv4 or ipv6
	 */
	if (pxe->Mode->UsingIpv6){
		efi_status = parseDhcp6(netbootname);
	} else
		efi_status = parseDhcp4(netbootname);
	return efi_status;
}

/* Convert a TFTP error code to an EFI status code. */
static EFI_STATUS
status_from_error(UINT8 error_code)
{
	switch (error_code) {
	case TFTP_ERROR_NOT_FOUND:
		return EFI_NOT_FOUND;
	case TFTP_ERROR_ACCESS:
	case TFTP_ERROR_NO_USER:
		return EFI_ACCESS_DENIED;
	case TFTP_ERROR_NO_SPACE:
		return EFI_VOLUME_FULL;
	case TFTP_ERROR_ILLEGAL_OP:
		return EFI_PROTOCOL_ERROR;
	case TFTP_ERROR_UNKNOWN_ID:
		return EFI_INVALID_PARAMETER;
	case TFTP_ERROR_EXISTS:
		return EFI_WRITE_PROTECTED;
	case TFTP_ERROR_NOT_DEFINED:
	default:
		/* Use a generic TFTP error for anything else. */
		return EFI_TFTP_ERROR;
	}
}

EFI_STATUS FetchNetbootimage(EFI_HANDLE image_handle UNUSED, VOID **buffer,
	UINT64 *bufsiz, int flags)
{
	EFI_STATUS efi_status;
	EFI_PXE_BASE_CODE_TFTP_OPCODE read = EFI_PXE_BASE_CODE_TFTP_READ_FILE;
	BOOLEAN overwrite = FALSE;
	BOOLEAN nobuffer = FALSE;
	UINTN blksz = 512;

	if (~flags & SUPPRESS_NETBOOT_OPEN_FAILURE_NOISE)
		console_print(L"Fetching Netboot Image %a\n", full_path);
	if (*buffer == NULL) {
		*buffer = AllocatePool(4096 * 1024);
		if (!*buffer)
			return EFI_OUT_OF_RESOURCES;
		*bufsiz = 4096 * 1024;
	}

try_again:
	efi_status = pxe->Mtftp(pxe, read, *buffer, overwrite, bufsiz, &blksz,
			      &tftp_addr, (UINT8 *)full_path, NULL, nobuffer);
	if (efi_status == EFI_BUFFER_TOO_SMALL) {
		/* try again, doubling buf size */
		*bufsiz *= 2;
		FreePool(*buffer);
		*buffer = AllocatePool(*bufsiz);
		if (!*buffer)
			return EFI_OUT_OF_RESOURCES;
		goto try_again;
	}

	if (EFI_ERROR(efi_status)) {
		if (pxe->Mode->TftpErrorReceived) {
			if (~flags & SUPPRESS_NETBOOT_OPEN_FAILURE_NOISE)
				console_print(L"TFTP error %u: %a\n",
				      pxe->Mode->TftpError.ErrorCode,
				      pxe->Mode->TftpError.ErrorString);

			efi_status = status_from_error(pxe->Mode->TftpError.ErrorCode);
		} else if (efi_status == EFI_TFTP_ERROR) {
			/*
			 * Unfortunately, some firmware doesn't fill in the
			 * error details. Treat all TFTP errors like file not
			 * found so shim falls back to the default loader.
			 *
			 * https://github.com/tianocore/edk2/pull/6287
			 */
			if (~flags & SUPPRESS_NETBOOT_OPEN_FAILURE_NOISE)
				console_print(L"Unknown TFTP error, treating "
					"as file not found\n");
			efi_status = EFI_NOT_FOUND;
		}

		if (*buffer) {
			FreePool(*buffer);
		}
	}
	return efi_status;
}
