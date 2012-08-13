#if defined(VENDOR_CERT_FILE)
	.globl	vendor_cert
	.data
	.align	16
	.type	vendor_cert, @object
	.size	vendor_cert_size, vendor_cert_size-vendor_cert
vendor_cert:
.incbin VENDOR_CERT_FILE

	.globl	vendor_cert_size
	.data
	.align	16
	.type	vendor_cert_size, @object
	.size	vendor_cert_size, 4
vendor_cert_size:
	.long	vendor_cert_size - vendor_cert
#else
	.globl	vendor_cert
	.bss
	.type	vendor_cert, @object
	.size	vendor_cert, 1
vendor_cert:
	.zero	1

	.globl	vendor_cert_size
	.data
	.align 4
	.type	vendor_cert_size, @object
	.size	vendor_cert_size, 4
vendor_cert_size:
	.long	1
#endif
