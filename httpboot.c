/*
 * Copyright 2015 SUSE LINUX GmbH <glin@suse.com>
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

#include <efi.h>
#include <efilib.h>

#include "shim.h"

static UINTN
ascii_to_int (CONST CHAR8 *str)
{
    UINTN u;
    CHAR8 c;

    // skip preceeding white space
    while (*str && *str == ' ') {
        str += 1;
    }

    // convert digits
    u = 0;
    while ((c = *(str++))) {
        if (c >= '0' && c <= '9') {
            u = (u * 10) + c - '0';
        } else {
            break;
        }
    }

    return u;
}

static UINTN
convert_http_status_code (EFI_HTTP_STATUS_CODE status_code)
{
	if (status_code >= HTTP_STATUS_100_CONTINUE &&
	    status_code <  HTTP_STATUS_200_OK) {
		return (status_code - HTTP_STATUS_100_CONTINUE + 100);
	} else if (status_code >= HTTP_STATUS_200_OK &&
		   status_code <  HTTP_STATUS_300_MULTIPLE_CHIOCES) {
		return (status_code - HTTP_STATUS_200_OK + 200);
	} else if (status_code >= HTTP_STATUS_300_MULTIPLE_CHIOCES &&
		   status_code <  HTTP_STATUS_400_BAD_REQUEST) {
		return (status_code - HTTP_STATUS_300_MULTIPLE_CHIOCES + 300);
	} else if (status_code >= HTTP_STATUS_400_BAD_REQUEST &&
		   status_code <  HTTP_STATUS_500_INTERNAL_SERVER_ERROR) {
		return (status_code - HTTP_STATUS_400_BAD_REQUEST + 400);
	} else if (status_code >= HTTP_STATUS_500_INTERNAL_SERVER_ERROR) {
		return (status_code - HTTP_STATUS_500_INTERNAL_SERVER_ERROR + 500);
	}

	return 0;
}

static EFI_DEVICE_PATH *devpath;
static EFI_MAC_ADDRESS mac_addr;
static IPv4_DEVICE_PATH ip4_node;
static IPv6_DEVICE_PATH ip6_node;
static BOOLEAN is_ip6;
static CHAR8 *uri;

BOOLEAN
find_httpboot (EFI_HANDLE device)
{
	EFI_DEVICE_PATH *unpacked;
	EFI_DEVICE_PATH *Node;
	MAC_ADDR_DEVICE_PATH *MacNode;
	URI_DEVICE_PATH *UriNode;
	UINTN uri_size;
	BOOLEAN ip_found = FALSE;
	BOOLEAN ret = FALSE;

	if (uri) {
		FreePool(uri);
		uri = NULL;
	}

	devpath = DevicePathFromHandle(device);
	if (!devpath) {
		perror(L"Failed to get device path\n");
		return FALSE;
	}

	unpacked = UnpackDevicePath(devpath);
	if (!unpacked) {
		perror(L"Failed to unpack device path\n");
		return FALSE;
	}
	Node = unpacked;

	/* Traverse the device path to find IPv4()/.../Uri() or
	 * IPv6()/.../Uri() */
	while (!IsDevicePathEnd(Node)) {
		/* Save the MAC node so we can match the net card later */
		if (DevicePathType(Node) == MESSAGING_DEVICE_PATH &&
		    DevicePathSubType(Node) == MSG_MAC_ADDR_DP) {
			MacNode = (MAC_ADDR_DEVICE_PATH *)Node;
			CopyMem(&mac_addr, &MacNode->MacAddress,
				sizeof(EFI_MAC_ADDRESS));
		} else if (DevicePathType(Node) == MESSAGING_DEVICE_PATH &&
			   (DevicePathSubType(Node) == MSG_IPv4_DP ||
			    DevicePathSubType(Node) == MSG_IPv6_DP)) {
			/* Save the IP node so we can set up the connection */
			/* later */
			if (DevicePathSubType(Node) == MSG_IPv6_DP) {
				CopyMem(&ip6_node, Node,
					sizeof(IPv6_DEVICE_PATH));
				is_ip6 = TRUE;
			} else {
				CopyMem(&ip4_node, Node,
					sizeof(IPv4_DEVICE_PATH));
				is_ip6 = FALSE;
			}

			ip_found = TRUE;
		} else if (ip_found == TRUE &&
			   (DevicePathType(Node) == MESSAGING_DEVICE_PATH &&
			    DevicePathSubType(Node) == MSG_URI_DP)) {
			EFI_DEVICE_PATH *NextNode;

			/* Check if the URI node is the last node since the */
			/* RAMDISK node could be appended, and we don't need */
			/* to download the second stage loader in that case. */
			NextNode = NextDevicePathNode(Node);
			if (!IsDevicePathEnd(NextNode))
				goto out;

			/* Save the current URI */
			UriNode = (URI_DEVICE_PATH *)Node;
			uri_size = strlena(UriNode->Uri);
			uri = AllocatePool(uri_size + 1);
			if (!uri) {
				perror(L"Failed to allocate uri\n");
				goto out;
			}
			CopyMem(uri, UriNode->Uri, uri_size + 1);
			ret = TRUE;
			goto out;
		}
		Node = NextDevicePathNode(Node);
	}
