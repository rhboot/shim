#include "edk2asm.h"

       .file   "riscv64_cache.S"
       .text

GCC_ASM_EXPORT(__riscv_flush_icache)

ASM_PFX(__riscv_flush_icache):
       fence.i
       ret
