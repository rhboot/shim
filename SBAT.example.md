SBAT: Current proposal
================

`.sbat` section
-------------

the `.sbat` section has the following fields:
| field | meaning |
|---|---|
| component_name | the name we're comparing
| component_generation | the generation number for the comparison
| vendor_name | human readable vendor name
| vendor_package_name | human readable package name
| vendor_version | human readable package version (maybe machine parseable too, not specified here)
| vendor_url | url to look stuff up, contact, whatever.

`SBAT` EFI variable
-----------------
The SBAT EFI variable (`SBAT-605dab50-e046-4300-abb6-3dd810dd8b23`) is structured as a series of records:

```
struct sbat_entry {
  uint16_t component_generation;
  uint16_t component_name_size;
  char component_name[];
}
```

with the first record being our structure version number expressed as: `{ 0, 5, "sbat" }`

Validation Rules
-----------------
- If an `SBAT` variable is set, binaries validated directly by shim (i.e. not with the API) get validated using SBAT
- If a binary validated by the API *does* have a `.sbat` section, it also gets validating using SBAT
- If a binary is enrolled in `db` by its hash rather than by certificate, the validation result is logged only, not enforced
- When validating SBAT, any `component_name` that's in both `SBAT` and `.sbat` gets compared
- `component_generation` in `.sbat` must be >= `component_generation` of the identical `component_name` in `SBAT`
- that version comparison includes the `"sbat"` entry

Example universe starting point
-------------------------------
For grub, a build from a fresh checkout of upstream might have the following in
`.sbat`:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,0,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
```

A Fedora build believed to have exactly the same set of vulnerabilities plus
one that was never upstream might have:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,0,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.fedora,0,The Fedora Project,grub2,2.04-31.fc33,https://src.fedoraproject.org/rpms/grub2
```

Likewise, Red Hat has various builds for RHEL 7 and RHEL 8, all of which have
something akin to the following in `.sbat`:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,0,Free Software Foundation,grub,2.02,https://www.gnu.org/software/grub/
grub.fedora,0,Red Hat Enterprise Linux,grub2,2.02-0.34.fc24,mail:secalert@redhat.com
grub.rhel,0,Red Hat Enterprise Linux,grub2,2.02-0.34.el7_2,mail:secalert@redhat.com
```

The Debian package believed to have the same set of vulnerabilities as upstream
might have:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,0,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.debian,0,Debian,grub2,2.04-12,https://packages.debian.org/source/sid/grub2
```

Another party known for less than high quality software who carry a bunch of
out of tree grub patches on top of a very old grub version from before any of
the upstream vulns were committed to the tree.  They haven't ever had the
upstream vulns, and in fact have never shipped any vulnerabilities.  Their grub
`.sbat` might have the following (which we'd be very suspect of signing, but
hey, suppose it turns out to be correct):
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub.acme,0,Acme Corporation,grub,1.96-8191,https://acme.arpa/packages/grub
```

At the same time, we're all shipping the same `shim-16` codebase, and in our
`shim` builds, we all have the following in `.sbat`:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
```

The `SBAT` data we've all agreed on and the UEFI CA has distributed is:
```
{
  { 0, 5, "sbat" },
  { 0, 5, "shim" },
  { 0, 5, "grub" },
  { 0, 12, "grub.fedora" },
}
```

Along comes bug 0
-----------------
Let's say we find a bug in Fedora that's serious, but it's only in Fedora's
patches, because we're the dumb ones.  Fedora issues a new shim build with the
following `.sbat` info:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,0,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.fedora,1,The Fedora Project,grub2,2.04-32.fc33,https://src.fedoraproject.org/rpms/grub2
```

For some (clearly insane) reason, 9 RHEL builds are also produced with the same
fixes and the same data in `.sbat` that Fedora has.  The RHEL 7.2 update (just
one example, same one as the RHEL example above) has the following in its
`.sbat` data:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,0,Free Software Foundation,grub,2.02,https://www.gnu.org/software/grub/
grub.fedora,1,Red Hat Enterprise Linux,grub2,2.02-0.34.fc24,mail:secalert@redhat.com
grub.rhel,1,Red Hat Enterprise Linux,grub2,2.02-0.34.el7_2.1,mail:secalert@redhat.com
```

The UEFI CA issues a new `SBAT` update which looks like:
```
{
  { 0, 5, "sbat" },
  { 0, 5, "shim" },
  { 0, 5, "grub" },
  { 1, 12, "grub.fedora" },
}
```

Along comes bug 1
-----------------
Another kind security researcher shows up with a serious bug, and this one was
in upstream grub-0.94 and every version after that, and is shipped by all
vendors.

At this point, each vendor updates their grub builds, and updates the
`component_generation` in `.sbat` to `1`.  The upstream build now looks like:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,1,Free Software Foundation,grub,2.05,https://www.gnu.org/software/grub/
```

But Fedora's now looks like:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,1,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.fedora,1,The Fedora Project,grub2,2.04-33.fc33,https://src.fedoraproject.org/rpms/grub2
```

Other distros either rebase on 2.05 or theirs change similarly to Fedora's.  We now have two options for Acme Corp:
- add a `{ 1, 10, "grub.acme" }` entry to `SBAT`
- have Acme Corp add `grub,1,Free Software Foundation,grub,1.96,https://www.gnu.org/software/grub/` to their new build's `.sbat`

We talk to Acme and they agree to do the latter, thus saving flash real estate
to be developed on another day:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,1,Free Software Foundation,grub,1.96,https://www.gnu.org/software/grub/
grub.acme,0,Acme Corporation,grub,1.96-8192,https://acme.arpa/packages/grub
```

The UEFI CA issues an update which looks like:
```
{
  { 0, 5, "sbat" },
  { 0, 5, "shim" },
  { 1, 5, "grub" },
  { 1, 12, "grub.fedora" },
}
```

Acme Corp gets with the program
-------------------------------
Acme at this point discovers some features have been added to grub and they
want them.  They ship a new grub build that's completely rebased on top of
upstream and has no known vulnerabilities.  Its `.sbat` data looks like:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,1,Free Software Foundation,grub,2.05,https://www.gnu.org/software/grub/
grub.acme,0,Acme Corporation,grub,2.05-1,https://acme.arpa/packages/grub
```

Someone was wrong on the internet and bug 2
-------------------------------------------
Debian discovers that they actually shipped bug 0 as well (woops).  They
produce a new build which fixes it and has the following in `.sbat`:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,1,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.debian,1,Debian,grub2,2.04-13,https://packages.debian.org/source/sid/grub2
```

Before the UEFI CA has released an update, though, another upstream issue is
found.  Everybody updates their builds as they did for bug 1.  Debian also
updates theirs, as they would, and their new build has:
```
sbat,1,SBAT Version,sbat,1,https://github.com/rhboot/shim/blob/main/SBAT.md
grub,2,Free Software Foundation,grub,2.04,https://www.gnu.org/software/grub/
grub.debian,1,Debian,grub2,2.04-13,https://packages.debian.org/source/sid/grub2
```

And the UEFI CA issues an update to SBAT which has:
```
{
  { 0, 5, "sbat" },
  { 0, 5, "shim" },
  { 2, 5, "grub" },
  { 1, 12, "grub.fedora" },
}
```

Two key things here:
- `debian.grub` still got updated to `1` in their `.sbat` data, because a vuln was fixed that is only covered by that updated number.
- There is still no `SBAT` update for `debian.grub`, because there's no binary that needs it which is not covered by updating `grub` to `2`.