out:
	FreePool(unpacked);
	return ret;
}

static EFI_STATUS
generate_next_uri (CONST CHAR8 *current_uri, CONST CHAR8 *next_loader,
		   CHAR8 **uri)
{
	CONST CHAR8 *ptr;
	UINTN next_len;
	UINTN path_len = 0;
	UINTN count = 0;

	if (strncmpa(current_uri, (CHAR8 *)"http://", 7) == 0) {
		ptr = current_uri + 7;
		count += 7;
	} else if (strncmpa(current_uri, (CHAR8 *)"https://", 8) == 0) {
		ptr = current_uri + 8;
		count += 8;
	} else {
		return EFI_INVALID_PARAMETER;
	}

	/* Extract the path */
	next_len = strlena(next_loader);
	while (*ptr != '\0') {
		count++;
		if (*ptr == '/')
			path_len = count;
		ptr++;
	}

	*uri = AllocatePool(sizeof(CHAR8) * (path_len + next_len + 1));
	if (!*uri)
		return EFI_OUT_OF_RESOURCES;

	CopyMem(*uri, current_uri, path_len);
	CopyMem(*uri + path_len, next_loader, next_len);
	(*uri)[path_len + next_len] = '\0';

	return EFI_SUCCESS;
}

static EFI_STATUS
extract_hostname (CONST CHAR8 *url, CHAR8 **hostname)
{
	CONST CHAR8 *ptr, *start;
	UINTN host_len = 0;

	if (strncmpa(url, (CHAR8 *)"http://", 7) == 0)
		start = url + 7;
	else if (strncmpa(url, (CHAR8 *)"https://", 8) == 0)
		start = url + 8;
	else
		return EFI_INVALID_PARAMETER;

	ptr = start;
	while (*ptr != '/' && *ptr != '\0') {
		host_len++;
		ptr++;
	}

	*hostname = AllocatePool(sizeof(CHAR8) * (host_len + 1));
	if (!*hostname)
		return EFI_OUT_OF_RESOURCES;

	CopyMem(*hostname, start, host_len);
	(*hostname)[host_len] = '\0';

	return EFI_SUCCESS;
}

#define SAME_MAC_ADDR(a, b) (!CompareMem(a, b, sizeof(EFI_MAC_ADDRESS)))

