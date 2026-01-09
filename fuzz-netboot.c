// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * fuzz-netboot.c - fuzz TFTP netboot code.
 */
#include <stdint.h>
#include <stdio.h>

#ifndef SHIM_UNIT_TEST
#define SHIM_UNIT_TEST
#endif

#include "shim.h"

extern EFI_PXE_BASE_CODE *pxe;
extern CHAR8 *full_path;

UINT8 mok_policy = 0;
UINTN hsi_status = 0;

/* A struct to track fuzzing input bytes */
typedef struct {
	const uint8_t *data;
	size_t len;
} state_t;

/* Consumes `len` fuzzing input bytes into `dst` */
static int
fuzzer_consume_bytes(state_t *state, void *dst, size_t len)
{
	if (state->len < len)
		return 1;

	memcpy(dst, state->data, len);
	state->data += len;
	state->len -= len;
	return 0;
}

/* Returns a random length of bytes that `state` is guaranteed to
 * be able to satisfy */
static int
fuzzer_consume_len(state_t *state, size_t *len)
{
	if (state->len <= sizeof(*len))
		return 1;

	fuzzer_consume_bytes(state, len, sizeof(*len));
	*len %= state->len;

	return 0;
}

/* Consumes a `BOOLEAN` from the fuzzing input bytes */
static int
fuzzer_consume_bool(state_t *state, BOOLEAN *b)
{
	int ret, val = 0;

	ret = fuzzer_consume_bytes(state, &val, 1);
	if (!ret)
		*b = val & 1;
	return ret;
}

/* Global fuzzing state, set from LLVMFuzzerTestOneInput() so that
 * mtftp_xfer() can access input bytes */
static state_t *gstate = NULL;

static EFI_STATUS EFIAPI
mtftp_xfer(struct _EFI_PXE_BASE_CODE_PROTOCOL *pxe,
           EFI_PXE_BASE_CODE_TFTP_OPCODE op, VOID *buf,
           BOOLEAN overwrite UNUSED, UINT64 *bufsize, UINT64 *blocksize UNUSED,
           EFI_IP_ADDRESS *addr UNUSED, UINT8 *filename UNUSED,
           EFI_PXE_BASE_CODE_MTFTP_INFO *info UNUSED, BOOLEAN dontusebuf UNUSED)
{
	EFI_STATUS status;
	size_t size;
	unsigned int i;
	EFI_PXE_BASE_CODE_TFTP_ERROR *error;
	uint8_t c;

	if (op != EFI_PXE_BASE_CODE_TFTP_READ_FILE) {
		status = EFI_UNSUPPORTED;
		goto out_err;
	}

	if (fuzzer_consume_len(gstate, &size)) {
		status = EFI_TFTP_ERROR;
		goto out_err;
	}

	if (*bufsize < size) {
		status = EFI_BUFFER_TOO_SMALL;
		goto out_err;
	}

	fuzzer_consume_bytes(gstate, buf, size);

	*bufsize = size;
	return EFI_SUCCESS;

out_err:
	pxe->Mode->TftpErrorReceived = 1;
	error = &pxe->Mode->TftpError;
	for (i = 0;
	     i < sizeof(error->ErrorString) / sizeof(error->ErrorString[0]);
	     ++i) {
		if (fuzzer_consume_bytes(gstate, &c, sizeof(c)))
			error->ErrorString[i] = c;
	}
	return status;
}

static int
fuzzer_init_mode(state_t *state, EFI_PXE_BASE_CODE_MODE *mode)
{
#define FUZZ_GET_BOOL(_state, _dst)                         \
	do {                                                \
		if (fuzzer_consume_bool(_state, _dst) != 0) \
			goto out;                           \
	} while (0)

#define FUZZ_GET_BYTES(_state, _dst)                                        \
	do {                                                                \
		if (fuzzer_consume_bytes(_state, _dst, sizeof(*_dst)) != 0) \
			goto out;                                           \
	} while (0)

	memset(mode, 0, sizeof(*mode));

	FUZZ_GET_BOOL(state, &mode->DhcpAckReceived);
	FUZZ_GET_BOOL(state, &mode->ProxyOfferReceived);
	FUZZ_GET_BOOL(state, &mode->PxeReplyReceived);
	FUZZ_GET_BOOL(state, &mode->UsingIpv6);

	if (mode->UsingIpv6) {
		FUZZ_GET_BYTES(state, &mode->DhcpAck.Dhcpv6);
		FUZZ_GET_BYTES(state, &mode->PxeReply.Dhcpv6);
		FUZZ_GET_BYTES(state, &mode->ProxyOffer.Dhcpv6);
	} else {
		FUZZ_GET_BYTES(state, &mode->DhcpAck.Dhcpv4);
		FUZZ_GET_BYTES(state, &mode->PxeReply.Dhcpv4);
		FUZZ_GET_BYTES(state, &mode->ProxyOffer.Dhcpv4);
	}

	return 0;

out:
	return -1;
}

static char *
fuzzer_create_name(state_t *state)
{
	char *name;
	size_t name_len = 0;

	if (fuzzer_consume_len(state, &name_len) || !name_len)
		return NULL;

	name = calloc(1, name_len);
	if (name)
		fuzzer_consume_bytes(state, name, name_len - 1);

	return name;
}

static int
fuzzer_main(state_t *state)
{
	EFI_STATUS status;
	char *name = NULL, *netbootname;
	void *sourcebuffer = NULL;
	UINT64 sourcesize;

	if (fuzzer_init_mode(state, pxe->Mode))
		return -1;

	name = fuzzer_create_name(state);
	netbootname = name ? name : "boot64.efi";

	status = parseNetbootinfo(NULL, netbootname);
	if (EFI_ERROR(status))
		goto out;

	status = FetchNetbootimage(NULL, &sourcebuffer, &sourcesize, 0);
	if (!EFI_ERROR(status))
		FreePool(sourcebuffer);

out:
	if (full_path) {
		FreePool(full_path);
		full_path = NULL;
	}
	if (name)
		free(name);
	return 0;
}

static EFI_PXE_BASE_CODE_MODE fuzz_mode = { 0 };

static EFI_PXE_BASE_CODE fuzz_pxe = {
	.Mtftp = mtftp_xfer,
	.Mode = &fuzz_mode,
};

int
LLVMFuzzerTestOneInput(const UINT8 *data, size_t size)
{
	state_t state = { .data = data, .len = size };
	pxe = &fuzz_pxe;
	gstate = &state;
	return fuzzer_main(&state);
}
