

# UEFI shim bootloader secure boot lifecycle improvements
## Background
In the PC ecosystem, [UEFI Secure Boot](https://docs.microsoft.com/en-us/windows-hardware/design/device-experiences/oem-secure-boot) is typically configured to trust 2 authorities for signing UEFI boot code, the Microsoft UEFI Certificate Authority (CA) and Windows CA. When malicious or security compromised code is detected, 2 revocation mechanisms are provided by compatible UEFI implementations, signing certificate or image hash. The UEFI Specification provides additional revocation mechanisms, but their availability and compatibility have not undergone testing.
Signing certificate revocation is not practical for the Windows and Microsoft UEFI CAs because it would revoke too many UEFI applications and drivers, especially for Option ROMs. This is true even for the UEFI CA leaf certificates as they generally sign 1 entire year of UEFI images. For this reason UEFI revocations have, until recently, been performed via image hash.
The UEFI shim bootloader provides a level of digital signature indirection, enabling more authorities to participate in UEFI Secure Boot. Shims' certificates typically sign targeted UEFI applications, enabling certificate-based revocation where it makes sense.
As part of the recent "boothole" security incident [CVE-2020-10713](https://nvd.nist.gov/vuln/detail/CVE-2020-10713), 3 certificates and 150 image hashes were added to the UEFI Secure Boot revocation database `dbx` on the popular x64 architecture. This single revocation event consumes 10kB of the 32kB, or roughly one third, of revocation storage typically available on UEFI platforms. Due to the way that UEFI merges revocation lists, this plus prior revocation events can result in a `dbx` that is almost 15kB in size, approaching 50% capacity.
The large size of the boothole revocation event is due to the inefficiency of revocation by image hash when there is a security vulnerability in a popular module signed by many authorities, sometimes with many versions.
Coordinating the boothole revocation has required numerous person months of planning, implementation, and testing multiplied by the number of authorities, deployments, & devices. It is not yet complete, and we anticipate many months of upgrades and testing with a long tail that may last years.
Additionally, when bugs or features require updates to UEFI shim, the number of images signed are multiplied by the number of authorities.

## Summary
Given the tremendous cost and disruption of a revocation event like boothole, and increased activity by security researchers in the secure boot space, we should take action to greatly improve this process. Updating revocation capabilities in the UEFI spec and system BIOS implementations will take years to deploy into the ecosystem. As such, the focus of this document is on improvements that can be made to the UEFI shim, which are compatible with existing UEFI implementations. Shim can move faster than the UEFI system BIOS ecosystem while providing large impact to the in-market Secure Boot ecosystem.

The background section identified 2 opportunities for improvement:

1. Improving the efficiency of revocation when a number of version have a vulnerability
  
   * For example, a vulnerability spans foo versions, it might be more efficient to be able to revoke by version, and simply modify the revocation entry to modify the version each time a vulnerablity is detected.
2. Improving the efficiency of revocation when there are many shim variations
  
   * For example, a new shim is released to address bugs or adding features. In the current model, the number of images signed are multiplied by the number of authorities as they sign shims to gain the fixes and features.

Microsoft has brainstormed with partners possible solutions for evaluation and feedback:

1. To improve revocation when there are many versions of vulnerable boot images, shim, grub, or otherwise, investigate methods of revoking by image metadata that includes version numbers. Once targeting data is established (e.g. Company foo, product bar, boot component zed), each revocation event ideally edits an existing entry, increasing the trusted minimum security version.
2. To improve revocation when there is a shim vulnerability, and there are many shim images, standardize on a single image shared by authorities. Each release of bug fixes and features result in 1 shim being signed, compressing the number by dozens. This has the stellar additional benefit of reducing the number of shim reviews, which should result in much rejoicing. A vendor's certificates can be revoked by placing the image hash into dbx, similar to the many shim solution we have today.

## Proposals
This document focuses on the shim bootloader, not the UEFI specification or updates to UEFI system BIOS.

### Version Number Based Revocation
Microsoft may refer to this as a form of Secure Boot Advanced Targeting (SBAT), perhaps to be named EFI_CERT_SBAT. This introduces a mechanism to require a
specific level of resistance to secure boot bypasses.

#### Version-Based Revocation Overview
Metadata that includes the vendor, product family, product/module, & version are added to modules. This metadata is protected by the digital signature. New image authorization data structures, akin to the EFI_CERT_foo EFI_SIGNATURE_DATA structure, describe how this metadata can be incorporated into allow or deny lists. In a simple implementation, 1 SBAT entry with security version could be used for each revocable boot module, replacing many image hashes with 1 entry with security version. To minimize the size of EFI_CERT_SBAT, the signature owner field might be omitted, and recommend that either metadata use shortened names, or perhaps the EFI_CERT_SBAT contains a hash of the non-version metadata instead of the metadata itself.
Ideally, servicing of the image authorization databases would be updated to support replace of individual EFI_SIGNATURE_DATA items. However, if we assume that new UEFI variable(s) are used, to be serviced by 1 entity per variable (no sharing), then the existing, in-market SetVariable(), without the APPEND attribute, could be used. Microsoft currently issues dbx updates exclusively with the APPEND attribute under the assumption that multiple entities might be servicing dbx. When a new revocation event takes place, rather than increasing the size of variables with image hashes, existing variables can simply be updated with new security versions, consuming no additional space. This constrains the number of entries to the number of unique boot components revoked, independent of versions revoked. The solution may support several major/minor versions, limiting revocation to build/security versions, perhaps via wildcards.

While previously the APPEND attribute guaranteed that it is not possible to downgrade the set of revocations on a system using a previously signed variable update, this guarantee can also be accomplished by setting the EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute. This will verify that the
timestamp value of the signed data is later than the current timestamp value associated with the data currently stored in that variable.

#### Version-Based Revocation Scenarios

 Products (not vendors, a vendor can have multiple products or even
pass a product from one vendor to another over time) are assigned a
name. Product names can specify a specifc version or refer to the
entire prodcut family. For example mydistro and mydistro-12.

 Components that are used as a link in the UEFI secure boot chain of
trust are also assigned names. Examples of components are shim, GRUB,
kernel, hypervisors, etc.

 We could conceivably support sub-components, but it's hard to
conceive of a scenario that would trigger a UEFI variable update that
wouldn't justify a hypervisor or kernel re-release to enforce that sub
component level from there. Something like a "level 1.5 hypervisor"
that can exist between different kernel generations can be considered
its own component.

 Each component is assigned a minimum global generation number. Vendors
signing component binary artifacts with a specific global generation
number are required to include fixes for any public or pre-disclosed
issue required for that generation. Additionally, in the event that a
bypass only manifests in a specific products component, vendors may
ask for a product specific generation number to be published for one
of their products components. This avoids triggering an industry wide
re-publishing of otherwise safe components.

 A product specific minimum generation number only applies to the
instance of that component that is signed with that product
name. Another products instance of the same component may be installed
on the same system and would not be subject to the other products
product specific minimum generation number. However both of those
components will need to meet the global minimum generation number for
that component. A very likely scenario would be that a product is
shipped with an incomplete fix required for a specific minimum
generation number, but is labeled with that number. Rather than having
the entire industry that uses that component re-release, just that
products minimum generation number is incremented and that products
component is re-released along with a UEFI variable update that
specifies that requirement.

 The global and product specific generation number name spaces are not
tied to each other. The global number is managed externally, and the
vast majority of products will never publish a minimum product
specific generation number for any of their components. These
components will be signed with a product specific generation number of
0.

 A minimum feature set, for example enforced kernel lock down, may be
required as well to sign and label a component with a specific
generation number. As time goes on it is likely that the minimum
feature set required for the currently valid generation number will
expand. (For example, hyper visors supporting secure boot guests may
at some point require memory encryption or similar protection
mechanism.)

#### Retiring Signed Releases

Products that have reached the end of their support life by definition
no longer receive patches. They are also generally not examined	for
CVEs. Allowing such unsupported	products to continue to	participate in
UEFI Secure Boot is at the very	least questionable. If an EoSL product
is made up of commonly	used components,	such as	the GRUB and the Linux
kernel,	it is reasonable	to assume that the global generation numbers
will eventually move forward and	exclude	those products from booting on
a UEFI Secure Boot enabled system. However a product made up of	GRUB
and a closed source kernel is just a conceivable. In that case the
kernel version may never move forward once the product reaches its end
of support. Therefor it	is recommended that the	product	specific
generation number be incremented past the latest one shown in any
binary for that	product, effectively disabling that product on UEFI
Secure Boot enabled systems.

A subset of this case are beta-release that may contain eventually
abandoned, experimental, kernel code. Such releases should have their
product specific generation numbers incremented past the latest one
shown in any, released or unreleased, production key signed binary.

Until a release is retired in this manner, vendors	are responsible	for
keeping up with fixes for CVEs and ensuring that any known signed
binaries containing known CVEs are denied from booting on UEFI Secure
Boot enabled systems via the most up to date UEFI	meta data.

#### Key Revocations
Since Vendor Product keys are brought into Shim	as signed binaries,
generation numbering can and should be used to revoke them in case of
a private key compromise.

#### Kernel support for SBAT

The initial SBAT implementation will add SBAT metadata to Shim and
GRUB and enforce SBAT on all components labeled with it. Until a
component like say the Linux kernel gains SBAT metadata it can not be
revoked	via SBAT, but only by revoking the keys signing those
kernels. These keys will should live in separate, product specific
signed PE files that contain only the certificate and SBAT metadata for the key
files. These key files can then be revoked via SBAT in order to invalidate
and replace a specic key. While certificates built into Shim can be revoked via
SBAT and Shim introspection, this practice would still result in a proliferation of
Shim binaries that would need to be revoked via dbx in the event of an
early Shim code bug. Therefor, SBAT must be used in conjunction with
separate Vendor Product Key binaries.

#### Kernels execing other kernels (aka kexec, fast reboot)

It is expected that kexec and other similar implementations of kernels
spawning other kernels will eventually consume and honor SBAT
metadata. Until they do, the same Vendor Product Key binary based
revocation needs to be used for them.



#### Version-Based Revocation Metadata
Existing specifications and tools exist for adding this type of authenticated metadata to PE images. For example, Windows applications and drivers accomplish this by adding a resource section (.RSRC) containing a VS_VERSIONINFO structure. Microsoft has produced [an extension to the edk2-pytools-extensions build system](https://github.com/tianocore/edk2-pytool-extensions/pull/214/files) to easily add this metadata to EFI PE images on both [Windows](https://dev.azure.com/tianocore/edk2-pytool-extensions/_build/results?buildId=12047&view=logs&j=1372d9e0-5cd3-5ef5-5e82-7ce7a218d320&t=807a83c4-d85c-5f7b-2bd5-ab855378aa38)
and [Linux](https://dev.azure.com/tianocore/edk2-pytool-extensions/_build/results?buildId=12046&view=logs&j=ec3a3918-b516-55e3-c6c9-a51bd985c058&t=0fc8d1a9-cd73-59c5-9625-ee1ecb5064e8) build platforms.

 Each component carries a meta data payload within the signed binary
that contains the publishing product name, the component name and both
a global and product specific generation number. This data is embedded
in a VS_VERSIONINFO structure as StringFileInfo components making up a
VERSION_RECORD_ENTRIES structure:

```
BEGIN
      // encoding
      BLOCK "040904b0"
      BEGIN
        // unused by sbat, required for VERSIONINFO
        VALUE "CompanyName",        "ACME Corporation"
        VALUE "FileDescription",    "GRUB2 bootloader"
        VALUE "FileVersion",        "11.6-preview.rc7552"
        VALUE "OriginalFilename",    "kernel.img"

        // used by sbat
        VALUE "InternalName",          "GRUB2"
        VALUE "ComponentGeneration",    "4"
        VALUE "ProductName",        "Friendly Distro"
        VALUE "ProductGeneration",    "16"
        VALUE "ProductVersion",     "11.6"
        VALUE "VersionGeneration",    "0"
     END
END
```

#### UEFI SBAT Variable content
The SBAT UEFI variable then contains a descriptive form of all
components used by all UEFI signed Operating Systems, along with a
minimum generation number for each one.	It may also contain a product
specific generation number, which in turn also specify version
specific generation numbers. It	is expected that specific generation
numbers will be	exceptions that	will be	obsoleted if and when the
global number for a component is incremented.

Example:
```
// XXX evolving, this does is not meant to imply any specific form of notation or markup
COMPONENTS
  InternalName                   "SHIM"
    MinComponentGeneration       0
  InternalName                   "GRUB2"
    MinComponentGeneration       3
  InternalName                   "Linux Kernel"
    MinComponentGeneration       73
    PRODUCTS
      ProductName                "Some Linux"
        MinProductGeneration     1
      ProductName                "Other Linux"
        MinProductGeneration     0
        VERSIONS
          ProductVersion         "32"
            MinVersionGeneration 2
          ProductVersion         "33"
            MinVersionGeneration 1
        /VERSIONS
    /PRODUCTS
  InternalName                   "Some Vendor Cert"
    MinComponentGeneration       22
  InternalName                   "Other Vendor Cert"
    MinComponentGeneration       4
/COMPONENTS

```

An EDK2 example demonstrating a method of image authorization, based upon version metadata, should be available soon. We expect the structures and methods to evolve as feedback is incorporated.