static EFI_HANDLE
get_nic_handle (EFI_MAC_ADDRESS *mac)
{
	EFI_DEVICE_PATH *unpacked = NULL;
	EFI_DEVICE_PATH *Node;
	EFI_DEVICE_PATH *temp_path = NULL;
	MAC_ADDR_DEVICE_PATH *MacNode;
	EFI_HANDLE handle = NULL;
	EFI_HANDLE *buffer;
	UINTN NoHandles;
	UINTN i;
	EFI_STATUS efi_status;

	/* Get the list of handles that support the HTTP service binding
	   protocol */
	efi_status = gBS->LocateHandleBuffer(ByProtocol,
					     &EFI_HTTP_BINDING_GUID,
					     NULL, &NoHandles, &buffer);
	if (EFI_ERROR(efi_status))
		return NULL;

	for (i = 0; i < NoHandles; i++) {
		temp_path = DevicePathFromHandle(buffer[i]);

		/* Match the MAC address */
		unpacked = UnpackDevicePath(temp_path);
		if (!unpacked) {
			perror(L"Failed to unpack device path\n");
			continue;
		}
		Node = unpacked;
		while (!IsDevicePathEnd(Node)) {
			if (DevicePathType(Node) == MESSAGING_DEVICE_PATH &&
			    DevicePathSubType(Node) == MSG_MAC_ADDR_DP) {
				MacNode = (MAC_ADDR_DEVICE_PATH *)Node;
				if (SAME_MAC_ADDR(mac, &MacNode->MacAddress)) {
					handle = buffer[i];
					goto out;
				}
			}
			Node = NextDevicePathNode(Node);
		}
		FreePool(unpacked);
		unpacked = NULL;
	}

out:
	if (unpacked)
		FreePool(unpacked);
	FreePool(buffer);

	return handle;
}

static BOOLEAN
is_unspecified_addr (EFI_IPv6_ADDRESS ip6)
{
	UINT8 i;

	for (i = 0; i<16; i++) {
		if (ip6.Addr[i] != 0)
			return FALSE;
	}

	return TRUE;
}

static EFI_STATUS
set_ip6(EFI_HANDLE *nic, IPv6_DEVICE_PATH *ip6node)
{
	EFI_IP6_CONFIG_PROTOCOL *ip6cfg;
	EFI_IP6_CONFIG_MANUAL_ADDRESS ip6;
	EFI_IPv6_ADDRESS gateway;
	EFI_STATUS efi_status;

	efi_status = gBS->HandleProtocol(nic, &EFI_IP6_CONFIG_GUID,
					 (VOID **)&ip6cfg);
	if (EFI_ERROR(efi_status))
		return efi_status;

	ip6.Address = ip6node->LocalIpAddress;
	ip6.PrefixLength = ip6node->PrefixLength;
	ip6.IsAnycast = FALSE;
	efi_status = ip6cfg->SetData(ip6cfg, Ip6ConfigDataTypeManualAddress,
				     sizeof(ip6), &ip6);
	if (EFI_ERROR(efi_status))
		return efi_status;

	gateway = ip6node->GatewayIpAddress;
	if (is_unspecified_addr(gateway))
		return EFI_SUCCESS;

	efi_status = ip6cfg->SetData(ip6cfg, Ip6ConfigDataTypeGateway,
				     sizeof(gateway), &gateway);
	if (EFI_ERROR(efi_status))
		return efi_status;

	return EFI_SUCCESS;
}

static EFI_STATUS
set_ip4(EFI_HANDLE *nic, IPv4_DEVICE_PATH *ip4node)
{
	EFI_IP4_CONFIG2_PROTOCOL *ip4cfg2;
	EFI_IP4_CONFIG2_MANUAL_ADDRESS ip4;
	EFI_IPv4_ADDRESS gateway;
	EFI_STATUS efi_status;

	efi_status = gBS->HandleProtocol(nic, &EFI_IP4_CONFIG2_GUID,
					 (VOID **)&ip4cfg2);
	if (EFI_ERROR(efi_status))
		return efi_status;

	ip4.Address = ip4node->LocalIpAddress;
	ip4.SubnetMask = ip4node->SubnetMask;
	efi_status = ip4cfg2->SetData(ip4cfg2, Ip4Config2DataTypeManualAddress,
				      sizeof(ip4), &ip4);
	if (EFI_ERROR(efi_status))
		return efi_status;

	gateway = ip4node->GatewayIpAddress;
	efi_status = ip4cfg2->SetData(ip4cfg2, Ip4Config2DataTypeGateway,
				      sizeof(gateway), &gateway);
	if (EFI_ERROR(efi_status))
		return efi_status;

	return EFI_SUCCESS;
}

