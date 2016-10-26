FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://bsp.conf"

INSTALLDIR = "/etc/modules-load.d"
FILES_${PN} += "${INSTALLDIR}"
FILES_${PN}-dbg += "${INSTALLDIR}/.debug"

do_install_append () {
	install -d ${D}${INSTALLDIR}
        install -m 0755 ${WORKDIR}/bsp.conf ${D}${INSTALLDIR}/
}

