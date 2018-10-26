#ifdef __i386__
# define SYMBOL(S)		\
	.global	_##S;		\
	.p2align	2;	\
	_##S:
#else
# define SYMBOL(S)		\
	.global	S;		\
	.p2align	2;	\
	S:
#endif

	.section	.rdata,"dr"

SYMBOL(cert_table)
#if defined(VENDOR_CERT_FILE)
	.long	vendor_cert_priv_end - vendor_cert_priv
#else
	.long	0
#endif
#if defined(VENDOR_DBX_FILE)
	.long	vendor_dbx_priv_end - vendor_dbx_priv
#else
	.long	0
#endif
#if defined(VENDOR_CERT_FILE)
	.long	vendor_cert_priv - cert_table
#else
	.long	0
#endif
#if defined(VENDOR_DBX_FILE)
	.long	vendor_dbx_priv - cert_table
#else
	.long	0
#endif

#if defined(VENDOR_CERT_FILE)
	.p2align	2
vendor_cert_priv:
.incbin VENDOR_CERT_FILE
vendor_cert_priv_end:
#endif

#if defined(VENDOR_DBX_FILE)
	.p2align	2
vendor_dbx_priv:
.incbin VENDOR_DBX_FILE
vendor_dbx_priv_end:
#endif
