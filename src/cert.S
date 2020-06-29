#include "config.h"

#if defined(VENDOR_DB_FILE) && defined(VENDOR_CERT_FILE)
# error both VENDOR_DB_FILE and VENDOR_CERT_FILE have been configured
#endif

	.globl cert_table
	.type	cert_table, %object
	.size	cert_table, 16
	.section .vendor_cert, "a", %progbits
cert_table:
#if defined(VENDOR_DB_FILE)
	.long	vendor_db_priv_end - vendor_db_priv
#elif defined(VENDOR_CERT_FILE)
	.long	vendor_cert_priv_end - vendor_cert_priv
#else
	.long	0
#endif
#if defined(VENDOR_DBX_FILE)
	.long	vendor_dbx_priv_end - vendor_dbx_priv
#else
	.long	0
#endif
#if defined(VENDOR_DB_FILE)
	.long	vendor_db_priv - cert_table
#elif defined(VENDOR_CERT_FILE)
	.long	vendor_cert_priv - cert_table
#else
	.long	0
#endif
	.long	vendor_dbx_priv - cert_table
#if defined(VENDOR_DB_FILE)
	.data
	.align	1
	.type	vendor_db_priv, %object
	.size	vendor_db_priv, vendor_db_priv_end-vendor_db_priv
	.section .vendor_cert, "a", %progbits
vendor_db_priv:
.incbin VENDOR_DB_FILE

	.data
	.align 4
	.type	vendor_db_priv_end, %object
	.size	vendor_db_priv_end, 4
	.section .vendor_cert, "a", %progbits
vendor_db_priv_end:
#elif defined(VENDOR_CERT_FILE)
	.data
	.align	1
	.type	vendor_cert_priv, %object
	.size	vendor_cert_priv, vendor_cert_priv_end-vendor_cert_priv
	.section .vendor_cert, "a", %progbits
vendor_cert_priv:
.incbin VENDOR_CERT_FILE

	.data
	.align 4
	.type	vendor_cert_priv_end, %object
	.size	vendor_cert_priv_end, 4
	.section .vendor_cert, "a", %progbits
vendor_cert_priv_end:
#else
	.bss
	.type	vendor_priv, %object
	.size	vendor_priv, 1
	.section .vendor_cert, "a", %progbits
vendor_priv:
	.zero	1

	.data
	.align 4
	.type	vendor_priv_end, %object
	.size	vendor_priv_end, 4
	.section .vendor_cert, "a", %progbits
vendor_priv_end:
#endif
#if defined(VENDOR_DBX_FILE)
	.data
	.align	1
	.type	vendor_dbx_priv, %object
	.size	vendor_dbx_priv, vendor_dbx_priv_end-vendor_dbx_priv
	.section .vendor_cert, "a", %progbits
vendor_dbx_priv:
.incbin VENDOR_DBX_FILE
vendor_dbx_priv_end:
#else
	.bss
	.type	vendor_dbx_priv, %object
	.size	vendor_dbx_priv, 1
	.section .vendor_cert, "a", %progbits
vendor_dbx_priv:
	.zero	1

	.data
	.align 4
	.type	vendor_dbx_priv_end, %object
	.size	vendor_dbx_priv_end, 4
	.section .vendor_cert, "a", %progbits
vendor_dbx_priv_end:
#endif
