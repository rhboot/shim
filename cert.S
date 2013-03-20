#if defined(VENDOR_CERT_FILE)
	.globl	vendor_cert_size
	.data
	.align	1
	.type	vendor_cert_size, @object
	.size	vendor_cert_size, 4
	.section .vendor_cert, "", @progbits
vendor_cert_size:
	.long	.L0 - vendor_cert
	.globl	vendor_cert
	.data
	.align	1
	.type	vendor_cert, @object
	.size	vendor_cert, .L0-vendor_cert
	.section .vendor_cert, "", @progbits
vendor_cert:
.incbin VENDOR_CERT_FILE
.L0:
#else
	.globl	vendor_cert
	.bss
	.type	vendor_cert, @object
	.size	vendor_cert, 1
	.section .vendor_cert, "", @progbits
vendor_cert:
	.zero	1

	.globl	vendor_cert_size
	.data
	.align 4
	.type	vendor_cert_size, @object
	.size	vendor_cert_size, 4
	.section .vendor_cert, "", @progbits
vendor_cert_size:
	.long	1
#endif
