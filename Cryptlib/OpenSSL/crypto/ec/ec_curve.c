/* crypto/ec/ec_curve.c */
/*
 * Written by Nils Larsch for the OpenSSL project.
 */
/* ====================================================================
 * Copyright (c) 1998-2010 The OpenSSL Project.  All rights reserved.
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
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 *
 * Portions of the attached software ("Contribution") are developed by
 * SUN MICROSYSTEMS, INC., and are contributed to the OpenSSL project.
 *
 * The Contribution is licensed pursuant to the OpenSSL open source
 * license provided above.
 *
 * The elliptic curve binary polynomial software is originally written by
 * Sheueling Chang Shantz and Douglas Stebila of Sun Microsystems Laboratories.
 *
 */

#include <string.h>
#include "ec_lcl.h"
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/opensslconf.h>

#ifdef OPENSSL_FIPS
# include <openssl/fips.h>
#endif

typedef struct {
    int field_type,             /* either NID_X9_62_prime_field or
                                 * NID_X9_62_characteristic_two_field */
     seed_len, param_len;
    unsigned int cofactor;      /* promoted to BN_ULONG */
} EC_CURVE_DATA;

#ifndef OPENSSL_NO_SM2
static const struct {
    EC_CURVE_DATA h;
    unsigned char data[0 + 32 * 6];
} _EC_sm2p256v1 = {
    {
       NID_X9_62_prime_field, 0, 32, 1
    },
    {
        /* no seed */

        /* p */
        0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        /* a */
        0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc,
        /* b */
        0x28, 0xe9, 0xfa, 0x9e, 0x9d, 0x9f, 0x5e, 0x34, 0x4d, 0x5a, 0x9e, 0x4b,
        0xcf, 0x65, 0x09, 0xa7, 0xf3, 0x97, 0x89, 0xf5, 0x15, 0xab, 0x8f, 0x92,
        0xdd, 0xbc, 0xbd, 0x41, 0x4d, 0x94, 0x0e, 0x93,
        /* x */
        0x32, 0xc4, 0xae, 0x2c, 0x1f, 0x19, 0x81, 0x19, 0x5f, 0x99, 0x04, 0x46,
        0x6a, 0x39, 0xc9, 0x94, 0x8f, 0xe3, 0x0b, 0xbf, 0xf2, 0x66, 0x0b, 0xe1,
        0x71, 0x5a, 0x45, 0x89, 0x33, 0x4c, 0x74, 0xc7,
        /* y */
        0xbc, 0x37, 0x36, 0xa2, 0xf4, 0xf6, 0x77, 0x9c, 0x59, 0xbd, 0xce, 0xe3,
        0x6b, 0x69, 0x21, 0x53, 0xd0, 0xa9, 0x87, 0x7c, 0xc6, 0x2a, 0x47, 0x40,
        0x02, 0xdf, 0x32, 0xe5, 0x21, 0x39, 0xf0, 0xa0,
        /* order */
        0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0x72, 0x03, 0xdf, 0x6b, 0x21, 0xc6, 0x05, 0x2b,
        0x53, 0xbb, 0xf4, 0x09, 0x39, 0xd5, 0x41, 0x23,
    }
};
#endif /* OPENSSL_NO_SM2 */

typedef struct _ec_list_element_st {
    int nid;
    const EC_CURVE_DATA *data;
    const EC_METHOD *(*meth) (void);
    const char *comment;
} ec_list_element;

static const ec_list_element curve_list[] = {
    /* prime field curves */
    /* secg curves */
#ifndef OPENSSL_NO_SM2
    {NID_sm2, &_EC_sm2p256v1.h, 0,
     "SM2 curve over a 256 bit prime field"},
#endif
};

#define curve_list_length (sizeof(curve_list)/sizeof(ec_list_element))

static EC_GROUP *ec_group_new_from_data(const ec_list_element curve)
{
    EC_GROUP *group = NULL;
    EC_POINT *P = NULL;
    BN_CTX *ctx = NULL;
    BIGNUM *p = NULL, *a = NULL, *b = NULL, *x = NULL, *y = NULL, *order =
        NULL;
    int ok = 0;
    int seed_len, param_len;
    const EC_METHOD *meth;
    const EC_CURVE_DATA *data;
    const unsigned char *params;

    if ((ctx = BN_CTX_new()) == NULL) {
        ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    data = curve.data;
    seed_len = data->seed_len;
    param_len = data->param_len;
    params = (const unsigned char *)(data + 1); /* skip header */
    params += seed_len;         /* skip seed */

    if (!(p = BN_bin2bn(params + 0 * param_len, param_len, NULL))
        || !(a = BN_bin2bn(params + 1 * param_len, param_len, NULL))
        || !(b = BN_bin2bn(params + 2 * param_len, param_len, NULL))) {
        ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_BN_LIB);
        goto err;
    }

    if (curve.meth != 0) {
        meth = curve.meth();
        if (((group = EC_GROUP_new(meth)) == NULL) ||
            (!(group->meth->group_set_curve(group, p, a, b, ctx)))) {
            ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_EC_LIB);
            goto err;
        }
    } else if (data->field_type == NID_X9_62_prime_field) {
        if ((group = EC_GROUP_new_curve_GFp(p, a, b, ctx)) == NULL) {
            ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_EC_LIB);
            goto err;
        }
    }
