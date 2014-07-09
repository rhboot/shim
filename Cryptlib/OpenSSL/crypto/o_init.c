/* o_init.c */
/* Written by Dr Stephen N Henson (steve@openssl.org) for the OpenSSL
 * project.
 */
/* ====================================================================
 * Copyright (c) 2007 The OpenSSL Project.  All rights reserved.
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
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

#include <e_os.h>
#include <openssl/err.h>

/* Internal only functions: only ever used here */
#ifdef OPENSSL_FIPS
extern	void int_ERR_lib_init(void);
# ifndef OPENSSL_NO_ENGINE
extern	void int_EVP_MD_init_engine_callbacks(void );
extern	void int_EVP_CIPHER_init_engine_callbacks(void );
extern	void int_RAND_init_engine_callbacks(void );
# endif
#endif

/* Perform any essential OpenSSL initialization operations.
 * Currently only sets FIPS callbacks
 */

void OPENSSL_init(void)
	{
#ifdef OPENSSL_FIPS
	static int done = 0;
	if (!done)
		{
		int_ERR_lib_init();
#ifdef CRYPTO_MDEBUG
		CRYPTO_malloc_debug_init();
#endif
#ifndef OPENSSL_NO_ENGINE
		int_EVP_MD_init_engine_callbacks();
		int_EVP_CIPHER_init_engine_callbacks();
		int_RAND_init_engine_callbacks();
#endif
		done = 1;
		}
#endif
	}
		
#ifdef OPENSSL_FIPS

int CRYPTO_memcmp(const void *in_a, const void *in_b, size_t len)
	{
	size_t i;
	const unsigned char *a = in_a;
	const unsigned char *b = in_b;
	unsigned char x = 0;

	for (i = 0; i < len; i++)
		x |= a[i] ^ b[i];

	return x;
	}
#endif
