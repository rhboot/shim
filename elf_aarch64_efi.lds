OUTPUT_FORMAT("elf64-littleaarch64", "elf64-littleaarch64", "elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)
SECTIONS
{
  . = 0;
  ImageBase = .;
  .hash : { *(.hash) }	/* this MUST come first! */
  . = ALIGN(65536);
  .eh_frame :
  {
    *(.eh_frame)
  }
  . = ALIGN(65536);
  .text :
  {
   _text = .;
   *(.text)
   *(.text.*)
   *(.gnu.linkonce.t.*)
   _etext = .;
  }
  . = ALIGN(65536);
  .reloc :
  {
   *(.reloc)
  }
  . = ALIGN(65536);
  .note.gnu.build-id : {
    *(.note.gnu.build-id)
  }

  . = ALIGN(65536);
  .data.ident : {
    *(.data.ident)
  }
  . = ALIGN(65536);
  .sbatlevel : {
    *(.sbatlevel)
  }

  . = ALIGN(65536);
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

  . = ALIGN(65536);
  .vendor_cert :
  {
   *(.vendor_cert)
  }
  . = ALIGN(65536);
  .dynamic  : { *(.dynamic) }
  . = ALIGN(65536);
  .rela :
  {
    *(.rela.data*)
    *(.rela.got*)
    *(.rela.stab*)
  }
  _edata = .;
  _data_size = . - _data;
  . = ALIGN(65536);
  .sbat :
  {
    _sbat = .;
    *(.sbat)
    *(.sbat.*)
  }
  _esbat = .;
  _sbat_size = . - _sbat;

  . = ALIGN(65536);
  .dynsym   : { *(.dynsym) }
  . = ALIGN(65536);
  .dynstr   : { *(.dynstr) }
  . = ALIGN(65536);
  .ignored.reloc :
  {
    *(.rela.reloc)
    *(.eh_frame)
    *(.note.GNU-stack)
  }
  .comment 0 : { *(.comment) }
  .note.gnu.build-id : { *(.note.gnu.build-id) }
}
