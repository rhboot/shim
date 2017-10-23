#ifndef SHIM_PASSWORDCRYPT_H
#define SHIM_PASSWORDCRYPT_H

enum HashMethod {
	TRADITIONAL_DES = 0,
	EXTEND_BSDI_DES,
	MD5_BASED,
	SHA256_BASED,
	SHA512_BASED,
	BLOWFISH_BASED
};

typedef struct {
	UINT16 method;
	UINT64 iter_count;
	UINT16 salt_size;
	UINT8  salt[32];
	UINT8  hash[128];
} __attribute__ ((packed)) PASSWORD_CRYPT;

#define PASSWORD_CRYPT_SIZE sizeof(PASSWORD_CRYPT)

EFI_STATUS password_crypt (const char *password, UINT32 pw_length,
			   const PASSWORD_CRYPT *pw_hash, UINT8 *hash);
UINT16 get_hash_size (const UINT16 method);

#endif /* SHIM_PASSWORDCRYPT_H */
