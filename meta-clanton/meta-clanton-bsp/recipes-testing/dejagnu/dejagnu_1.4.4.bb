DESCRIPTION = "GNU unit testing framework, written in Expect and Tcl"
LICENSE = "GPLv2"
SECTION = "devel"
PR = "r1"

#set1hr_timeout.patch increases default 5min timeout to 1hour;
#due to Yocto being very resource demanding such a timeout makes sense
SRC_URI = "ftp://ftp.gnu.org/gnu/dejagnu/dejagnu-${PV}.tar.gz \
           file://set1hr_timeout.patch"

inherit autotools


SRC_URI[md5sum] = "053f18fd5d00873de365413cab17a666"
SRC_URI[sha256sum] = "d0fbedef20fb0843318d60551023631176b27ceb1e11de7468a971770d0e048d"

LIC_FILES_CHKSUM = "file://COPYING;md5=c93c0550bd3173f4504b2cbd8991e50b"
