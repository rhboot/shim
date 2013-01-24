#include <efi.h>
#include <efilib.h>
#include <Library/BaseCryptLib.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include "PasswordCrypt.h"


UINT16 get_hash_size (const UINT16 method)
{
	switch (method) {
	case TRANDITIONAL_DES:
		return 64 / 8; /* per "man crypt" */
	case EXTEND_BSDI_DES:
		return 64 / 8; /* per "man crypt" */
	case MD5_BASED:
		return MD5_DIGEST_LENGTH;
	case SHA256_BASED:
		return SHA256_DIGEST_LENGTH;
	case SHA512_BASED:
		return SHA512_DIGEST_LENGTH;
	case BLOWFISH_BASED:
		return 184 / 8; /* per "man crypt" */
	}

	return 0;
}

static EFI_STATUS sha256_crypt (const char *key,  UINT32 key_len,
				const char *salt, UINT32 salt_size,
				const UINT32 rounds, UINT8 *hash)
{
	SHA256_CTX ctx, alt_ctx;
	UINT8 alt_result[SHA256_DIGEST_SIZE];
	UINT8 tmp_result[SHA256_DIGEST_SIZE];
	UINT8 *cp, *p_bytes, *s_bytes;
	UINTN cnt;

	SHA256_Init(&ctx);
	SHA256_Update(&ctx, key, key_len);
	SHA256_Update(&ctx, salt, salt_size);

	SHA256_Init(&alt_ctx);
	SHA256_Update(&alt_ctx, key, key_len);
	SHA256_Update(&alt_ctx, salt, salt_size);
	SHA256_Update(&alt_ctx, key, key_len);
	SHA256_Final(alt_result, &alt_ctx);

	for (cnt = key_len; cnt > 32; cnt -= 32)
		SHA256_Update(&ctx, alt_result, 32);
	SHA256_Update(&ctx, alt_result, cnt);

	for (cnt = key_len; cnt > 0; cnt >>= 1) {
		if ((cnt & 1) != 0) {
			SHA256_Update(&ctx, alt_result, 32);
		} else {
			SHA256_Update(&ctx, key, key_len);
		}
	}
	SHA256_Final(alt_result, &ctx);

	SHA256_Init(&alt_ctx);
	for (cnt = 0; cnt < key_len; ++cnt)
		SHA256_Update(&alt_ctx, key, key_len);
	SHA256_Final(tmp_result, &alt_ctx);

	cp = p_bytes = AllocatePool(key_len);
	for (cnt = key_len; cnt >= 32; cnt -= 32) {
		CopyMem(cp, tmp_result, 32);
		cp += 32;
	}
	CopyMem(cp, tmp_result, cnt);

	SHA256_Init(&alt_ctx);
	for (cnt = 0; cnt < 16 + alt_result[0]; ++cnt)
		SHA256_Update(&alt_ctx, salt, salt_size);
	SHA256_Final(tmp_result, &alt_ctx);

	cp = s_bytes = AllocatePool(salt_size);
	for (cnt = salt_size; cnt >= 32; cnt -= 32) {
		CopyMem(cp, tmp_result, 32);
		cp += 32;
	}
	CopyMem(cp, tmp_result, cnt);

	for (cnt = 0; cnt < rounds; ++cnt) {
		SHA256_Init(&ctx);

		if ((cnt & 1) != 0)
			SHA256_Update(&ctx, p_bytes, key_len);
		else
			SHA256_Update(&ctx, alt_result, 32);

		if (cnt % 3 != 0)
			SHA256_Update(&ctx, s_bytes, salt_size);

		if (cnt % 7 != 0)
			SHA256_Update(&ctx, p_bytes, key_len);

		if ((cnt & 1) != 0)
			SHA256_Update(&ctx, alt_result, 32);
		else
			SHA256_Update(&ctx, p_bytes, key_len);

		SHA256_Final(alt_result, &ctx);
	}

	CopyMem(hash, alt_result, SHA256_DIGEST_SIZE);

	FreePool(p_bytes);
	FreePool(s_bytes);

	return EFI_SUCCESS;
}

