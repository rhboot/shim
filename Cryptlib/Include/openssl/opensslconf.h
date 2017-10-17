/*
 * WARNING: do not edit!
 * Generated from include/openssl/opensslconf.h.in
 *
 * Copyright 2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef OPENSSL_ALGORITHM_DEFINES
# error OPENSSL_ALGORITHM_DEFINES no longer supported
#endif

/*
 * OpenSSL was configured with the following options:
 */

#ifndef OPENSSL_SYS_UEFI
# define OPENSSL_SYS_UEFI 1
#endif
#define OPENSSL_MIN_API 0x10100000L
#ifndef OPENSSL_NO_BF
# define OPENSSL_NO_BF
#endif
#ifndef OPENSSL_NO_BLAKE2
# define OPENSSL_NO_BLAKE2
#endif
#ifndef OPENSSL_NO_CAMELLIA
# define OPENSSL_NO_CAMELLIA
#endif
#ifndef OPENSSL_NO_CAST
# define OPENSSL_NO_CAST
#endif
#ifndef OPENSSL_NO_CHACHA
# define OPENSSL_NO_CHACHA
#endif
#ifndef OPENSSL_NO_CMS
# define OPENSSL_NO_CMS
#endif
#ifndef OPENSSL_NO_CT
# define OPENSSL_NO_CT
#endif
#ifndef OPENSSL_NO_DES
# define OPENSSL_NO_DES
#endif
#ifndef OPENSSL_NO_DSA
# define OPENSSL_NO_DSA
#endif
#ifndef OPENSSL_NO_EC
# define OPENSSL_NO_EC
#endif
#ifndef OPENSSL_NO_IDEA
# define OPENSSL_NO_IDEA
#endif
#ifndef OPENSSL_NO_MD2
# define OPENSSL_NO_MD2
#endif
#ifndef OPENSSL_NO_MD4
# define OPENSSL_NO_MD4
#endif
#ifndef OPENSSL_NO_MDC2
# define OPENSSL_NO_MDC2
#endif
#ifndef OPENSSL_NO_POLY1305
# define OPENSSL_NO_POLY1305
#endif
#ifndef OPENSSL_NO_RC2
# define OPENSSL_NO_RC2
#endif
#ifndef OPENSSL_NO_RC5
# define OPENSSL_NO_RC5
#endif
#ifndef OPENSSL_NO_RMD160
# define OPENSSL_NO_RMD160
#endif
#ifndef OPENSSL_NO_SEED
# define OPENSSL_NO_SEED
#endif
#ifndef OPENSSL_NO_SRP
# define OPENSSL_NO_SRP
#endif
#ifndef OPENSSL_NO_TS
# define OPENSSL_NO_TS
#endif
#ifndef OPENSSL_NO_UI
# define OPENSSL_NO_UI
#endif
#ifndef OPENSSL_NO_WHIRLPOOL
# define OPENSSL_NO_WHIRLPOOL
#endif
#ifndef OPENSSL_NO_AFALGENG
# define OPENSSL_NO_AFALGENG
#endif
#ifndef OPENSSL_NO_APPS
# define OPENSSL_NO_APPS
#endif
#ifndef OPENSSL_NO_ASAN
# define OPENSSL_NO_ASAN
#endif
#ifndef OPENSSL_NO_ASM
# define OPENSSL_NO_ASM
#endif
#ifndef OPENSSL_NO_ASYNC
# define OPENSSL_NO_ASYNC
#endif
#ifndef OPENSSL_NO_AUTOALGINIT
# define OPENSSL_NO_AUTOALGINIT
#endif
#ifndef OPENSSL_NO_AUTOERRINIT
# define OPENSSL_NO_AUTOERRINIT
#endif
#ifndef OPENSSL_NO_CAPIENG
# define OPENSSL_NO_CAPIENG
#endif
#ifndef OPENSSL_NO_CRYPTO_MDEBUG
# define OPENSSL_NO_CRYPTO_MDEBUG
#endif
#ifndef OPENSSL_NO_CRYPTO_MDEBUG_BACKTRACE
# define OPENSSL_NO_CRYPTO_MDEBUG_BACKTRACE
#endif
#ifndef OPENSSL_NO_DEPRECATED
# define OPENSSL_NO_DEPRECATED
#endif
#ifndef OPENSSL_NO_DGRAM
# define OPENSSL_NO_DGRAM
#endif
#ifndef OPENSSL_NO_DTLS
# define OPENSSL_NO_DTLS
#endif
#ifndef OPENSSL_NO_DTLS1
# define OPENSSL_NO_DTLS1
#endif
#ifndef OPENSSL_NO_DTLS1_2
# define OPENSSL_NO_DTLS1_2
#endif
#ifndef OPENSSL_NO_EC2M
# define OPENSSL_NO_EC2M
#endif
#ifndef OPENSSL_NO_EC_NISTP_64_GCC_128
# define OPENSSL_NO_EC_NISTP_64_GCC_128
#endif
#ifndef OPENSSL_NO_ECDH
# define OPENSSL_NO_ECDH
#endif
#ifndef OPENSSL_NO_ECDSA
# define OPENSSL_NO_ECDSA
#endif
#ifndef OPENSSL_NO_EGD
# define OPENSSL_NO_EGD
#endif
#ifndef OPENSSL_NO_ENGINE
# define OPENSSL_NO_ENGINE
#endif
#ifndef OPENSSL_NO_ERR
# define OPENSSL_NO_ERR
#endif
#ifndef OPENSSL_NO_FILENAMES
# define OPENSSL_NO_FILENAMES
#endif
#ifndef OPENSSL_NO_FUZZ_AFL
# define OPENSSL_NO_FUZZ_AFL
#endif
#ifndef OPENSSL_NO_FUZZ_LIBFUZZER
# define OPENSSL_NO_FUZZ_LIBFUZZER
#endif
#ifndef OPENSSL_NO_GOST
# define OPENSSL_NO_GOST
#endif
#ifndef OPENSSL_NO_HEARTBEATS
# define OPENSSL_NO_HEARTBEATS
#endif
#ifndef OPENSSL_NO_HW
# define OPENSSL_NO_HW
#endif
#ifndef OPENSSL_NO_MSAN
# define OPENSSL_NO_MSAN
#endif
#ifndef OPENSSL_NO_OCB
# define OPENSSL_NO_OCB
#endif
#ifndef OPENSSL_NO_POSIX_IO
# define OPENSSL_NO_POSIX_IO
#endif
#ifndef OPENSSL_NO_RFC3779
# define OPENSSL_NO_RFC3779
#endif
#ifndef OPENSSL_NO_SCRYPT
# define OPENSSL_NO_SCRYPT
#endif
#ifndef OPENSSL_NO_SCTP
# define OPENSSL_NO_SCTP
#endif
#ifndef OPENSSL_NO_SOCK
# define OPENSSL_NO_SOCK
#endif
#ifndef OPENSSL_NO_SSL_TRACE
# define OPENSSL_NO_SSL_TRACE
#endif
#ifndef OPENSSL_NO_SSL3
# define OPENSSL_NO_SSL3
#endif
#ifndef OPENSSL_NO_SSL3_METHOD
# define OPENSSL_NO_SSL3_METHOD
#endif
#ifndef OPENSSL_NO_STDIO
# define OPENSSL_NO_STDIO
#endif
#ifndef OPENSSL_NO_TESTS
# define OPENSSL_NO_TESTS
#endif
#ifndef OPENSSL_NO_UBSAN
# define OPENSSL_NO_UBSAN
#endif
#ifndef OPENSSL_NO_UNIT_TEST
# define OPENSSL_NO_UNIT_TEST
#endif
#ifndef OPENSSL_NO_WEAK_SSL_CIPHERS
# define OPENSSL_NO_WEAK_SSL_CIPHERS
#endif
#ifndef OPENSSL_NO_AFALGENG
# define OPENSSL_NO_AFALGENG
#endif


