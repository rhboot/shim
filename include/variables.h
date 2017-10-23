#ifndef SHIM_VARIABLES_H
#define SHIM_VARIABLES_H

#include <efiauthenticated.h>
#include <PeImage.h>		/* for SHA256_DIGEST_SIZE */

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
SetSecureVariable(CHAR16 *var, UINT8 *Data, UINTN len, EFI_GUID owner, UINT32 options, int createtimebased);
EFI_STATUS
get_variable(CHAR16 *var, UINT8 **data, UINTN *len, EFI_GUID owner);
EFI_STATUS
get_variable_attr(CHAR16 *var, UINT8 **data, UINTN *len, EFI_GUID owner,
		  UINT32 *attributes);
EFI_STATUS
find_in_esl(UINT8 *Data, UINTN DataSize, UINT8 *key, UINTN keylen);
EFI_STATUS
find_in_variable_esl(CHAR16* var, EFI_GUID owner, UINT8 *key, UINTN keylen);

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
variable_enroll_hash(CHAR16 *var, EFI_GUID owner,
		     UINT8 hash[SHA256_DIGEST_SIZE]);
EFI_STATUS
variable_create_esl(void *cert, int cert_len, EFI_GUID *type, EFI_GUID *owner,
		    void **out, int *outlen);

#endif /* SHIM_VARIABLES_H */