#ifndef OPENSSL_NO_EC2M
    else {                      /* field_type ==
                                 * NID_X9_62_characteristic_two_field */

        if ((group = EC_GROUP_new_curve_GF2m(p, a, b, ctx)) == NULL) {
            ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_EC_LIB);
            goto err;
        }
    }
#endif

    if ((P = EC_POINT_new(group)) == NULL) {
        ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_EC_LIB);
        goto err;
    }

    if (!(x = BN_bin2bn(params + 3 * param_len, param_len, NULL))
        || !(y = BN_bin2bn(params + 4 * param_len, param_len, NULL))) {
        ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_BN_LIB);
        goto err;
    }
    if (!EC_POINT_set_affine_coordinates_GFp(group, P, x, y, ctx)) {
        ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_EC_LIB);
        goto err;
    }
    if (!(order = BN_bin2bn(params + 5 * param_len, param_len, NULL))
        || !BN_set_word(x, (BN_ULONG)data->cofactor)) {
        ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_BN_LIB);
        goto err;
    }
    if (!EC_GROUP_set_generator(group, P, order, x)) {
        ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_EC_LIB);
        goto err;
    }
    if (seed_len) {
        if (!EC_GROUP_set_seed(group, params - seed_len, seed_len)) {
            ECerr(EC_F_EC_GROUP_NEW_FROM_DATA, ERR_R_EC_LIB);
            goto err;
        }
    }
    ok = 1;
 err:
    if (!ok) {
        EC_GROUP_free(group);
        group = NULL;
    }
    if (P)
        EC_POINT_free(P);
    if (ctx)
        BN_CTX_free(ctx);
    if (p)
        BN_free(p);
    if (a)
        BN_free(a);
    if (b)
        BN_free(b);
    if (order)
        BN_free(order);
    if (x)
        BN_free(x);
    if (y)
        BN_free(y);
    return group;
}

EC_GROUP *EC_GROUP_new_by_curve_name(int nid)
{
    size_t i;
    EC_GROUP *ret = NULL;

#ifdef OPENSSL_FIPS
    if (FIPS_mode())
        return FIPS_ec_group_new_by_curve_name(nid);
#endif
    if (nid <= 0)
        return NULL;

    for (i = 0; i < curve_list_length; i++)
        if (curve_list[i].nid == nid) {
            ret = ec_group_new_from_data(curve_list[i]);
            break;
        }

    if (ret == NULL) {
        ECerr(EC_F_EC_GROUP_NEW_BY_CURVE_NAME, EC_R_UNKNOWN_GROUP);
        return NULL;
    }

    EC_GROUP_set_curve_name(ret, nid);

    return ret;
}

size_t EC_get_builtin_curves(EC_builtin_curve *r, size_t nitems)
{
    size_t i, min;

    if (r == NULL || nitems == 0)
        return curve_list_length;

    min = nitems < curve_list_length ? nitems : curve_list_length;

    for (i = 0; i < min; i++) {
        r[i].nid = curve_list[i].nid;
        r[i].comment = curve_list[i].comment;
    }

    return curve_list_length;
}

/* Functions to translate between common NIST curve names and NIDs */

typedef struct {
    const char *name;           /* NIST Name of curve */
    int nid;                    /* Curve NID */
} EC_NIST_NAME;

static EC_NIST_NAME nist_curves[] = {
    {"B-163", NID_sect163r2},
    {"B-233", NID_sect233r1},
    {"B-283", NID_sect283r1},
    {"B-409", NID_sect409r1},
    {"B-571", NID_sect571r1},
    {"K-163", NID_sect163k1},
    {"K-233", NID_sect233k1},
    {"K-283", NID_sect283k1},
    {"K-409", NID_sect409k1},
    {"K-571", NID_sect571k1},
    {"P-192", NID_X9_62_prime192v1},
    {"P-224", NID_secp224r1},
    {"P-256", NID_X9_62_prime256v1},
    {"P-384", NID_secp384r1},
    {"P-521", NID_secp521r1}
};

const char *EC_curve_nid2nist(int nid)
{
    size_t i;
    for (i = 0; i < sizeof(nist_curves) / sizeof(EC_NIST_NAME); i++) {
        if (nist_curves[i].nid == nid)
            return nist_curves[i].name;
    }
    return NULL;
}

int EC_curve_nist2nid(const char *name)
{
    size_t i;
    for (i = 0; i < sizeof(nist_curves) / sizeof(EC_NIST_NAME); i++) {
        if (!strcmp(nist_curves[i].name, name))
            return nist_curves[i].nid;
    }
    return NID_undef;
}