/*
 * Sometimes OPENSSSL_NO_xxx ends up with an empty file and some compilers
 * don't like that.  This will hopefully silence them.
 */
#define NON_EMPTY_TRANSLATION_UNIT static void *dummy = &dummy;

/*
 * Applications should use -DOPENSSL_API_COMPAT=<version> to suppress the
 * declarations of functions deprecated in or before <version>. Otherwise, they
 * still won't see them if the library has been built to disable deprecated
 * functions.
 */
#if defined(OPENSSL_NO_DEPRECATED)
# define DECLARE_DEPRECATED(f)
#elif __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0)
# define DECLARE_DEPRECATED(f)    f __attribute__ ((deprecated));
#else
# define DECLARE_DEPRECATED(f)   f;
#endif

#ifndef OPENSSL_FILE
# ifdef OPENSSL_NO_FILENAMES
#  define OPENSSL_FILE ""
#  define OPENSSL_LINE 0
# else
#  define OPENSSL_FILE __FILE__
#  define OPENSSL_LINE __LINE__
# endif
#endif

#ifndef OPENSSL_MIN_API
# define OPENSSL_MIN_API 0
#endif

#if !defined(OPENSSL_API_COMPAT) || OPENSSL_API_COMPAT < OPENSSL_MIN_API
# undef OPENSSL_API_COMPAT
# define OPENSSL_API_COMPAT OPENSSL_MIN_API
#endif

#if OPENSSL_API_COMPAT < 0x10100000L
# define DEPRECATEDIN_1_1_0(f)   DECLARE_DEPRECATED(f)
#else
# define DEPRECATEDIN_1_1_0(f)
#endif

#if OPENSSL_API_COMPAT < 0x10000000L
# define DEPRECATEDIN_1_0_0(f)   DECLARE_DEPRECATED(f)
#else
# define DEPRECATEDIN_1_0_0(f)
#endif

#if OPENSSL_API_COMPAT < 0x00908000L
# define DEPRECATEDIN_0_9_8(f)   DECLARE_DEPRECATED(f)
#else
# define DEPRECATEDIN_0_9_8(f)
#endif



/* Generate 80386 code? */
#undef I386_ONLY

#undef OPENSSL_UNISTD
#define OPENSSL_UNISTD <unistd.h>

#undef OPENSSL_EXPORT_VAR_AS_FUNCTION

/*
 * The following are cipher-specific, but are part of the public API.
 */
#if !defined(OPENSSL_SYS_UEFI)
# undef BN_LLONG
/* Only one for the following should be defined */
# undef SIXTY_FOUR_BIT_LONG
# undef SIXTY_FOUR_BIT
# define THIRTY_TWO_BIT
#endif

#define RC4_INT unsigned int

#ifdef  __cplusplus
}
#endif
