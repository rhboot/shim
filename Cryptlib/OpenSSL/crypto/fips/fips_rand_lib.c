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

/* If we don't define _XOPEN_SOURCE_EXTENDED, struct timeval won't
   be defined and gettimeofday() won't be declared with strict compilers
   like DEC C in ANSI C mode.  */
#ifndef _XOPEN_SOURCE_EXTENDED
# define _XOPEN_SOURCE_EXTENDED 1
#endif

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/fips.h>
#include "internal/fips_int.h"
#include <openssl/fips_rand.h>
#include "e_os.h"

#if !(defined(OPENSSL_SYS_WIN32) || defined(OPENSSL_SYS_VXWORKS))
# include <sys/time.h>
#endif
#if defined(OPENSSL_SYS_VXWORKS)
# include <time.h>
#endif
#ifndef OPENSSL_SYS_WIN32
# ifdef OPENSSL_UNISTD
#  include OPENSSL_UNISTD
# else
#  include <unistd.h>
# endif
#endif

/* FIPS API for PRNG use. Similar to RAND functionality but without
 * ENGINE and additional checking for non-FIPS rand methods.
 */

static const RAND_METHOD *fips_rand_meth = NULL;
static int fips_approved_rand_meth = 0;
static int fips_rand_bits = 0;

/* Allows application to override number of bits and uses non-FIPS methods */
void FIPS_rand_set_bits(int nbits)
{
    fips_rand_bits = nbits;
}

int FIPS_rand_set_method(const RAND_METHOD *meth)
{
    if (!fips_rand_bits) {
        if (meth == FIPS_drbg_method())
            fips_approved_rand_meth = 1;
        else {
            fips_approved_rand_meth = 0;
            if (FIPS_module_mode()) {
                FIPSerr(FIPS_F_FIPS_RAND_SET_METHOD, FIPS_R_NON_FIPS_METHOD);
                return 0;
            }
        }
    }
    fips_rand_meth = meth;
    return 1;
}

const RAND_METHOD *FIPS_rand_get_method(void)
{
    return fips_rand_meth;
}

void FIPS_rand_reset(void)
{
    if (fips_rand_meth && fips_rand_meth->cleanup)
        fips_rand_meth->cleanup();
}

int FIPS_rand_seed(const void *buf, int num)
{
    if (!fips_approved_rand_meth && FIPS_module_mode()) {
        FIPSerr(FIPS_F_FIPS_RAND_SEED, FIPS_R_NON_FIPS_METHOD);
        return 0;
    }
    if (fips_rand_meth && fips_rand_meth->seed)
        fips_rand_meth->seed(buf, num);
    return 1;
}

int FIPS_rand_bytes(unsigned char *buf, int num)
{
    if (!fips_approved_rand_meth && FIPS_module_mode()) {
        FIPSerr(FIPS_F_FIPS_RAND_BYTES, FIPS_R_NON_FIPS_METHOD);
        return 0;
    }
    if (fips_rand_meth && fips_rand_meth->bytes)
        return fips_rand_meth->bytes(buf, num);
    return 0;
}

int FIPS_rand_status(void)
{
    if (!fips_approved_rand_meth && FIPS_module_mode()) {
        FIPSerr(FIPS_F_FIPS_RAND_STATUS, FIPS_R_NON_FIPS_METHOD);
        return 0;
    }
    if (fips_rand_meth && fips_rand_meth->status)
        return fips_rand_meth->status();
    return 0;
}

/* Return instantiated strength of PRNG. For DRBG this is an internal
 * parameter. Any other type of PRNG is not approved and returns 0 in
 * FIPS mode and maximum 256 outside FIPS mode.
 */

int FIPS_rand_strength(void)
{
    if (fips_rand_bits)
        return fips_rand_bits;
    if (fips_approved_rand_meth == 1)
        return FIPS_drbg_get_strength(FIPS_get_default_drbg());
    else if (fips_approved_rand_meth == 0) {
        if (FIPS_module_mode())
            return 0;
        else
            return 256;
    }
    return 0;
}

void FIPS_get_timevec(unsigned char *buf, unsigned long *pctr)
{
# ifdef OPENSSL_SYS_WIN32
    FILETIME ft;
# elif defined(OPENSSL_SYS_VXWORKS)
    struct timespec ts;
# else
    struct timeval tv;
# endif

#if defined(OPENSSL_SYS_UNIX) || defined(__DJGPP__)
    unsigned long pid;
# endif

# ifdef OPENSSL_SYS_WIN32
    GetSystemTimeAsFileTime(&ft);
    buf[0] = (unsigned char)(ft.dwHighDateTime & 0xff);
    buf[1] = (unsigned char)((ft.dwHighDateTime >> 8) & 0xff);
    buf[2] = (unsigned char)((ft.dwHighDateTime >> 16) & 0xff);
    buf[3] = (unsigned char)((ft.dwHighDateTime >> 24) & 0xff);
    buf[4] = (unsigned char)(ft.dwLowDateTime & 0xff);
    buf[5] = (unsigned char)((ft.dwLowDateTime >> 8) & 0xff);
    buf[6] = (unsigned char)((ft.dwLowDateTime >> 16) & 0xff);
    buf[7] = (unsigned char)((ft.dwLowDateTime >> 24) & 0xff);
# elif defined(OPENSSL_SYS_VXWORKS)
    clock_gettime(CLOCK_REALTIME, &ts);
    buf[0] = (unsigned char)(ts.tv_sec & 0xff);
    buf[1] = (unsigned char)((ts.tv_sec >> 8) & 0xff);
    buf[2] = (unsigned char)((ts.tv_sec >> 16) & 0xff);
    buf[3] = (unsigned char)((ts.tv_sec >> 24) & 0xff);
    buf[4] = (unsigned char)(ts.tv_nsec & 0xff);
    buf[5] = (unsigned char)((ts.tv_nsec >> 8) & 0xff);
    buf[6] = (unsigned char)((ts.tv_nsec >> 16) & 0xff);
    buf[7] = (unsigned char)((ts.tv_nsec >> 24) & 0xff);
# else
    gettimeofday(&tv, NULL);
    buf[0] = (unsigned char)(tv.tv_sec & 0xff);
    buf[1] = (unsigned char)((tv.tv_sec >> 8) & 0xff);
    buf[2] = (unsigned char)((tv.tv_sec >> 16) & 0xff);
    buf[3] = (unsigned char)((tv.tv_sec >> 24) & 0xff);
    buf[4] = (unsigned char)(tv.tv_usec & 0xff);
    buf[5] = (unsigned char)((tv.tv_usec >> 8) & 0xff);
    buf[6] = (unsigned char)((tv.tv_usec >> 16) & 0xff);
    buf[7] = (unsigned char)((tv.tv_usec >> 24) & 0xff);
# endif
    buf[8] = (unsigned char)(*pctr & 0xff);
    buf[9] = (unsigned char)((*pctr >> 8) & 0xff);
    buf[10] = (unsigned char)((*pctr >> 16) & 0xff);
    buf[11] = (unsigned char)((*pctr >> 24) & 0xff);

    (*pctr)++;

#if defined(OPENSSL_SYS_UNIX) || defined(__DJGPP__)
    pid = (unsigned long)getpid();
    buf[12] = (unsigned char)(pid & 0xff);
    buf[13] = (unsigned char)((pid >> 8) & 0xff);
    buf[14] = (unsigned char)((pid >> 16) & 0xff);
    buf[15] = (unsigned char)((pid >> 24) & 0xff);
# endif
}

