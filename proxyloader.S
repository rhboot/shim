	.globl proxy_loader_table
	.type	proxy_loader_table, %object
	.size	proxy_loader_table, 4
	.section .proxy_loader, "a", %progbits
proxy_loader_table:
#if defined(PROXY_LOADER_FILE)
	.long	proxy_loader_priv_end - proxy_loader_priv
#else
	.long	0
#endif
	.long	proxy_loader_priv - proxy_loader_table
#if defined(PROXY_LOADER_FILE)
	.data
	.align	1
	.type	proxy_loader_priv, %object
	.size	proxy_loader_priv, proxy_loader_priv_end-proxy_loader_priv
	.section .proxy_loader, "a", %progbits
proxy_loader_priv:
.incbin PROXY_LOADER_FILE
proxy_loader_priv_end:
#else
	.bss
	.type	proxy_loader_priv, %object
	.size	proxy_loader_priv, 1
	.section .proxy_loader, "a", %progbits
proxy_loader_priv:
	.zero	1

	.data
	.align 4
	.type	proxy_loader_size_priv, %object
	.size	proxy_loader_size_priv, 4
	.section .proxy_loader, "a", %progbits
proxy_loader_priv_end:
#endif
