# Native UEFI shim, a first-stage UEFI bootloader

Forked from https://github.com/rhboot/shim

by Ruslan Nikolaev

## Distinctive Features:
- Completely standalone, i.e., does not require the gnu-efi library. To access
  UEFI interfaces, it is using official Edk2/TianoCore headers.
- Native UEFI PE/COFF (DLL) compilation using out-of-the-box clang/llvm
  (or, potentially, the mingw-gcc cross-compiler) instead of creating
  internally relocatable ELF objects with PE/COFF wrapper stubs (gnu-efi).
- Edk's FwImage tool (which I ported from Windows to Linux/POSIX)
  converts DLL to EFI.
- Using smarter linking with llvm's PE/COFF lld-link to prune all unused
  functions/objects in the OpenSSL library.
- Added own printf, string, memory, etc implementations to substitute
  functions previously imported from gnu-efi.
- As a result, substantially reduced footprints of EFI files
  (e.g., 425Kb vs. 1Mb for shimx64.efi compiled with -Os).
- Fixed some bugs, removed a lot of copy & paste code from TianoCore
  (official TianoCore headers are now used anyway) and added other
  improvements to the code.

Note: I have done some preliminary testing, but keep in mind that it is
an experimental version. The code is provided "AS IS" without any guarantees,
and I am not responsible/liable for data loss and/or any other damage!

Licensing terms are the same as in the original shim.

## Requirements:
clang/llvm/lld 7.0.0+ (needs the -mno-stack-arg-probe flag)

Official binaries for various Linux distributions are available at
http://releases.llvm.org/download.html

## Description:
shim is a trivial EFI application that, when run, attempts to open and
execute another application. It will initially attempt to do this via the
standard EFI `LoadImage()` and `StartImage()` calls. If these fail (because Secure
Boot is enabled and the binary is not signed with an appropriate key, for
instance) it will then validate the binary against a built-in certificate. If
this succeeds and if the binary or signing key are not forbidden then shim
will relocate and execute the binary.

shim will also install a protocol which permits the second-stage bootloader
to perform similar binary validation. This protocol has a GUID as described
in the shim.h header file and provides a single entry point. On 64-bit systems
this entry point expects to be called with SysV ABI rather than MSABI, so calls
to it should not be wrapped.

On systems with a TPM chip enabled and supported by the system firmware,
shim will extend various PCRs with the digests of the targets it is
loading.  A full list is in the file [README.tpm](README.tpm) .

To use shim, simply place a DER-encoded public certificate in a file such as
pub.cer and build with `make VENDOR_CERT_FILE=pub.cer`.

There are a couple of build options, and a couple of ways to customize the
build, described in [BUILDING](BUILDING).
