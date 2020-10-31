#include "config.h"

#if defined(VENDOR_DB_FILE) && defined(VENDOR_CERT_FILE)
# error both VENDOR_DB_FILE and VENDOR_CERT_FILE have been configured
#elif defined(VENDOR_DB_FILE)
# define vendor_authorized vendor_db
# define vendor_authorized_end vendor_db_end
# define vendor_authorized_size vendor_db_size
# define vendor_authorized_size_end vendor_db_size_end
#elif defined(VENDOR_CERT_FILE)
# define vendor_authorized vendor_cert
# define vendor_authorized_end vendor_cert_end
# define vendor_authorized_size vendor_cert_size
# define vendor_authorized_size_end vendor_cert_size_end
#endif

#if defined(VENDOR_DBX_FILE)
# define vendor_deauthorized vendor_dbx
# define vendor_deauthorized_end vendor_dbx_end
# define vendor_deauthorized_size vendor_dbx_size
# define vendor_deauthorized_size_end vendor_dbx_size_end
#endif

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
	.long	.Lvendor_authorized_end - vendor_authorized
	.long	.Lvendor_deauthorized_end - vendor_deauthorized
	.long	vendor_authorized - cert_table
	.long	vendor_deauthorized - cert_table
	.balign	1, 0
vendor_authorized:
#if defined(VENDOR_DB_FILE)
.incbin VENDOR_DB_FILE
#elif defined(VENDOR_CERT_FILE)
.incbin VENDOR_CERT_FILE
#endif
.Lvendor_authorized_end:
	.balign	1, 0
vendor_deauthorized:
#if defined(VENDOR_DBX_FILE)
.incbin VENDOR_DBX_FILE
#endif
.Lvendor_deauthorized_end:
