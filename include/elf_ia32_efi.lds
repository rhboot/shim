OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
  . = 0;
  ImageBase = .;

  /*
   * The first entry here MUST be above the end of the PE OptHeader and the
   * section table, or else ld will clobber the OptHeader.
   */
  .text 0x1000 : ALIGN(1024)
  {
    _text = .;
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
    _etext = .;
  }

  . = ALIGN(4096);
  .data :
  {
    _data = .;
    *(.rodata*)
    *(.data)
    *(.data1)
    *(.data.*)
    *(.sdata)
    *(.got.plt)
    *(.got)

    . = ALIGN(16);
    _init_array = .;
    *(SORT_BY_NAME(.init_array))
    _init_array_end = .;
    __CTOR_LIST__ = .;
    *(SORT_BY_NAME(.ctors))
    __CTOR_END__ = .;
    __DTOR_LIST__ = .;
    *(SORT_BY_NAME(.dtors))
    __DTOR_END__ = .;
    _fini_array = .;
    *(SORT_BY_NAME(.fini_array))
    _fini_array_end = .;

    /* the EFI loader doesn't seem to like a .bss section, so we stick
       it all into .data: */
    *(.sbss)
    *(.scommon)
    *(.dynbss)
    *(.bss)
    *(COMMON)
  }

  . = ALIGN(4096);
  .vendor_cert : ALIGN(4096) { *(.vendor_cert) }
  _edata = .;
  _data_size = . - _data;

  . = ALIGN(4096);
  .reloc : ALIGN(512) { *(.reloc) }
  .eh_frame : ALIGN(512) { *(.eh_frame) }
  .rel : ALIGN(512)
  {
    *(.rel.data)
    *(.rel.data.*)
    *(.rel.got)
    *(.rel.stab)
    *(.rel.init_array)
    *(.data.rel.ro.local)
    *(.data.rel.local)
    *(.data.rel.ro)
    *(.data.rel*)
  }

  . = ALIGN(4096);
  .note.gnu.build-id : ALIGN(512) { *(.note.gnu.build-id)  }
  .data.ident : ALIGN(512) { *(.data.ident) }
  . = ALIGN(4096);
  .dynamic : ALIGN(512) { *(.dynamic) }
  .dynsym : ALIGN(512) { *(.dynsym) }

  /*
   * All our debuginfo stuff
   */
  .debug : ALIGN(512) { *(.debug) }
  .debug_info : ALIGN(512) { *(.debug_info) }
  .debug_abbrev : ALIGN(512) { *(.debug_abbrev) }
  .debug_aranges : ALIGN(512) { *(.debug_aranges) }
  .debug_line : ALIGN(512) { *(.debug_line) }
  .debug_str : ALIGN(512) { *(.debug_str) }
  .debug_ranges : ALIGN(512) { *(.debug_ranges) }

  /*
   * Everything after here is stuff we're stripping from the .efi binary
   */

  .hash : ALIGN(512) { *(.hash) }
  .comment : ALIGN(512) { *(.comment) }
  .dynstr : ALIGN(512) { *(.dynstr) }

  /DISCARD/ :
  {
    *(.rela.reloc)
    *(.note.GNU-stack)
  }

}
