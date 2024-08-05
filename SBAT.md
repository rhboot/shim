# UEFI shim bootloader secure boot life-cycle improvements

## Background

In the PC ecosystem, [UEFI Secure Boot](https://docs.microsoft.com/en-us/windows-hardware/design/device-experiences/oem-secure-boot)
is typically configured to trust 2 authorities for signing UEFI boot code, the
Microsoft UEFI Certificate Authority (CA) and Windows CA. When malicious or
security compromised code is detected, 2 revocation mechanisms are provided by
compatible UEFI implementations: signing certificate or image hash. The UEFI
Specification does not provides any well tested additional revocation
mechanisms.

Signing certificate revocation is not practical for the Windows and Microsoft
UEFI CAs because it would revoke too many UEFI applications and drivers,
especially for Option ROMs. This is true even for the UEFI CA leaf certificates
as they generally sign 1 entire year of UEFI images. For this reason UEFI
revocations have, until recently, been performed via image hash.

The UEFI shim bootloader provides a level of digital signature indirection,
enabling more authorities to participate in UEFI Secure Boot. Shims'
certificates typically sign targeted UEFI applications, enabling
certificate-based revocation where it makes sense.  As part of the recent
"BootHole" security incident
[CVE-2020-10713](https://nvd.nist.gov/vuln/detail/CVE-2020-10713), 3
certificates and 150 image hashes were added to the UEFI Secure Boot revocation
database `dbx` on the popular x64 architecture. This single revocation event
consumes 10kB of the 32kB, or roughly one third, of revocation storage
typically available on UEFI platforms. Due to the way that UEFI merges
revocation lists, this plus prior revocation events can result in a `dbx` that
is almost 15kB in size, approaching 50% capacity.

The large size of the BootHole revocation event is due to the inefficiency of
revocation by image hash when there is a security vulnerability in a popular
component signed by many authorities, sometimes with many versions.

Coordinating the BootHole revocation has required numerous person months of
planning, implementation, and testing multiplied by the number of authorities,
deployments, & devices. It is not yet complete, and we anticipate many months
of upgrades and testing with a long tail that may last years.

Additionally, when bugs or features require updates to UEFI shim, the number of
images signed are multiplied by the number of authorities.

## Summary

Given the tremendous cost and disruption of a revocation event like BootHole,
and increased activity by security researchers in the UEFI Secure Boot space,
we should take action to greatly improve this process. Updating revocation
capabilities in the UEFI specification and system firmware implementations will
take years to deploy into the ecosystem. As such, the focus of this document is
on improvements that can be made to the UEFI shim, which are compatible with
existing UEFI implementations. Shim can move faster than the UEFI system
firmware ecosystem while providing large impact to the in-market UEFI Secure
Boot ecosystem.

The background section identified 2 opportunities for improvement:

1. Improving the efficiency of revocation when a number of versions have a
   vulnerability

   * For example, a vulnerability spans some number of versions, it might be
     more efficient to be able to revoke by version, and simply modify the
     revocation entry to modify the version each time a vulnerability is
     detected.

2. Improving the efficiency of revocation when there are many shim variations

   * For example, a new shim is released to address bugs or adding features. In
     the current model, the number of images signed are multiplied by the
     number of authorities as they sign shims to gain the fixes and features.

Microsoft has brainstormed with partners possible solutions for evaluation and
feedback:

1. To improve revocation when there are many versions of vulnerable boot
   images, shim, GRUB, or otherwise, investigate methods of revoking by image
   metadata that includes generation numbers. Once targeting data is
   established (e.g. Company foo, product bar, boot component zed), each
   revocation event ideally edits an existing entry, increasing the trusted
   minimum security generation.

2. To improve revocation when there is a shim vulnerability, and there are many
   shim images, standardize on a single image shared by authorities. Each
   release of bug fixes and features result in 1 shim being signed, compressing
   the number by dozens. This has the stellar additional benefit of reducing
   the number of shim reviews, which should result in much rejoicing. The
   certificates used by a vendor to sign individual boot components would be
   picked up from additional PE files that are signed either by a shim-specific
   key controlled by Microsoft, or controlled by a vendor, but used only to
   sign additional key files. This key built into shim is functionally similar
   to a CA certificate. The certificates built into shim can be revoked by
   placing the image hash into dbx, similar to the many shim solution we have
   today.

## Proposals

This document focuses on the shim bootloader, not the UEFI specification or
updates to UEFI firmware.

### Generation Number Based Revocation

Microsoft may refer to this as a form of UEFI Secure Boot Advanced Targeting
(SBAT), perhaps to be named EFI_CERT_SBAT. This introduces a mechanism to
require a specific level of resistance to UEFI Secure Boot bypasses.

#### Generation-Based Revocation Overview

Metadata that includes the vendor, product family, product, component, version
and generation are added to artifacts. This metadata is protected by the
digital signature. New image authorization data structures, akin to the
EFI_CERT_foo EFI_SIGNATURE_DATA structure (see Signature Database in UEFI
specification), describe how this metadata can be incorporated into allow or
deny lists. In a simple implementation, 1 SBAT entry with security generations
could be used for each revocable boot module, replacing many image hashes with
1 entry with security generations. To minimize the size of EFI_CERT_SBAT, the
signature owner field might be omitted, and recommend that either metadata use
shortened names, or perhaps the EFI_CERT_SBAT contains a hash of the
non-generation metadata instead of the metadata itself.

Ideally, servicing of the image authorization databases would be updated to
support replacement of individual EFI_SIGNATURE_DATA items. However, if we
assume that new UEFI variable(s) are used, to be serviced by 1 entity per
variable (no sharing), then the existing, in-market SetVariable(), without the
APPEND attribute, could be used. Microsoft currently issues dbx updates
exclusively with the APPEND attribute under the assumption that multiple
entities might be servicing dbx. When a new revocation event takes place,
rather than increasing the size of variables with image hashes, existing
variables can simply be updated with new security generations, consuming no
additional space. This constrains the number of entries to the number of unique
boot components revoked, independent of generations revoked. The solution may
support several major/minor versions, limiting revocation to build/security
generations, perhaps via wildcards.

While previously the APPEND attribute guaranteed that it would not be possible
to downgrade the set of revocations on a system using a previously signed
variable update, this guarantee can also be accomplished by setting the
EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute. This will verify
that the timestamp value of the signed data is later than the current timestamp
value associated with the data currently stored in that variable.

#### Generation-Based Revocation Scenarios

**Products** (**not** vendors, a vendor can have multiple products or even pass a
product from one vendor to another over time) are assigned a name. Product
names can specify a specific version or refer to the entire product family. For
example mydistro and mydistro,12.

**Components** that are used as a link in the UEFI Secure Boot chain of trust are
assigned names. Examples of components are shim, GRUB, kernel, hypervisors, etc.

Below is an example of a product and component, both in the same `sbat.csv` file. `sbat`
defines the version of the format of the revocation metadata itself.
`grub.acme` is an example of a product name.
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub.acme,1,Acme Corporation,grub,1.96-8191,https://acme.arpa/packages/grub
```

We could conceivably support sub-components, but it's hard to conceive of a
scenario that would trigger a UEFI variable update that wouldn't justify a
hypervisor or kernel re-release to enforce that sub-component level from there.
Something like a "level 1.5 hypervisor" that can exist between different kernel
generations can be considered its own component.

Each component is assigned a **minimum global generation number**. Vendors signing
component binary artifacts with a specific global generation number are
required to include fixes for any public or pre-disclosed issue required for
that generation. Additionally, in the event that a bypass only manifests in a
specific product's component, vendors may ask for a product-specific generation
number to be published for one of their product's components. This avoids
triggering an industry wide re-publishing of otherwise safe components.

In the example above, 1 in `sbat,1` is sbat's minimum global generation number.

A **product-specific minimum generation number** only applies to the instance of
that component that is signed with that product name. Another product's
instance of the same component may be installed on the same system and would
not be subject to the other product's product-specific minimum generation
number. However, both of those components will need to meet the global minimum
generation number for that component. A very likely scenario would be that a
product is shipped with an incomplete fix required for a specific minimum
generation number, but is labeled with that number. Rather than having the
entire industry that uses that component re-release, just that product's
minimum generation number would be incremented and that product's component
re-released along with a UEFI variable update specifying that requirement.

In the example above, 1 in `grub.acme,1` is grub.acme's product-specific minimum
generation number.

The global and product-specific generation number name spaces are not tied to
each other. The global number is managed externally, and the vast majority of
products will never publish a minimum product-specific generation number for
any of their components. Unspecified, more specific generation numbers are
treated as 0.

A minimum feature set, for example enforced kernel lock down, may be required
as well to sign and label a component with a specific generation number. As
time goes on, it is likely that the minimum feature set required for the
currently valid generation number will expand. (For example, hypervisors
supporting UEFI Secure Boot guests may at some point require memory encryption
or similar protection mechanism.)

The footprint of the UEFI variable payload will expand as product-specific
generation numbers ahead of the global number are added. However, it will
shrink again as the global number for that component is incremented again.  The
expectation is that a product-specific or vendor-specific generation number is
a rare event, and that the generation number for the upstream code base will
suffice in most cases.

A product-specific generation number is needed if a CVE is fixed in code that
**only** exists in a specific product's branch. This would either be something
like product-specific patches, or a mis-merge that only occurred in that
product. Setting a product-specific generation number for such an event
eliminates the need for other vendors to have to re-release the binaries for
their products with an incremented global number.

Both generation numbers should only ever go up; they should never be reset.

#### Example: a vendor forking a global project

Let's imagine a fictional company named "Vendor C" having an active fork of the
upstream GNU GRUB2. Therefore, Vendor C provides their own product-specific
generation number. This is happening at the point in time, when the upstream
product's entry starts with `grub,3`, hence why Vendor C's product ships with
entries similar to:

```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,3,Free Software Foundation,[...]
grub.vendorc,1,Vendor C,[...]
```

Suddenly there is a global CVE disclosure and all vendors coordinate
to release fixed components on the disclosure date. This release bumps the
global generation number for GRUB from 3 to 4. Vendor C's product's binaries
are now shipped with the entries:

```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,4,Free Software Foundation,[...]
grub.vendorc,1,Vendor C,[...]
```

After this, the UEFI SBAT revocation variable (named *SbatLevel*) would be
updated to raise the mandatory minimal global generation number for GRUB to 4.

However, Vendor C mis-merges the patches into one of their products and does
not become aware of the fact that this mis-merge created an additional
vulnerability until after they have published a signed binary in that vulnerable
state. Vendor C's GRUB binary can now be abused to compromise their systems.

To remedy this, Vendor C will release a security-fixed binary with the same
global generation number and an updated product-specific generation number (set
to 2):

```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,4,Free Software Foundation,[...]
grub.vendorc,2,Vendor C,[...]
```

Again, in the perfect scenario, to provide the perfect security, the UEFI SBAT
revocation variable would then be set, so that GRUB with a global generation
number of only 4 or higher would be able to be booted, as well as Vendor C's
products with their number of only 2 or higher. See for yourself, how this looks
like, in the [SbatLevel_Variable.txt](./SbatLevel_Variable.txt) file.

If and when there is another upstream fix for a CVE that would bump the GRUB
global number to 5, this product-specific number can be dropped from the UEFI
*SbatLevel* variable (because the binaries starting with upstream's `grub,4`
entry would get denylisted anyway), but with the current consensus it's
important to keep the product-specific number shipped with the product's binary,
like in the case of Vendor C:

```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,5,Free Software Foundation,[...]
grub.vendorc,2,Vendor C,[...]
```

The goal is generally to limit end user impact with as few
re-releases as possible, while not creating an unnecessarily large UEFI
revocation variable payload.

|                                                                                      | prior to GRUB<br>first disclosure\* | after GRUB<br>first disclosure\* | after Vendor C's<br>first update | after Vendor C's<br>second update | after GRUB<br>second disclosure\* |
|--------------------------------------------------------------------------------------|-------------------------------------|----------------------------------|----------------------------------|-----------------------------------|-----------------------------------|
| GRUB global<br>generation number in<br>artifacts .sbat section                       | 3                                   | 4                                | 4                                | 4                                 | 5                                 |
| Vendor C's product<br>generation number in<br>artifact's .sbat section               | 1                                   | 1                                | 2                                | 3                                 | 3                                 |
| GRUB global<br>generation number in<br>UEFI *SbatLevel* variable                     | 3                                   | 4                                | 4                                | 4                                 | 5                                 |
| Vendor C's product<br>generation number in<br>UEFI *SbatLevel* variable              | not set                             | not set                          | 2                                | 3                                 | not set                           |

\* A disclosure is the event/date where a CVE and fixes for it are made public.

The product-specific generation number does not reset and continues to increase
over the course of these non-global events. Continuity of more specific
generation numbers must be maintained in this way in order to satisfy checks
against older revocation data.

The UEFI *SbatLevel* variable payload will be stored publicly in the shim source
base and identify the global generation associated with a product or
version-specific one. The payload is also built into shim to additionally limit
exposure.

#### Retiring Signed Releases

Products that have reached the end of their support life by definition no
longer receive patches. They are also generally not examined for CVEs. Allowing
such unsupported products to continue to participate in UEFI Secure Boot is at
the very least questionable. If an EoSL product is made up of commonly used
components, such as the GRUB and the Linux kernel, it is reasonable to assume
that the global generation numbers will eventually move forward and exclude
those products from booting on a UEFI Secure Boot enabled system. However a
product made up of GRUB and a closed source kernel is just as conceivable. In
that case the kernel version may never move forward once the product reaches
its end of support. Therefore it is recommended that the product-specific
generation number be incremented past the latest one shown in any binary for
that product, effectively disabling that product on UEFI Secure Boot enabled
systems.

A subset of this case would be a beta-release that may contain eventually
abandoned, experimental, kernel code. Such releases should have their
product-specific generation numbers incremented past the latest one shown in
any released, or unreleased, binary signed with a production key.

Until a release is retired in this manner, vendors are responsible for keeping
up with fixes for CVEs and ensuring that any known signed binaries containing
known CVEs are denied from booting on UEFI Secure Boot enabled systems via the
most up to date UEFI metadata.

#### Vendor Key Files

Even prior to or without moving to one-shim, it is desirable to get every
vendor onto as few shims as possible. Ideally a vendor would have a single shim
signed with their certificate embedded and then use that certificate to sign
additional `<Vendor>_key.EFI` key files that then contain all the keys that the
individual components for their products are signed with. This file name needs
to be registered at the time of shim review and should not be changed without
going back to a shim review. A vendor should be able to store as many
certificated (or a CA certificate) as they need for all the components of all
of their products. Older versions of this file can be revoked via SBAT. In
order to limit the footprint of the SBAT revocation metadata, it is vital that
vendors do not create additional key files beyond what they have been approved
for at shim review.

#### Key Revocations

Since Vendor Product keys are brought into Shim as signed binaries, generation
numbering can and should be used to revoke them in case of a private key
compromise.

#### Kernel support for SBAT

The initial SBAT implementation will add SBAT metadata to Shim and GRUB and
enforce SBAT on all components labeled with it. Until a component (e.g. the
Linux kernel) gains SBAT metadata it can not be revoked via SBAT, but only by
revoking the keys signing that component. These keys will live in
separate, product-specific signed PE files that contain **only** the
certificate and SBAT metadata for the key files. These key files can then be
revoked via SBAT in order to invalidate and replace a specific key. While
certificates built into Shim can be revoked via SBAT and Shim introspection,
this practice would still result in a proliferation of Shim binaries that would
need to be revoked via dbx in the event of an early Shim code bug. Therefore,
SBAT must be used in conjunction with separate Vendor Product Key binaries.

At the time of this writing, revoking a Linux kernel with a lockdown compromise
is not spelled out as a requirement for shim signing. In fact, with limited dbx
space and the size of the attack surface for lockdown it would be impractical
do so without SBAT. With SBAT it should be possible to raise the bar, and treat
lockdown bugs that would allow a kexec of a tampered kernel as revocations.

#### Kernels execing other kernels (aka kexec, fast reboot)

It is expected that kexec and other similar implementations of kernels spawning
other kernels will eventually consume and honor SBAT metadata. Until they do,
the same Vendor Product Key binary based revocation will need to be used for
them.

#### Generation-Based Revocation Metadata

Adding a .sbat section containing the SBAT metadata structure to PE images.

| field                | meaning                                                                          |
|----------------------|----------------------------------------------------------------------------------|
| component_name       | the name we're comparing                                                         |
| component_generation | the generation number for the comparison                                         |
| vendor_name          | human readable vendor name                                                       |
| vendor_package_name  | human readable package name                                                      |
| vendor_version       | human readable package version (maybe machine parseable too, not specified here) |
| vendor_url           | url to look stuff up, contact, whatever.                                         |

The format of this .sbat section is comma separated values, or more
specifically ASCII encoded strings.

## Example sbat sections

For grub, a build from a fresh checkout of upstream might have the following in
`.sbat`:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,1,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
```

A Fedora build believed to have exactly the same set of vulnerabilities plus
one that was never upstream might have:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,1,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.fedora,1,The Fedora Project,grub2,2.04-31.fc33,https://src.fedoraproject.org/rpms/grub2
```

Likewise, Red Hat has various builds for RHEL 7 and RHEL 8, all of which have
something akin to the following in `.sbat`:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,1,Free Software Foundation,grub,2.02,https://www.gnu.org/software/grub/
grub.fedora,1,Red Hat Enterprise Linux,grub2,2.02-0.34.fc24,mail:secalert@redhat.com
grub.rhel,1,Red Hat Enterprise Linux,grub2,2.02-0.34.el7_2,mail:secalert@redhat.com
```

The Debian package believed to have the same set of vulnerabilities as upstream
might have:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,1,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.debian,1,Debian,grub2,2.04-12,https://packages.debian.org/source/sid/grub2
```

Another party known for less than high quality software who carry a bunch of
out of tree grub patches on top of a very old grub version from before any of
the upstream vulns were committed to the tree.  They haven't ever had the
upstream vulns, and in fact have never shipped any vulnerabilities.  Their grub
`.sbat` might have the following (which we'd be very suspect of signing, but
hey, suppose it turns out to be correct):
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub.acme,1,Acme Corporation,grub,1.96-8191,https://acme.arpa/packages/grub
```

At the same time, we're all shipping the same `shim-16` codebase, and in our
`shim` builds, we all have the following in `.sbat`:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
shim,1,UEFI shim,shim,16,https://github.com/rhboot/shim
```

## How to add .sbat sections

Components that do not have special code to construct the final PE files can
simply add this section using objcopy(1):

```
objcopy --set-section-alignment '.sbat=512' --add-section .sbat=sbat.csv foo.efi

```

Older versions of objcopy(1) do not support --set-section-alignment which is
required to force the correct alignment expected from a PE file. As long as
there is another step, later in the build process, such as an linker invocation
that forces alignment, objcopy(1) does not need to align an intermediate file.

#### UEFI SBAT Variable content

The SBAT UEFI variable contains a descriptive form of all components used by
all UEFI signed Operating Systems, along with a minimum generation number for
each one. It may also contain a product-specific generation number, which in
turn may also specify version-specific generation numbers. It is expected that
specific generation numbers will be exceptions that will be obsoleted if and
when the global number for a component is incremented.

Initially the SBAT UEFI variable will set generation numbers for
components to 1, but is expected to grow as CVEs are discovered and
fixed. The following show the evolution over a sample set of events:

## Starting point

Before CVEs are encountered, an undesirable module was built into Fedora's
grub, so it's product-specific generation number has been bumped:

```
sbat,1
shim,1
grub,1
grub.fedora,2
```

## Along comes bug 1

Another kind security researcher shows up with a serious bug, and this one was
in upstream grub-0.94 and every version after that, and is shipped by all
vendors.

At this point, each vendor updates their grub builds, and updates the
`component_generation` in `.sbat` to `2`.  The GRUB upstream build now looks like:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,2,Free Software Foundation,grub,2.05,https://www.gnu.org/software/grub/
```

But Fedora's now looks like:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,2,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.fedora,2,The Fedora Project,grub2,2.04-33.fc33,https://src.fedoraproject.org/rpms/grub2
```

Other distros either rebase on 2.05 or theirs change similarly to Fedora's.  We
now have two options for Acme Corp:
- add a `grub.acme,2` entry to `SBAT`
- have Acme Corp add
  `grub,2,Free Software Foundation,grub,1.96,https://www.gnu.org/software/grub/`
  to their new build's `.sbat`

We talk to Acme and they agree to do the latter, thus saving flash real estate
to be developed on another day.  Their binary now looks like:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,2,Free Software Foundation,grub,1.96,https://www.gnu.org/software/grub/
grub.acme,1,Acme Corporation,grub,1.96-8192,https://acme.arpa/packages/grub
```

The UEFI CA issues an update which looks like:
```
sbat,1
shim,1
grub,2
grub.fedora,2
```

Which is literally the byte array:
```
{
  's', 'b', 'a', 't', ',', '1', '\n',
  's', 'h', 'i', 'm', ',', '1', '\n',
  'g', 'r', 'u', 'b', ',', '2', '\n',
  'g', 'r', 'u', 'b', '.', 'f', 'e', 'd', 'o', 'r', 'a', ',', '2', '\n',
}
```

## Acme Corp gets with the program

Acme at this point discovers some features have been added to grub and they
want them.  They ship a new grub build that's completely rebased on top of
upstream and has no known vulnerabilities.  Its `.sbat` data looks like:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,2,Free Software Foundation,grub,2.05,https://www.gnu.org/software/grub/
grub.acme,1,Acme Corporation,grub,2.05-1,https://acme.arpa/packages/grub
```

## Someone was wrong on the Internet and bug 2

Debian discovers that they actually shipped bug 0 as well (woops).  They
produce a new build which fixes it and has the following in `.sbat`:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,2,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.debian,2,Debian,grub2,2.04-13,https://packages.debian.org/source/sid/grub2
```

Before the UEFI CA has released an update, though, another upstream issue is
found.  Everybody updates their builds as they did for bug 1.  Debian also
updates theirs, as they would, and their new build has:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,3,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.debian,2,Debian,grub2,2.04-13,https://packages.debian.org/source/sid/grub2
```

And the UEFI CA issues an update to SBAT which has:
```
sbat,1
shim,1
grub,3
grub.fedora,2
```

The grub.fedora product-specific line could be dropped since a Fedora GRUB with
a global generation number that also contained the bug that prompted the
fedora-specific revocation was never published. This results in the following
reduced UEFI SBAT revocation update:
```
sbat,1
shim,1
grub,3
```

Two key things here:
- `grub.debian` still got updated to `2` in their `.sbat` data, because a
  vulnerability was fixed that is only covered by that updated number.
- There is still no `SBAT` update for `grub.debian`, because there's no binary
  that needs it which is not covered by updating `grub` to `3`.
