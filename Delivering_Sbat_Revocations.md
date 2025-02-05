When new sbat based revocations become public they are added to
https://github.com/rhboot/shim/blob/main/SbatLevel_Variable.txt They
are identified by their year, month, day, counter YYYYMMDDCC field in
the header.

If secure boot is disabled, shim will always clear the applied
revocations.

shim binaries will include the opt-in latest revocation payload
available at the time that they are built. This can be applied by
running mokutil --set-sbat-policy latest and rebooting with the new
shim binary in place. A shim build can also specify a
-DSBAT_AUTOMATIC_DATE=YYYYMMDDCC on the command line which will
include and automatically apply that revocation. shim will never
downgrade a revocation. The only way to roll back is to disable secure
boot, load shim to clear the revocations and then re-apply the desired
level.

In addition to building revocation levels into shim, they can also be
delivered via a revocations_sbat.efi binary. These binaries can be
created from the https://github.com/rhboot/certwrapper
repository. This repository uses the same
https://github.com/rhboot/shim/blob/main/SbatLevel_Variable.txt file
as the source of the revocation metadata. Both
SBAT_LATEST_DATE=YYYYMMDDCC and SBAT_AUTOMATIC_DATE=YYYYMMDDCC can be
specified there. These files need to be signed with a certificate that
your shim trusts. These files can be created without the need to
deliver a new shim and can be set to have shim automatically apply a
new revocations whey they are delivered into the system partition.
