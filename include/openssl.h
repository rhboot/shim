/*
 * openssl.h
 * Copyright 2018 Peter Jones <pjones@redhat.com>
 */

#ifndef SHIM_OPENSSL_H_
#define SHIM_OPENSSL_H_

extern void drain_openssl_errors(void);
extern BOOLEAN verify_x509(UINT8 *Cert, UINTN CertSize);
extern BOOLEAN verify_eku(UINT8 *Cert, UINTN CertSize);
extern void show_ca_warning(void);
extern void init_openssl(void);

#endif /* !SHIM_OPENSSL_H_ */
// vim:fenc=utf-8:tw=75:et
