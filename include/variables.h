// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SHIM_VARIABLES_H
#define SHIM_VARIABLES_H

#include "efiauthenticated.h"
#include "peimage.h"		/* for SHA256_DIGEST_SIZE */

#define certlist_for_each_certentry(cl, cl_init, s, s_init)		\
	for (cl = (EFI_SIGNATURE_LIST *)(cl_init), s = (s_init);	\
		s > 0 && s >= cl->SignatureListSize;			\
		s -= cl->SignatureListSize,				\
		cl = (EFI_SIGNATURE_LIST *) ((UINT8 *)cl + cl->SignatureListSize))

/*
 * Warning: this assumes (cl)->SignatureHeaderSize is zero.  It is for all
 * the signatures we process (X509, RSA2048, SHA256)
 */
#define certentry_for_each_cert(c, cl)	\
  for (c = (EFI_SIGNATURE_DATA *)((UINT8 *) (cl) + sizeof(EFI_SIGNATURE_LIST) + (cl)->SignatureHeaderSize); \
	(UINT8 *)c < ((UINT8 *)(cl)) + (cl)->SignatureListSize; \
	c = (EFI_SIGNATURE_DATA *)((UINT8 *)c + (cl)->SignatureSize))

EFI_STATUS
CreatePkX509SignatureList (
  IN	UINT8			    *X509Data,
  IN	UINTN			    X509DataSize,
  IN	EFI_GUID		    owner,
  OUT   EFI_SIGNATURE_LIST          **PkCert
			   );
EFI_STATUS
CreateTimeBasedPayload (
  IN OUT UINTN            *DataSize,
  IN OUT UINT8            **Data
			);
EFI_STATUS
SetSecureVariable(const CHAR16 * const var, UINT8 *Data, UINTN len, EFI_GUID owner, UINT32 options, int createtimebased);
EFI_STATUS
get_variable(const CHAR16 * const var, UINT8 **data, UINTN *len, EFI_GUID owner);
EFI_STATUS
get_variable_attr(const CHAR16 * const var, UINT8 **data, UINTN *len, EFI_GUID owner, UINT32 *attributes);
EFI_STATUS
get_variable_size(const CHAR16 * const var, EFI_GUID owner, UINTN *lenp);
EFI_STATUS
set_variable(CHAR16 *var, EFI_GUID owner, UINT32 attributes, UINTN datasize, void *data);
EFI_STATUS
del_variable(CHAR16 *var, EFI_GUID owner);
EFI_STATUS
find_in_esl(UINT8 *Data, UINTN DataSize, UINT8 *key, UINTN keylen);
EFI_STATUS
find_in_variable_esl(const CHAR16 * const var, EFI_GUID owner, UINT8 *key, UINTN keylen);

#define EFI_OS_INDICATIONS_BOOT_TO_FW_UI 0x0000000000000001

UINT64
GetOSIndications(void);
EFI_STATUS
SETOSIndicationsAndReboot(UINT64 indications);
int
variable_is_secureboot(void);
int
variable_is_setupmode(int default_return);
EFI_STATUS
variable_enroll_hash(const CHAR16 * const var, EFI_GUID owner,
		     UINT8 hash[SHA256_DIGEST_SIZE]);
EFI_STATUS
variable_create_esl(const EFI_SIGNATURE_DATA *first_sig, const size_t howmany,
		    const EFI_GUID *type, const UINT32 sig_size,
		    uint8_t **out, size_t *outlen);
EFI_STATUS
variable_create_esl_with_one_signature(const uint8_t* data, const size_t data_len,
				       const EFI_GUID *type, const EFI_GUID *owner,
				       uint8_t **out, size_t *outlen);
EFI_STATUS
fill_esl(const EFI_SIGNATURE_DATA *first_sig, const size_t howmany,
	 const EFI_GUID *type, const UINT32 sig_size,
	 uint8_t *out, size_t *outlen);
EFI_STATUS
fill_esl_with_one_signature(const uint8_t *data, const uint32_t data_len,
			    const EFI_GUID *type, const EFI_GUID *owner,
			    uint8_t *out, size_t *outlen);

#endif /* SHIM_VARIABLES_H */
