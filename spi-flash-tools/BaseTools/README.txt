

For convenience, these BaseTools binaries were pre-built for Linux
x86_64 from this source code:

  BaseTools -r13937 http://svn.code.sf.net/p/edk2/code/branches/UDK2010.SR1/BaseTools

If these binaries are not compatible with your system then you must:

A. Either download and build these tools yourself; follow these instructions:
   http://sourceforge.net/apps/mediawiki/tianocore/index.php?title=Edk2-buildtools

B. Or you can re-use the ones embedded in EDKII if you have built EDKII from source.


After either A. or B. you should override the BASETOOLS Make variable
on the command line, for instance like this:

  BASETOOLS=../Quark_EDKII/BaseTools/Source/C/bin   ../spi-flash-tools/Makefile
