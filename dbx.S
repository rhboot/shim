#if defined(VENDOR_DBX_FILE)
	.globl	vendor_dbx_size
	.data
	.align	1
	.type	vendor_dbx_size, @object
	.size	vendor_dbx_size, 4
	.section .vendor_cert, "", @progbits
vendor_dbx_size:
	.long	.L0 - vendor_dbx
	.globl	vendor_dbx
	.data
	.align	1
	.type	vendor_dbx, @object
	.size	vendor_dbx, .L0-vendor_dbx
	.section .vendor_cert, "", @progbits
vendor_dbx:
.incbin VENDOR_DBX_FILE
.L0:
#else
	.globl	vendor_dbx
	.bss
	.type	vendor_dbx, @object
	.size	vendor_dbx, 1
	.section .vendor_cert, "", @progbits
vendor_dbx:
	.zero	1

	.globl	vendor_dbx_size
	.data
	.align 4
	.type	vendor_dbx_size, @object
	.size	vendor_dbx_size, 4
	.section .vendor_cert, "", @progbits
vendor_dbx_size:
	.long	0
#endif
