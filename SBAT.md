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
Microsoft may refer to this as a form of Secure Boot Advanced Targeting (SBAT), perhaps to be named EFI_CERT_SBAT.

#### Version-Based Revocation Overview
Metadata that includes the vendor, product family, product/module, & version are added to modules. This metadata is protected by the digital signature. New image authorization data structures, akin to the EFI_CERT_foo EFI_SIGNATURE_DATA structure, describe how this metadata can be incorporated into allow or deny lists. In a simple implementation, 1 SBAT entry with security version could be used for each revocable boot module, replacing many image hashes with 1 entry with security version. To minimize the size of EFI_CERT_SBAT, the signature owner field might be omitted, and recommend that either metadata use shortened names, or perhaps the EFI_CERT_SBAT contains a hash of the non-version metadata instead of the metadata itself.
Ideally, servicing of the image authorization databases would be updated to support replace of individual EFI_SIGNATURE_DATA items. However, if we assume that new UEFI variable(s) are used, to be serviced by 1 entity per variable (no sharing), then the existing, in-market SetVariable(), without the APPEND attribute, could be used. Microsoft currently issues dbx updates exclusively with the APPEND attribute under the assumption that multiple entities might be servicing dbx. When a new revocation event takes place, rather than increasing the size of variables with image hashes, existing variables can simply be updated with new security versions, consuming no additional space. This constrains the number of entries to the number of unique boot components revoked, independent of versions revoked. The solution may support several major/minor versions, limiting revocation to build/security versions, perhaps via wildcards.

#### Version-Based Revocation Metadata
Existing specifications and tools exist for adding this type of authenticated metadata to PE images. For example, Windows applications and drivers accomplish this by adding a resource section (.RSRC) containing a VS_VERSIONINFO structure. Microsoft has produced [an extension to the edk2-pytools-extensions build system](https://github.com/tianocore/edk2-pytool-extensions/pull/214/files) to easily add this metadata to EFI PE images on both
[Windows](https://dev.azure.com/tianocore/edk2-pytool-extensions/_build/results?buildId=12047&view=logs&j=1372d9e0-5cd3-5ef5-5e82-7ce7a218d320&t=807a83c4-d85c-5f7b-2bd5-ab855378aa38)
and
[Linux](https://dev.azure.com/tianocore/edk2-pytool-extensions/_build/results?buildId=12046&view=logs&j=ec3a3918-b516-55e3-c6c9-a51bd985c058&t=0fc8d1a9-cd73-59c5-9625-ee1ecb5064e8) build platforms.

_Open item:_ define VS_VERSIONINFO usage for compatibility with standardized authorization and revocation mechanisms

#### Version-Based Revocation Authorization
A compatible system may need to trust multiple, in-support major/minor versions of products, only revoking ranges of build &/or security version numbers. The authorization system must support this, details TBD.

An EDK2 example demonstrating a method of image authorization, based upon version metadata, should be available in the coming week. We expect the structures and methods to evolve as feedback is incorporated.
