	.globl cert_table
	.type	cert_table, %object
	.size	cert_table, 4
	.section .vendor_cert, "a", %progbits
cert_table:
#if defined(VENDOR_CERT_FILE)
	.long	vendor_cert_priv_end - vendor_cert_priv
#else
	.long	0
#endif
#if defined(VENDOR_ESL_FILE)
	.long	vendor_esl_priv_end - vendor_esl_priv
#else
	.long	0
#endif
#if defined(VENDOR_DBX_FILE)
	.long	vendor_dbx_priv_end - vendor_dbx_priv
#else
	.long	0
#endif
	.long	vendor_cert_priv - cert_table
	.long	vendor_esl_priv - cert_table
	.long	vendor_dbx_priv - cert_table
#if defined(VENDOR_CERT_FILE)
	.data
	.align	1
	.type	vendor_cert_priv, %object
	.size	vendor_cert_priv, vendor_cert_priv_end-vendor_cert_priv
	.section .vendor_cert, "a", %progbits
vendor_cert_priv:
.incbin VENDOR_CERT_FILE
vendor_cert_priv_end:
#else
	.bss
	.type	vendor_cert_priv, %object
	.size	vendor_cert_priv, 1
	.section .vendor_cert, "a", %progbits
vendor_cert_priv:
	.zero	1

	.data
	.align 4
	.type	vendor_cert_size_priv, %object
	.size	vendor_cert_size_priv, 4
	.section .vendor_cert, "a", %progbits
vendor_cert_priv_end:
#endif
#if defined(VENDOR_ESL_FILE)
	.data
	.align	1
	.type	vendor_esl_priv, %object
	.size	vendor_esl_priv, vendor_esl_priv_end-vendor_esl_priv
	.section .vendor_cert, "a", %progbits
vendor_esl_priv:
.incbin VENDOR_ESL_FILE
vendor_esl_priv_end:
#else
	.bss
	.type	vendor_esl_priv, %object
	.size	vendor_esl_priv, 1
	.section .vendor_cert, "a", %progbits
vendor_esl_priv:
	.zero	1

	.data
	.align 4
	.type	vendor_esl_size_priv, %object
	.size	vendor_esl_size_priv, 4
	.section .vendor_cert, "a", %progbits
vendor_esl_priv_end:
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
	.type	vendor_dbx_size_priv, %object
	.size	vendor_dbx_size_priv, 4
	.section .vendor_cert, "a", %progbits
vendor_dbx_priv_end:
#endif