static VOID EFIAPI
httpnotify (EFI_EVENT Event, VOID *Context)
{
	*((BOOLEAN *) Context) = TRUE;
}

static EFI_STATUS
configure_http (EFI_HTTP_PROTOCOL *http, BOOLEAN is_ip6)
{
	EFI_HTTP_CONFIG_DATA http_mode;
	EFI_HTTPv4_ACCESS_POINT ip4node;
	EFI_HTTPv6_ACCESS_POINT ip6node;

	/* Configure HTTP */
	ZeroMem(&http_mode, sizeof(http_mode));
	http_mode.HttpVersion = HttpVersion11;
	/* use the default time out */
	http_mode.TimeOutMillisec = 0;

	if (!is_ip6) {
		http_mode.LocalAddressIsIPv6 = FALSE;
		ZeroMem(&ip4node, sizeof(ip4node));
		ip4node.UseDefaultAddress = TRUE;
		http_mode.AccessPoint.IPv4Node = &ip4node;
	} else {
		http_mode.LocalAddressIsIPv6 = TRUE;
		ZeroMem(&ip6node, sizeof(ip6node));
		http_mode.AccessPoint.IPv6Node = &ip6node;
	}

	return http->Configure(http, &http_mode);
}

static EFI_STATUS
send_http_request (EFI_HTTP_PROTOCOL *http, CHAR8 *hostname, CHAR8 *uri)
{
	EFI_HTTP_TOKEN tx_token;
	EFI_HTTP_MESSAGE tx_message;
	EFI_HTTP_REQUEST_DATA request;
	EFI_HTTP_HEADER headers[3];
	BOOLEAN request_done;
	CHAR16 *Url = NULL;
	EFI_STATUS efi_status;
	EFI_STATUS event_status;

	/* Convert the ascii string to the UCS2 string */
	Url = PoolPrint(L"%a", uri);
	if (!Url)
		return EFI_OUT_OF_RESOURCES;

	request.Method = HttpMethodGet;
	request.Url = Url;

	/* Prepare the HTTP headers */
	headers[0].FieldName = (CHAR8 *)"Host";
	headers[0].FieldValue = hostname;
	headers[1].FieldName = (CHAR8 *)"Accept";
	headers[1].FieldValue = (CHAR8 *)"*/*";
	headers[2].FieldName = (CHAR8 *)"User-Agent";
	headers[2].FieldValue = (CHAR8 *)"UefiHttpBoot/1.0";

	tx_message.Data.Request = &request;
	tx_message.HeaderCount = 3;
	tx_message.Headers = headers;
	tx_message.BodyLength = 0;
	tx_message.Body = NULL;

	tx_token.Status = EFI_NOT_READY;
	tx_token.Message = &tx_message;
	tx_token.Event = NULL;
	request_done = FALSE;
	efi_status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_NOTIFY,
				      httpnotify, &request_done,
				      &tx_token.Event);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to Create Event for HTTP request: %r\n",
		       efi_status);
		goto no_event;
	}

	/* Send out the request */
	efi_status = http->Request(http, &tx_token);
	if (EFI_ERROR(efi_status)) {
		perror(L"HTTP request failed: %r\n", efi_status);
		goto error;
	}

	/* Wait for the response */
	while (!request_done)
		http->Poll(http);

	if (EFI_ERROR(tx_token.Status)) {
		perror(L"HTTP request: %r\n", tx_token.Status);
		efi_status = tx_token.Status;
	}

error:
	event_status = gBS->CloseEvent(tx_token.Event);
	if (EFI_ERROR(event_status)) {
		perror(L"Failed to close Event for HTTP request: %r\n",
		       event_status);
	}

no_event:
	if (Url)
		FreePool(Url);

	return efi_status;
}

