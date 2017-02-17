require openssl.inc

# For target side versions of openssl enable support for OCF Linux driver
# if they are available.
DEPENDS += "ocf-linux"

CFLAG += "-DHAVE_CRYPTODEV -DUSE_CRYPTODEV_DIGESTS"

PR = "${INC_PR}.3"

LIC_FILES_CHKSUM = "file://LICENSE;md5=27ffa5d74bb5a337056c14b2ef93fbf6"

export DIRS = "crypto ssl apps engines"
export OE_LDFLAGS="${LDFLAGS}"

SRC_URI += " \
            file://debian/debian-targets.patch \
            file://debian/no-rpath.patch \
            file://0001-OpenSSL-1.0.2-Change-LIBDIR-engines-to-LIBDIR-ssl-en.patch \
            file://find.pl \
           "

SRC_URI[md5sum] = "f965fc0bf01bf882b31314b61391ae65"
SRC_URI[sha256sum] = "6b3977c61f2aedf0f96367dcfb5c6e578cf37e7b8d913b4ecb6643c3cb88d8c0"

PACKAGES =+ " \
	${PN}-engines \
	${PN}-engines-dbg \
	"

FILES_${PN}-engines = "${libdir}/ssl/engines/*.so ${libdir}/engines"
FILES_${PN}-engines-dbg = "${libdir}/ssl/engines/.debug"

PARALLEL_MAKE = ""
PARALLEL_MAKEINST = ""

do_configure_prepend() {
  cp ${WORKDIR}/find.pl ${S}/util/find.pl
}
