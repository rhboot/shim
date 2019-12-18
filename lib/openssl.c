/*
 * openssl.c
 * Copyright 2018 Peter Jones <pjones@redhat.com>
 *
 * OpenSSL helper / setup code
 */
#include "shim.h"

#include <stdarg.h>

#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/ocsp.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/rsa.h>
#include <internal/dso.h>

#include <Library/BaseCryptLib.h>

#include <stdint.h>

#define OID_EKU_MODSIGN "1.3.6.1.4.1.2312.16.1.2"

void
drain_openssl_errors(void)
{
        unsigned long err = -1;
        while (err != 0)
                err = ERR_get_error();
}

BOOLEAN verify_x509(UINT8 *Cert, UINTN CertSize)
{
        UINTN length;

        if (!Cert || CertSize < 4)
                return FALSE;

        /*
         * A DER encoding x509 certificate starts with SEQUENCE(0x30),
         * the number of length bytes, and the number of value bytes.
         * The size of a x509 certificate is usually between 127 bytes
         * and 64KB. For convenience, assume the number of value bytes
         * is 2, i.e. the second byte is 0x82.
         */
        if (Cert[0] != 0x30 || Cert[1] != 0x82)
                return FALSE;

        length = Cert[2]<<8 | Cert[3];
        if (length != (CertSize - 4))
                return FALSE;

        return TRUE;
}

BOOLEAN verify_eku(UINT8 *Cert, UINTN CertSize)
{
        X509 *x509;
        CONST UINT8 *Temp = Cert;
        EXTENDED_KEY_USAGE *eku;
        ASN1_OBJECT *module_signing;

        module_signing = OBJ_nid2obj(OBJ_create(OID_EKU_MODSIGN,
                                                "modsign-eku",
                                                "modsign-eku"));

        x509 = d2i_X509 (NULL, &Temp, (long) CertSize);
        if (x509 != NULL) {
                eku = X509_get_ext_d2i(x509, NID_ext_key_usage, NULL, NULL);

                if (eku) {
                        int i = 0;
                        for (i = 0; i < sk_ASN1_OBJECT_num(eku); i++) {
                                ASN1_OBJECT *key_usage = sk_ASN1_OBJECT_value(eku, i);

                                if (OBJ_cmp(module_signing, key_usage) == 0)
                                        return FALSE;
                        }
                        EXTENDED_KEY_USAGE_free(eku);
                }

                X509_free(x509);
        }

        return TRUE;
}

void show_ca_warning(void)
{
        CHAR16 *text[9];

        text[0] = L"WARNING!";
        text[1] = L"";
        text[2] = L"The CA certificate used to verify this image doesn't   ";
        text[3] = L"contain the CA flag in Basic Constraints or KeyCertSign";
        text[4] = L"in KeyUsage. Such CA certificates will not be supported";
        text[5] = L"in the future.                                         ";
        text[6] = L"";
        text[7] = L"Please contact the issuer to update the certificate.   ";
        text[8] = NULL;

        console_reset();
        console_print_box(text, -1);
}

void CONSTRUCTOR
init_openssl(void)
{
        OPENSSL_init();
        ERR_load_ERR_strings();
        ERR_load_BN_strings();
        ERR_load_RSA_strings();
        ERR_load_DH_strings();
        ERR_load_EVP_strings();
        ERR_load_BUF_strings();
        ERR_load_OBJ_strings();
        ERR_load_PEM_strings();
        ERR_load_X509_strings();
        ERR_load_ASN1_strings();
        ERR_load_CONF_strings();
        ERR_load_CRYPTO_strings();
        ERR_load_COMP_strings();
        ERR_load_BIO_strings();
        ERR_load_PKCS7_strings();
        ERR_load_X509V3_strings();
        ERR_load_PKCS12_strings();
        ERR_load_RAND_strings();
        ERR_load_DSO_strings();
        ERR_load_OCSP_strings();
}

// vim:fenc=utf-8:tw=75:et
