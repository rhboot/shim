#ifndef _SHIM_LIB_SECURITY_POLICY_H
#define _SHIM_LIB_SECURITY_POLICY_H 1

#if defined(OVERRIDE_SECURITY_POLICY)
typedef EFI_STATUS (*SecurityHook) (void *data, UINT32 len);

EFI_STATUS
security_policy_install(SecurityHook authentication);
EFI_STATUS
security_policy_uninstall(void);
void
security_protocol_set_hashes(unsigned char *esl, int len);
#endif /* OVERRIDE_SECURITY_POLICY */

#endif /* SHIM_LIB_SECURITY_POLICY_H */
