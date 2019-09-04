/* fips/ecdsa/fips_ecdsa_selftest.c */
/* Written by Dr Stephen N Henson (steve@openssl.org) for the OpenSSL
 * project 2011.
 */
/* ====================================================================
 * Copyright (c) 2011 The OpenSSL Project.  All rights reserved.
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
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
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
 */

#define OPENSSL_FIPSAPI

#include <string.h>
#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/fips.h>
#include "internal/fips_int.h"
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/bn.h>

#if defined(OPENSSL_FIPS) && !defined(OPENSSL_NO_EC)

static const char P_256_name[] = "ECDSA P-256";

static const unsigned char P_256_d[] = {
    0x51, 0xbd, 0x06, 0xa1, 0x1c, 0xda, 0xe2, 0x12, 0x99, 0xc9, 0x52, 0x3f,
    0xea, 0xa4, 0xd2, 0xd1, 0xf4, 0x7f, 0xd4, 0x3e, 0xbd, 0xf8, 0xfc, 0x87,
    0xdc, 0x82, 0x53, 0x21, 0xee, 0xa0, 0xdc, 0x64
};

static const unsigned char P_256_qx[] = {
    0x23, 0x89, 0xe0, 0xf4, 0x69, 0xe0, 0x49, 0xe5, 0xc7, 0xe5, 0x40, 0x6e,
    0x8f, 0x25, 0xdd, 0xad, 0x11, 0x16, 0x14, 0x9b, 0xab, 0x44, 0x06, 0x31,
    0xbf, 0x5e, 0xa6, 0x44, 0xac, 0x86, 0x00, 0x07
};

static const unsigned char P_256_qy[] = {
    0xb3, 0x05, 0x0d, 0xd0, 0xdc, 0xf7, 0x40, 0xe6, 0xf9, 0xd8, 0x6d, 0x7b,
    0x63, 0xca, 0x97, 0xe6, 0x12, 0xf9, 0xd4, 0x18, 0x59, 0xbe, 0xb2, 0x5e,
    0x4a, 0x6a, 0x77, 0x23, 0xf4, 0x11, 0x9d, 0xeb
};

typedef struct {
    int curve;
    const char *name;
    const unsigned char *x;
    size_t xlen;
    const unsigned char *y;
    size_t ylen;
    const unsigned char *d;
    size_t dlen;
} EC_SELFTEST_DATA;

# define make_ecdsa_test(nid, pr) { nid, pr##_name, \
                                pr##_qx, sizeof(pr##_qx), \
                                pr##_qy, sizeof(pr##_qy), \
                                pr##_d, sizeof(pr##_d)}

static EC_SELFTEST_DATA test_ec_data[] = {
    make_ecdsa_test(NID_X9_62_prime256v1, P_256),
};

int FIPS_selftest_ecdsa()
{
    EC_KEY *ec = NULL;
    BIGNUM *x = NULL, *y = NULL, *d = NULL;
    EVP_PKEY *pk = NULL;
    int rv = 0;
    size_t i;

    for (i = 0; i < sizeof(test_ec_data) / sizeof(EC_SELFTEST_DATA); i++) {
        EC_SELFTEST_DATA *ecd = test_ec_data + i;

        x = BN_bin2bn(ecd->x, ecd->xlen, x);
        y = BN_bin2bn(ecd->y, ecd->ylen, y);
        d = BN_bin2bn(ecd->d, ecd->dlen, d);

        if (!x || !y || !d)
            goto err;

        ec = EC_KEY_new_by_curve_name(ecd->curve);
        if (!ec)
            goto err;

        if (!EC_KEY_set_public_key_affine_coordinates(ec, x, y))
            goto err;

        if (!EC_KEY_set_private_key(ec, d))
            goto err;

        if ((pk = EVP_PKEY_new()) == NULL)
            goto err;

        EVP_PKEY_assign_EC_KEY(pk, ec);

        if (!fips_pkey_signature_test(pk, NULL, 0,
                                      NULL, 0, EVP_sha256(), 0, ecd->name))
            goto err;
    }

    rv = 1;

 err:

    if (x)
        BN_clear_free(x);
    if (y)
        BN_clear_free(y);
    if (d)
        BN_clear_free(d);
    if (pk)
        EVP_PKEY_free(pk);
    else if (ec)
        EC_KEY_free(ec);

    return rv;

}

#endif
