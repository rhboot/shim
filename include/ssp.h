#ifndef SSP_H_
#define SSP_H_

#define SSPVER_VAR_NAME L"SkuSiPolicyVersion"
#define SSPSIG_VAR_NAME L"SkuSiPolicyUpdateSigners"
#define SSP_VAR_ATTRS UEFI_VAR_NV_BS

#define SSPVER_SIZE 8
#define SSPSIG_SIZE 131

EFI_STATUS set_ssp_uefi_variable_internal(void);
EFI_STATUS set_ssp_uefi_variable(uint8_t*, uint8_t*, uint8_t*, uint8_t*);

#endif /* !SSP_H_ */
