SUMMARY = "GRUB is the GRand Unified Bootloader"
DESCRIPTION = "GRUB is a GPLed bootloader intended to unify bootloading across x86 \
operating systems. In addition to loading the Linux kernel, it implements the Multiboot \
standard, which allows for flexible loading of multiple boot images."
HOMEPAGE = "http://www.gnu.org/software/grub/"
SECTION = "bootloaders"

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=c93c0550bd3173f4504b2cbd8991e50b \
                    file://grub/main.c;beginline=3;endline=9;md5=22a5f28d2130fff9f2a17ed54be90ed6"

PV = "0.97+git${SRCPV}"

#check if this is needed 
RDEPENDS_${PN} = "diffutils"
DEPENDS = "gnu-efi"
PR = "r0"

SRC_URI = "git://github.com/mjg59/grub-fedora.git"

SRC_URI += "file://clanton.patch"

CFLAGS_append = " -Os -fno-strict-aliasing -Wall -Werror -Wno-shadow -Wno-unused  -Wno-pointer-sign \
  -DINTEL_QUARK_TEST=1"

S = "${WORKDIR}/git"
SEPB = "${S}"
#SRCREV = "${AUTOREV}"
SRCREV = "5775f32a6268dead6939d01cbe72f23972f6d3c0"

inherit autotools deploy

EXTRA_OECONF = "--without-curses --disable-auto-linux-mem-opt --with-platform=efi --libdir=${STAGING_LIBDIR}"

GRUB_TARGET = "i386"
GRUB_IMAGE = "/efi/grub.efi"

COMPATIBLE_HOST = "i.86.*-linux"

do_deploy() {
  install -m 644 ${S}/${GRUB_IMAGE} ${DEPLOYDIR}
}
addtask deploy after do_install before do_build

do_install[noexec] = "1"
do_populate_sysroot[noexec] = "1"
