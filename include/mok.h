// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * mok.h - structs for MoK data
 * Copyright Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_MOK_H_
#define SHIM_MOK_H_

#include "shim.h"

typedef enum {
	VENDOR_ADDEND_DB,
	VENDOR_ADDEND_X509,
	VENDOR_ADDEND_NONE,
} vendor_addend_category_t;

struct mok_state_variable;
typedef vendor_addend_category_t (vendor_addend_categorizer_t)(struct mok_state_variable *);

/*
 * MoK variables that need to have their storage validated.
 *
 * The order here is important, since this is where we measure for the
 * tpm as well.
 */
struct mok_state_variable {
	CHAR16 *name;	/* UCS-2 BS|NV variable name */
	char *name8;	/* UTF-8 BS|NV variable name */
	CHAR16 *rtname;	/* UCS-2 RT variable name */
	char *rtname8;	/* UTF-8 RT variable name */
	EFI_GUID *guid;	/* variable GUID */

	/*
	 * these are used during processing, they shouldn't be filled out
	 * in the static table below.
	 */
	UINT8 *data;
	UINTN data_size;

	/*
	 * addend are added to the input variable, as part of the runtime
	 * variable, so that they're visible to the kernel.  These are
	 * where we put vendor_cert / vendor_db / vendor_dbx
	 *
	 * These are indirect pointers just to make initialization saner...
	 */
	vendor_addend_categorizer_t *categorize_addend; /* determines format */
	/*
	 * we call categorize_addend() and it determines what kind of thing
	 * this is.  That is, if this shim was built with VENDOR_CERT, for
	 * the DB entry it'll return VENDOR_ADDEND_X509; if you used
	 * VENDOR_DB instead, it'll return VENDOR_ADDEND_DB.  If you used
	 * neither, it'll do VENDOR_ADDEND_NONE.
	 *
	 * The existing categorizers are for db and dbx; they differ
	 * because we don't currently support a CERT for dbx.
	 */
	UINT8 **addend;
	UINT32 *addend_size;

	UINT8 **user_cert;
	UINT32 *user_cert_size;

	/*
	 * build_cert is our build-time cert.  Like addend, this is added
	 * to the input variable, as part of the runtime variable, so that
	 * they're visible to the kernel.  This is the ephemeral cert used
	 * for signing MokManager.efi and fallback.efi.
	 *
	 * These are indirect pointers just to make initialization saner...
	 */
	UINT8 **build_cert;
	UINT32 *build_cert_size;

	UINT32 yes_attr;	/* var attrs that must be set */
	UINT32 no_attr;		/* var attrs that must not be set */
	UINT32 flags;		/* flags on what and how to mirror */
	/*
	 * MOK_MIRROR_KEYDB	    mirror this as a key database
	 * MOK_MIRROR_DELETE_FIRST  delete any existing variable first
	 * MOK_VARIABLE_MEASURE	    extend PCR 7 and log the hash change
	 * MOK_VARIABLE_LOG	    measure into whatever .pcr says and log
	 */
	UINTN pcr;		/* PCR to measure and hash to */

	/*
	 * if this is a state value, a pointer to our internal state to be
	 * mirrored.
	 */
	UINT8 *state;
};

extern size_t n_mok_state_variables;
extern struct mok_state_variable *mok_state_variables;

struct mok_variable_config_entry {
	CHAR8 name[256];
	UINT64 data_size;
	UINT8 data[];
};

/*
 * bit definitions for MokPolicy
 */
#define MOK_POLICY_REQUIRE_NX	1

#endif /* !SHIM_MOK_H_ */
// vim:fenc=utf-8:tw=75:noet
