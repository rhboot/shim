DESCRIPTION = "Linux PTP application"
SECTION = "net"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "git://git.code.sf.net/p/linuxptp/code"

SRCREV = "e6bbbb27e1da82aa180877f04f2efe1be425fc0a"

S = "${WORKDIR}/git"

EXTRA_OEMAKE = "'CC=${CC}' 'CFLAGS=${CFLAGS}' 'KBUILD_OUTPUT=${STAGING_DIR_TARGET}'"

INSTALL_BINS = "ptp4l" 
#This could also include: hwstamp_ctl phc2sys pmc

do_install() {
  install -d ${D}${sbindir}
  install -m 0755 ${INSTALL_BINS} ${D}${sbindir}
}
