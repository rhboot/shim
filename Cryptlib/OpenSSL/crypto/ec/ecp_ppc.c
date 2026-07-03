/*
 * Copyright 2009-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include "internal/cryptlib.h"
#include "crypto/ppc_arch.h"
#include "ec_local.h"

void ecp_nistz256_mul_mont(uint64_t res[4], const uint64_t a[4],
                           const uint64_t b[4]);

void ecp_nistz256_to_mont(uint64_t res[4], const uint64_t in[4]);
void ecp_nistz256_to_mont(uint64_t res[4], const uint64_t in[4])
{
    static const uint64_t RR[] = { 0x0000000000000003U,
                                        0xfffffffbffffffffU,
                                        0xfffffffffffffffeU,
                                        0x00000004fffffffdU };

    ecp_nistz256_mul_mont(res, in, RR);
}

void ecp_nistz256_from_mont(uint64_t res[4], const uint64_t in[4]);
void ecp_nistz256_from_mont(uint64_t res[4], const uint64_t in[4])
{
    static const uint64_t one[] = { 1, 0, 0, 0 };

    ecp_nistz256_mul_mont(res, in, one);
}