static EFI_STATUS sha512_crypt (const char *key,  UINT32 key_len,
				const char *salt, UINT32 salt_size,
				const UINT32 rounds, UINT8 *hash)
{
	SHA512_CTX ctx, alt_ctx;
	UINT8 alt_result[SHA512_DIGEST_LENGTH];
	UINT8 tmp_result[SHA512_DIGEST_LENGTH];
	UINT8 *cp, *p_bytes, *s_bytes;
	UINTN cnt;

	SHA512_Init(&ctx);
	SHA512_Update(&ctx, key, key_len);
	SHA512_Update(&ctx, salt, salt_size);

	SHA512_Init(&alt_ctx);
	SHA512_Update(&alt_ctx, key, key_len);
	SHA512_Update(&alt_ctx, salt, salt_size);
	SHA512_Update(&alt_ctx, key, key_len);

	SHA512_Final(alt_result, &alt_ctx);

	for (cnt = key_len; cnt > 64; cnt -= 64)
		SHA512_Update(&ctx, alt_result, 64);
	SHA512_Update(&ctx, alt_result, cnt);

	for (cnt = key_len; cnt > 0; cnt >>= 1) {
		if ((cnt & 1) != 0) {
			SHA512_Update(&ctx, alt_result, 64);
		} else {
			SHA512_Update(&ctx, key, key_len);
		}
	}
	SHA512_Final(alt_result, &ctx);

	SHA512_Init(&alt_ctx);
	for (cnt = 0; cnt < key_len; ++cnt)
		SHA512_Update(&alt_ctx, key, key_len);
	SHA512_Final(tmp_result, &alt_ctx);

	cp = p_bytes = AllocatePool(key_len);
	for (cnt = key_len; cnt >= 64; cnt -= 64) {
		CopyMem(cp, tmp_result, 64);
		cp += 64;
	}
	CopyMem(cp, tmp_result, cnt);

	SHA512_Init(&alt_ctx);
	for (cnt = 0; cnt < 16 + alt_result[0]; ++cnt)
		SHA512_Update(&alt_ctx, salt, salt_size);
	SHA512_Final(tmp_result, &alt_ctx);

	cp = s_bytes = AllocatePool(salt_size);
	for (cnt = salt_size; cnt >= 64; cnt -= 64) {
		CopyMem(cp, tmp_result, 64);
		cp += 64;
	}
	CopyMem(cp, tmp_result, cnt);

	for (cnt = 0; cnt < rounds; ++cnt) {
		SHA512_Init(&ctx);

		if ((cnt & 1) != 0)
			SHA512_Update(&ctx, p_bytes, key_len);
		else
			SHA512_Update(&ctx, alt_result, 64);

		if (cnt % 3 != 0)
			SHA512_Update(&ctx, s_bytes, salt_size);

		if (cnt % 7 != 0)
			SHA512_Update(&ctx, p_bytes, key_len);

		if ((cnt & 1) != 0)
			SHA512_Update(&ctx, alt_result, 64);
		else
			SHA512_Update(&ctx, p_bytes, key_len);

		SHA512_Final(alt_result, &ctx);
	}

	CopyMem(hash, alt_result, SHA512_DIGEST_LENGTH);

	FreePool(p_bytes);
	FreePool(s_bytes);

	return EFI_SUCCESS;
}

EFI_STATUS password_crypt (const char *password, UINT32 pw_length,
			   const PASSWORD_CRYPT *pw_crypt, UINT8 *hash)
{
	EFI_STATUS status;

	if (!pw_crypt)
		return EFI_INVALID_PARAMETER;

	switch (pw_crypt->method) {
	case TRANDITIONAL_DES:
	case EXTEND_BSDI_DES:
	case MD5_BASED:
		/* TODO unsupported */
		status = EFI_UNSUPPORTED;
		break;
	case SHA256_BASED:
		status = sha256_crypt(password, pw_length, (char *)pw_crypt->salt,
				      pw_crypt->salt_size, pw_crypt->iter_count,
				      hash);
		break;
	case SHA512_BASED:
		status = sha512_crypt(password, pw_length, (char *)pw_crypt->salt,
				      pw_crypt->salt_size, pw_crypt->iter_count,
				      hash);
		break;
	case BLOWFISH_BASED:
		/* TODO unsupported */
		status = EFI_UNSUPPORTED;
		break;
	default:
		return EFI_INVALID_PARAMETER;
	}

	return status;
}
