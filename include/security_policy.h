typedef EFI_STATUS (*SecurityHook) (void *data, UINT32 len);

EFI_STATUS
security_policy_install(SecurityHook authentication);
EFI_STATUS
security_policy_uninstall(void);
void
security_protocol_set_hashes(unsigned char *esl, int len);
