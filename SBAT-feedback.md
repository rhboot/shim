# In general

SBAT looks like a really good way of fixing the revocation problem. Yay!

# Summary

In the summary, there are two solutions outlined

1. image metadata (SBAT, AFAICS!)
2. using a single shim image build for all vendors

It's not clear if **both** of these are expected to be used here or
not. Does SBAT depend on the single shim build? (I don't think
so?). SBAT looks sane enough, I think, but the single image build has
a multitude of possible issues for vendors. Probably OT to list all
the details here.

The whole thing is (mostly) orthogonal to existing vendor signing
processes and keys. That looks like a good thing - we're just
re-working how **revocation** works. We're still missing a good
cross-vendor discussion on how to manage keys etc., but that's a topic
for another day.

#### Generation-Based Revocation Overview

(Maybe slightly OT) How does use of
EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS work? Does that
have problems on platforms without a working RTC? I'm assuming integer
rollover is checked for?

Who's responsible for defining global generation numbers? I'm guessing
we'll need a central group to co-ordinate things here.

#### Retiring Signed Releases

If we start to ratchet up the minimum feature set (and generation
number) in various places, that's likely to break some end-user
systems if they take updates. Ditto for users with EoSL
installations. Is it worth considering an extra setting somehwere as a
half-way house for the user to say "I want some of the SB benefits,
but my stuff doesn't meet full current spec" ?

#### Key Revocations

How does generation numbering work for keys? Do we use a specific
per-vendor "Vendor Key" component added somewhere in the SBAT
variable. This is described briefly in the SBAT variable content
below, but should probably be explained in more detail.

#### Version-Based Revocation Metadata

Yay, you've explicitly added versioning on the SBAT metadata
structure! \o/

The spec says UTF-8 for the .sbat section. Other stuff we do with PE
is all UCS-2 AFAIK, would we not be better to stay consistent here?

#### UEFI SBAT Variable content

AIUI the proposal is to include information for *all* known
vendors/products/components in the SBAT variable. Even if most of the
generation numbers collapse back to the base, will that fit? Will it
fit in future? (AKA "how many vendors, products and components do we
expect?"). I certainly *hope* so, but I don't see any estimates for
sizes.
