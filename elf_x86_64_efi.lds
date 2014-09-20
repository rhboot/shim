/* Same as elf_x86_64_fbsd_efi.lds, except for OUTPUT_FORMAT below - KEEP IN SYNC */
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{
  .text 0x0 : {
    *(.text.head)
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
    *(.srodata)
    *(.rodata*)
    . = ALIGN(16);
    _etext = .;
  }
  .dynamic : { *(.dynamic) }
  .data :
  {
    *(.sdata)
    *(.data)
    *(.data1)
    *(.data.*)
    *(.got.plt)
    *(.got)

    /* the EFI loader doesn't seem to like a .bss section, so we stick
     * it all into .data: */
    . = ALIGN(16);
    _bss = .;
    *(.sbss)
    *(.scommon)
    *(.dynbss)
    *(.bss)
    *(COMMON)
    . = ALIGN(16);
    _bss_end = .;
  }
  . = ALIGN(4096);
  .vendor_cert :
  {
    *(.vendor_cert)
  }

  . = ALIGN(4096);
  .rela.dyn : { *(.rela.dyn) }
  .rela.plt : { *(.rela.plt) }
  .rela.got : { *(.rela.got) }
  .rela.data : { *(.rela.data) *(.rela.data*) }
  _edata = .;
  _data_size = . - _etext;

  . = ALIGN(4096);
  .dynsym : { *(.dynsym) }
  . = ALIGN(4096);
  .dynstr : { *(.dynstr) }
  . = ALIGN(4096);
  /DISCARD/ :
  {
    *(.rel.reloc)
    *(.eh_frame)
    *(.note.GNU-stack)
  }
  .comment 0 : { *(.comment) }
}
