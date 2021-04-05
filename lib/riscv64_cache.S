.text

# __builtin___clear_cache() for riscv64 is implemented via vDSO in
# gcc, because when running under a full OS the possibility of the
# process being moved to a different hart by the scheduler has to
# be accounted for. In the case of shim, we're running on a single
# hart and there is no OS scheduler involved, so we can simply run
# the asm instructions directly ourselves. __riscv_flush_icache()
# is the symbol that gcc expects to be present to provide that
# functionality
.globl		__riscv_flush_icache
.type		__riscv_flush_icache, %function
__riscv_flush_icache:
    fence.i
    ret