static EFI_STATUS
receive_http_response(EFI_HTTP_PROTOCOL *http, VOID **buffer, UINT64 *buf_size)
{
	EFI_HTTP_TOKEN rx_token;
	EFI_HTTP_MESSAGE rx_message;
	EFI_HTTP_RESPONSE_DATA response;
	EFI_HTTP_STATUS_CODE http_status;
	BOOLEAN response_done;
	UINTN i, downloaded;
	CHAR8 rx_buffer[9216];
	EFI_STATUS efi_status;
	EFI_STATUS event_status;

	/* Initialize the rx message and buffer */
	response.StatusCode = HTTP_STATUS_UNSUPPORTED_STATUS;
	rx_message.Data.Response = &response;
	rx_message.HeaderCount = 0;
	rx_message.Headers = 0;
	rx_message.BodyLength = sizeof(rx_buffer);
	rx_message.Body = rx_buffer;

	rx_token.Status = EFI_NOT_READY;
	rx_token.Message = &rx_message;
	rx_token.Event = NULL;
	response_done = FALSE;
	efi_status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_NOTIFY,
				      httpnotify, &response_done,
				      &rx_token.Event);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to Create Event for HTTP response: %r\n",
		       efi_status);
		goto no_event;
	}

	/* Notify the firmware to receive the HTTP messages */
	efi_status = http->Response(http, &rx_token);
	if (EFI_ERROR(efi_status)) {
		perror(L"HTTP response failed: %r\n", efi_status);
		goto error;
	}

	/* Wait for the response */
	while (!response_done)
		http->Poll(http);

	if (EFI_ERROR(rx_token.Status)) {
		perror(L"HTTP response: %r\n", rx_token.Status);
		efi_status = rx_token.Status;
		goto error;
	}

	/* Check the HTTP status code */
	http_status = rx_token.Message->Data.Response->StatusCode;
	if (http_status != HTTP_STATUS_200_OK) {
		perror(L"HTTP Status Code: %d\n",
		       convert_http_status_code(http_status));
		efi_status = EFI_ABORTED;
		goto error;
	}

	/* Check the length of the file */
	for (i = 0; i < rx_message.HeaderCount; i++) {
		if (!strcmpa(rx_message.Headers[i].FieldName, (CHAR8 *)"Content-Length")) {
			*buf_size = ascii_to_int(rx_message.Headers[i].FieldValue);
		}
	}

	if (*buf_size == 0) {
		perror(L"Failed to get Content-Lenght\n");
		goto error;
	}

	*buffer = AllocatePool(*buf_size);
	if (!*buffer) {
		perror(L"Failed to allocate new rx buffer\n");
		goto error;
	}

	downloaded = rx_message.BodyLength;

	CopyMem(*buffer, rx_buffer, downloaded);

	/* Retreive the rest of the message */
	while (downloaded < *buf_size) {
		if (rx_message.Headers) {
			FreePool(rx_message.Headers);
		}
		rx_message.Headers = NULL;
		rx_message.HeaderCount = 0;
		rx_message.Data.Response = NULL;
		rx_message.BodyLength = sizeof(rx_buffer);
		rx_message.Body = rx_buffer;

		rx_token.Status = EFI_NOT_READY;
		response_done = FALSE;

		efi_status = http->Response(http, &rx_token);
		if (EFI_ERROR(efi_status)) {
			perror(L"HTTP response failed: %r\n", efi_status);
			goto error;
		}

		while (!response_done)
			http->Poll(http);

		if (EFI_ERROR(rx_token.Status)) {
			perror(L"HTTP response: %r\n", rx_token.Status);
			efi_status = rx_token.Status;
			goto error;
		}

		if (rx_message.BodyLength + downloaded > *buf_size) {
			efi_status = EFI_BAD_BUFFER_SIZE;
			goto error;
		}

		CopyMem(*buffer + downloaded, rx_buffer, rx_message.BodyLength);

		downloaded += rx_message.BodyLength;
	}

error:
	event_status = gBS->CloseEvent(rx_token.Event);
	if (EFI_ERROR(event_status)) {
		perror(L"Failed to close Event for HTTP response: %r\n",
		       event_status);
	}

