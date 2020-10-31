/* ====================================================================
 * Copyright (c) 2003 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <openssl/opensslconf.h>
#include <openssl/evp.h>

#ifndef OPENSSL_FIPS
# error FIPS is disabled.
#endif

#ifdef OPENSSL_FIPS

int FIPS_module_mode_set(int onoff);
int FIPS_module_mode(void);
int FIPS_module_installed(void);
int FIPS_selftest_sha1(void);
int FIPS_selftest_sha2(void);
int FIPS_selftest_sha3(void);
int FIPS_selftest_aes_ccm(void);
int FIPS_selftest_aes_gcm(void);
int FIPS_selftest_aes_xts(void);
int FIPS_selftest_aes(void);
int FIPS_selftest_des(void);
int FIPS_selftest_rsa(void);
int FIPS_selftest_dsa(void);
int FIPS_selftest_ecdsa(void);
int FIPS_selftest_ecdh(void);
int FIPS_selftest_dh(void);
void FIPS_drbg_stick(int onoff);
int FIPS_selftest_hmac(void);
int FIPS_selftest_drbg(void);
int FIPS_selftest_cmac(void);

int fips_in_post(void);

int fips_pkey_signature_test(EVP_PKEY *pkey,
                                 const unsigned char *tbs, int tbslen,
                                 const unsigned char *kat,
                                 unsigned int katlen,
                                 const EVP_MD *digest,
                                 unsigned int md_flags, const char *fail_str);

int fips_cipher_test(EVP_CIPHER_CTX *ctx,
                         const EVP_CIPHER *cipher,
                         const unsigned char *key,
                         const unsigned char *iv,
                         const unsigned char *plaintext,
                         const unsigned char *ciphertext, int len);

void fips_set_selftest_fail(void);

void FIPS_get_timevec(unsigned char *buf, unsigned long *pctr);

#endif
