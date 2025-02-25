// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * globals.c - global shim state
 * Copyright Peter Jones <pjones@redhat.com>
 */

#include "shim.h"

UINT32 vendor_authorized_size = 0;
UINT8 *vendor_authorized = NULL;

UINT32 vendor_deauthorized_size = 0;
UINT8 *vendor_deauthorized = NULL;

UINT32 user_cert_size;
UINT8 *user_cert;

#if defined(ENABLE_SHIM_CERT)
UINT32 build_cert_size;
UINT8 *build_cert;
#endif /* defined(ENABLE_SHIM_CERT) */

/*
 * indicator of how an image has been verified
 */
verification_method_t verification_method;

SHIM_IMAGE_LOADER shim_image_loader_interface;

UINT8 user_insecure_mode;
UINTN hsi_status = 0;
UINT8 ignore_db;
UINT8 trust_mok_list;
UINT8 mok_policy = 0;

UINT32 verbose = 0;

EFI_PHYSICAL_ADDRESS mok_config_table = 0;
UINTN mok_config_table_pages = 0;

// vim:fenc=utf-8:tw=75:noet
