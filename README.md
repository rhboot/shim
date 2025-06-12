# shim, a first-stage UEFI bootloader

shim is a trivial EFI application that, when run, attempts to open and
execute another application. It will initially attempt to do this via the
standard EFI `LoadImage()` and `StartImage()` calls. If these fail (because Secure
Boot is enabled and the binary is not signed with an appropriate key, for
instance) it will then validate the binary against a built-in certificate. If
this succeeds and if the binary or signing key are not forbidden then shim
will relocate and execute the binary.

## protocols

### shim lock protocol

shim will also install a protocol which permits the second-stage bootloader
to perform similar binary validation. This protocol has a GUID as described
in the shim.h header file and provides a single entry point. On 64-bit systems
this entry point expects to be called with SysV ABI rather than MSABI, so calls
to it should not be wrapped.

### shim loader protocol

Since version 16.1 shim overrides the system table and installs its own version
of the LoadImage()/StartImage()/UnloadImage()/Exit() functions, so that second
stages can simply call them from the system table, and it will work whether shim
is first stage or not, without requiring shim-specific code in the second stages.

When this protocol is installed, signed UKIs
[Unified Kernel Images](https://uapi-group.org/specifications/specs/unified_kernel_image/)
can be loaded even if the nested kernel is not signed, as after the UKI is loaded
and validated, shim builds an internal allowlist of all the sections that are
contained in the UKI. When an image is loaded from one such section, it is
validated against denylists (DBX/MOKX/SBAT at the time of writing), but it is
not checked against allowlists (DB/MOK hashes/signatures), as the outer image
was already validated and the inner image is thus covered by those signatures or
hashes. Furthermore, the inner image is not measured in the TPM, to avoid double
measurements.

## TPM

On systems with a TPM chip enabled and supported by the system firmware,
shim will extend various PCRs with the digests of the targets it is
loading.  A full list is in the file [README.tpm](README.tpm) .

## builds and tests

To use shim, simply place a DER-encoded public certificate in a file such as
pub.cer and build with `make VENDOR_CERT_FILE=pub.cer`.

There are a couple of build options, and a couple of ways to customize the
build, described in [BUILDING](BUILDING).

See the [test plan](testplan.txt), and file a ticket if anything fails!

## contacts

In the event that the developers need to be contacted related to a security
incident or vulnerability, please mail [secalert@redhat.com].

[secalert@redhat.com]: mailto:secalert@redhat.com
