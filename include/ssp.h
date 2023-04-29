#ifndef SSP_H_
#define SSP_H_

#define SSPVER_VAR_NAME L"SkuSiPolicyVersion"
#define SSPSIG_VAR_NAME L"SkuSiPolicyUpdateSigners"
/* #define SSP_GUID "77fa9abd-0359-4d32-bd60-28f4e78f784b" XXX */
#define SSP_VAR_ATTRS UEFI_VAR_NV_BS

EFI_STATUS set_ssp_uefi_variable_internal(void);
EFI_STATUS set_ssp_uefi_variable(uint8_t*, uint8_t*, uint8_t*, uint8_t*);

#endif /* !SSP_H_ */
