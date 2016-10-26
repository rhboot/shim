DESCRIPTION = "List of drivers to be auto-loaded"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://quark-init.sh \
           file://galileo.conf \
           file://galileo_gen2.conf"

INSTALLDIR = "/etc/modules-load.quark"
FILES_${PN} += "${INSTALLDIR}"
FILES_${PN}-dbg += "${INSTALLDIR}/.debug"

do_install() {
        install -d ${D}${INSTALLDIR}
        install -m 0755 ${WORKDIR}/galileo.conf ${D}${INSTALLDIR}/
        install -m 0755 ${WORKDIR}/galileo_gen2.conf ${D}${INSTALLDIR}/
        install -d ${D}${sysconfdir}/init.d
        install -m 0755 ${WORKDIR}/quark-init.sh ${D}${sysconfdir}/init.d
}

inherit update-rc.d

INITSCRIPT_NAME = "quark-init.sh"
INITSCRIPT_PARAMS = "start 75 5 ."

