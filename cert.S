// SPDX-License-Identifier: BSD-2-Clause-Patent

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

	.globl cert_table
	.type	cert_table, %object
	.size	cert_table, .Lcert_table_end - cert_table
	.section .vendor_cert, "a", %progbits
	.balignl 4, 0
cert_table:
	.4byte	.Lvendor_authorized_end - vendor_authorized
	.4byte	.Lvendor_deauthorized_end - vendor_deauthorized
	.4byte	vendor_authorized - cert_table
	.4byte	vendor_deauthorized - cert_table
	.balign	1, 0
	.type	vendor_authorized, %object
	.size	vendor_authorized, .Lvendor_authorized_end - vendor_authorized
	.section .vendor_cert, "a", %progbits
vendor_authorized:
#if defined(VENDOR_DB_FILE)
.incbin VENDOR_DB_FILE
#elif defined(VENDOR_CERT_FILE)
.incbin VENDOR_CERT_FILE
#endif
.Lvendor_authorized_end:
	.balign	1, 0
	.type	vendor_deauthorized, %object
	.size	vendor_deauthorized, .Lvendor_deauthorized_end - vendor_deauthorized
	.section .vendor_cert, "a", %progbits
vendor_deauthorized:
#if defined(VENDOR_DBX_FILE)
.incbin VENDOR_DBX_FILE
#endif
.Lvendor_deauthorized_end:
.Lcert_table_end:
	.section .note.GNU-stack,"a"
