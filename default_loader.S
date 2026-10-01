// SPDX-License-Identifier: BSD-2-Clause-Patent

	.section .loader, "a", %progbits
	.balign	1, 0
	.globl	default_loader
	.type	default_loader, %object
	.size	default_loader, .Ldefault_loader_end - default_loader
default_loader:
	.ascii DEFAULT_LOADER_CHAR
	.byte	0
.Ldefault_loader_end:
	.section .note.GNU-stack,"a"
