/*
 * netboot - trivial UEFI first-stage bootloader netboot support
 *
 * Copyright 2012 Red Hat, Inc <mjg@redhat.com>
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
 *
 * Significant portions of this code are derived from Tianocore
 * (http://tianocore.sf.net) and are Copyright 2009-2012 Intel
 * Corporation.
 */

#include "shim.h"

#include <string.h>

#define ntohs(x) __builtin_bswap16(x)	/* supported both by GCC and clang */
#define htons(x) ntohs(x)

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

	efi_status = gBS->HandleProtocol(device, &PxeBaseCodeProtocol,
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
	return (CHAR8 *)ip;
}

static BOOLEAN extract_tftp_info(CHAR8 *url)
{
	CHAR8 *start, *end;
	CHAR8 ip6str[40];
	CHAR8 ip6inv[16];
	CHAR8 *template = (CHAR8 *)translate_slashes(DEFAULT_LOADER_CHAR);

	// to check against str2ip6() errors
	memset(ip6inv, 0, sizeof(ip6inv));

	if (strncmp((UINT8 *)url, (UINT8 *)"tftp://", 7)) {
		console_print(L"URLS MUST START WITH tftp://\n");
		return FALSE;
	}
	start = url + 7;
	if (*start != '[') {
		console_print(L"TFTP SERVER MUST BE ENCLOSED IN [..]\n");
		return FALSE;
	}

	start++;
	end = start;
	while ((*end != '\0') && (*end != ']')) {
		end++;
		if (end - start >= (int)sizeof(ip6str)) {
			console_print(L"TFTP URL includes malformed IPv6 address\n");
			return FALSE;
		}
	}
	if (*end == '\0') {
		console_print(L"TFTP SERVER MUST BE ENCLOSED IN [..]\n");
		return FALSE;
	}
	memset(ip6str, 0, sizeof(ip6str));
	memcpy(ip6str, start, end - start);
	end++;
	memcpy(&tftp_addr.v6, str2ip6(ip6str), 16);
	if (memcmp(&tftp_addr.v6, ip6inv, sizeof(ip6inv)) == 0)
		return FALSE;
	full_path = AllocateZeroPool(strlen(end)+strlen(template)+1);
	if (!full_path)
		return FALSE;
	memcpy(full_path, end, strlen(end));
	end = (CHAR8 *)strrchr((char *)full_path, '/');
	if (!end)
		end = (CHAR8 *)full_path;
	memcpy(end, template, strlen(template));
	end[strlen(template)] = '\0';

	return TRUE;
}

static EFI_STATUS parseDhcp6()
{
	EFI_PXE_BASE_CODE_DHCPV6_PACKET *packet = (EFI_PXE_BASE_CODE_DHCPV6_PACKET *)&pxe->Mode->DhcpAck.Raw;
	CHAR8 *bootfile_url;

	bootfile_url = get_v6_bootfile_url(packet);
	if (!bootfile_url)
		return EFI_NOT_FOUND;
	if (extract_tftp_info(bootfile_url) == FALSE) {
		FreePool(bootfile_url);
		return EFI_NOT_FOUND;
	}
	FreePool(bootfile_url);
	return EFI_SUCCESS;
}

static EFI_STATUS parseDhcp4()
{
	CHAR8 *template = (CHAR8 *)translate_slashes(DEFAULT_LOADER_CHAR);
	INTN template_len = strlen(template) + 1;
	EFI_PXE_BASE_CODE_DHCPV4_PACKET* pkt_v4 = (EFI_PXE_BASE_CODE_DHCPV4_PACKET *)&pxe->Mode->DhcpAck.Dhcpv4;

	if(pxe->Mode->ProxyOfferReceived) {
		/*
		 * Proxy should not have precedence.  Check if DhcpAck
		 * contained boot info.
		 */
		if(pxe->Mode->DhcpAck.Dhcpv4.BootpBootFile[0] == '\0')
			pkt_v4 = &pxe->Mode->ProxyOffer.Dhcpv4;
	}

	INTN dir_len = strnlena(pkt_v4->BootpBootFile, 127);
	INTN i;
	UINT8 *dir = pkt_v4->BootpBootFile;

	for (i = dir_len; i >= 0; i--) {
		if (dir[i] == '/')
			break;
	}
	dir_len = (i >= 0) ? i + 1 : 0;

	full_path = AllocateZeroPool(dir_len + template_len);

	if (!full_path)
		return EFI_OUT_OF_RESOURCES;

	if (dir_len > 0) {
		strncpya(full_path, dir, dir_len);
		if (full_path[dir_len-1] == '/' && template[0] == '/')
			full_path[dir_len-1] = '\0';
	}
	if (dir_len == 0 && dir[0] != '/' && template[0] == '/')
		template++;
	strcata(full_path, template);
	memcpy(&tftp_addr.v4, pkt_v4->BootpSiAddr, 4);

	return EFI_SUCCESS;
}

EFI_STATUS parseNetbootinfo(EFI_HANDLE image_handle)
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
		efi_status = parseDhcp6();
	} else
		efi_status = parseDhcp4();
	return efi_status;
}

EFI_STATUS FetchNetbootimage(EFI_HANDLE image_handle, VOID **buffer, UINT64 *bufsiz)
{
	EFI_STATUS efi_status;
	EFI_PXE_BASE_CODE_TFTP_OPCODE read = EFI_PXE_BASE_CODE_TFTP_READ_FILE;
	BOOLEAN overwrite = FALSE;
	BOOLEAN nobuffer = FALSE;
	UINTN blksz = 512;

	console_print(L"Fetching Netboot Image\n");
	if (*buffer == NULL) {
		*buffer = AllocatePool(4096 * 1024);
		if (!*buffer)
			return EFI_OUT_OF_RESOURCES;
		*bufsiz = 4096 * 1024;
	}

try_again:
	efi_status = pxe->Mtftp(pxe, read, *buffer, overwrite, bufsiz, &blksz,
			      &tftp_addr, full_path, NULL, nobuffer);
	if (efi_status == EFI_BUFFER_TOO_SMALL) {
		/* try again, doubling buf size */
		*bufsiz *= 2;
		FreePool(*buffer);
		*buffer = AllocatePool(*bufsiz);
		if (!*buffer)
			return EFI_OUT_OF_RESOURCES;
		goto try_again;
	}

	if (EFI_ERROR(efi_status) && *buffer) {
		FreePool(*buffer);
	}
	return efi_status;
}
