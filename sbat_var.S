// SPDX-License-Identifier: BSD-2-Clause-Patent

#include "include/sbat_var_defs.h"
#include "generated_sbat_var_defs.h"

	.section .sbatlevel, "a", %progbits
	.balignl 4, 0
	.4byte  0 /* format version for external parsers */
	.globl  sbat_var_payload_header
	.type   sbat_var_payload_header, %object
	.size   sbat_var_payload_header, .Lsbat_var_payload_header_end - sbat_var_payload_header
sbat_var_payload_header:
	.4byte  .Lsbat_var_automatic - sbat_var_payload_header
	.4byte  .Lsbat_var_latest - sbat_var_payload_header
.Lsbat_var_payload_header_end:
	.balign	1, 0
.Lsbat_var_automatic:
	.ascii SBAT_VAR_AUTOMATIC
	.byte	0
	.balign	1, 0
.Lsbat_var_latest:
	.ascii SBAT_VAR_LATEST
	.byte 0
	.section .note.GNU-stack,"a"