no_event:
	if (EFI_ERROR(efi_status) && *buffer)
		FreePool(*buffer);

	return efi_status;
}

static EFI_STATUS
http_fetch (EFI_HANDLE image, EFI_HANDLE device,
	    CHAR8 *hostname, CHAR8 *uri, BOOLEAN is_ip6,
	    VOID **buffer, UINT64 *buf_size)
{
	EFI_SERVICE_BINDING *service;
	EFI_HANDLE http_handle;
	EFI_HTTP_PROTOCOL *http;
	EFI_STATUS efi_status;
	EFI_STATUS child_status;

	*buffer = NULL;
	*buf_size = 0;

	/* Open HTTP Service Binding Protocol */
	efi_status = gBS->OpenProtocol(device, &EFI_HTTP_BINDING_GUID,
				       (VOID **) &service, image, NULL,
				       EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efi_status))
		return efi_status;

	/* Create the ChildHandle from the Service Binding */
	/* Set the handle to NULL to request a new handle */
	http_handle = NULL;
	efi_status = service->CreateChild(service, &http_handle);
	if (EFI_ERROR(efi_status))
		return efi_status;

	/* Get the http protocol */
	efi_status = gBS->HandleProtocol(http_handle, &EFI_HTTP_PROTOCOL_GUID,
					 (VOID **) &http);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to get http\n");
		goto error;
	}

	efi_status = configure_http(http, is_ip6);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to configure http: %r\n", efi_status);
		goto error;
	}

	efi_status = send_http_request(http, hostname, uri);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to send HTTP request: %r\n", efi_status);
		goto error;
	}

	efi_status = receive_http_response(http, buffer, buf_size);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to receive HTTP response: %r\n", efi_status);
		goto error;
	}

error:
	child_status = service->DestroyChild(service, http_handle);
	if (EFI_ERROR(efi_status)) {
		return efi_status;
	} else if (EFI_ERROR(child_status)) {
		return child_status;
	}

	return EFI_SUCCESS;
}

EFI_STATUS
httpboot_fetch_buffer (EFI_HANDLE image, VOID **buffer, UINT64 *buf_size)
{
	EFI_STATUS efi_status;
	EFI_HANDLE nic;
	CHAR8 *next_loader = NULL;
	CHAR8 *next_uri = NULL;
	CHAR8 *hostname = NULL;

	if (!uri)
		return EFI_NOT_READY;

	next_loader = translate_slashes(DEFAULT_LOADER_CHAR);

	/* Create the URI for the next loader based on the original URI */
	efi_status = generate_next_uri(uri, next_loader, &next_uri);
	if (EFI_ERROR(efi_status)) {
		perror(L"Next URI: %a, %r\n", next_uri, efi_status);
		goto error;
	}

	/* Extract the hostname (or IP) from URI */
	efi_status = extract_hostname(uri, &hostname);
	if (EFI_ERROR(efi_status)) {
		perror(L"hostname: %a, %r\n", hostname, efi_status);
		goto error;
	}

	/* Get the handle that associates with the NIC we are using and
	   also supports the HTTP service binding protocol */
	nic = get_nic_handle(&mac_addr);
	if (!nic) {
		goto error;
	}

	/* UEFI stops DHCP after fetching the image and stores the related
	   information in the device path node. We have to set up the
	   connection on our own for the further operations. */
	if (!is_ip6)
		efi_status = set_ip4(nic, &ip4_node);
	else
		efi_status = set_ip6(nic, &ip6_node);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to set IP for HTTPBoot: %r\n", efi_status);
		goto error;
	}

	/* Use HTTP protocl to fetch the remote file */
	efi_status = http_fetch (image, nic, hostname, next_uri, is_ip6,
				 buffer, buf_size);
	if (EFI_ERROR(efi_status)) {
		perror(L"Failed to fetch image: %r\n", efi_status);
		goto error;
	}

error:
	FreePool(uri);
	uri = NULL;
	if (next_uri)
		FreePool(next_uri);
	if (hostname)
		FreePool(hostname);

	return efi_status;
}
