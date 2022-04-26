OUTPUT_FORMAT("elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)
SECTIONS
{
  . = 0;
  ImageBase = .;
  .hash : { *(.hash) }	/* this MUST come first! */
  . = ALIGN(4096);
  .eh_frame :
  {
    *(.eh_frame)
  }
  . = ALIGN(4096);
  .text :
  {
   _text = .;
   *(.text)
   *(.text.*)
   *(.gnu.linkonce.t.*)
   _etext = .;
  }
  . = ALIGN(4096);
  .reloc :
  {
   *(.reloc)
  }
  . = ALIGN(4096);
  .note.gnu.build-id : {
    *(.note.gnu.build-id)
  }

  . = ALIGN(4096);
  .data.ident : {
    *(.data.ident)
  }

  . = ALIGN(4096);
  .data :
  {
   _data = .;
   *(.rodata*)
   *(.got.plt)
   *(.got)
   *(.data*)
   *(.sdata)
   /* the EFI loader doesn't seem to like a .bss section, so we stick
      it all into .data: */
   *(.sbss)
   *(.scommon)
   *(.dynbss)
   *(.bss)
   *(COMMON)
   *(.rel.local)
  }

  . = ALIGN(4096);
  .vendor_cert :
  {
   *(.vendor_cert)
  }
  . = ALIGN(4096);
  .dynamic  : { *(.dynamic) }
  . = ALIGN(4096);
  .rela :
  {
    *(.rela.data*)
    *(.rela.got*)
    *(.rela.stab*)
  }
  _edata = .;
  _data_size = . - _data;
  . = ALIGN(4096);
  .sbat :
  {
    _sbat = .;
    *(.sbat)
    *(.sbat.*)
  }
  _esbat = .;
  _sbat_size = . - _sbat;

  . = ALIGN(4096);
  .dynsym   : { *(.dynsym) }
  . = ALIGN(4096);
  .dynstr   : { *(.dynstr) }
  . = ALIGN(4096);
  .ignored.reloc :
  {
    *(.rela.reloc)
    *(.eh_frame)
    *(.note.GNU-stack)
  }
  .comment 0 : { *(.comment) }
  .note.gnu.build-id : { *(.note.gnu.build-id) }
}
