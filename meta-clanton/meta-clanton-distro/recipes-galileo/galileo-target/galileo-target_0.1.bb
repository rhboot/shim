DESCRIPTION = "Clanton Software Sketch Framework"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "file://galileo-target.tar.bz2 \
           file://launcher.sh \
           file://usb_improv_patch-1.patch \
           file://GAL-193-clloader-1.patch \
           file://GAL-199-start_spi_upgrade-1.patch \
           file://MAKER-222-Sketch_download_unstable-5.patch \
           file://GAL-200-zombie_clloader_c-1.patch \
           file://GAL-200-zombie_clloader_h-1.patch"

INSTALLDIR = "/opt/cln/galileo"
FILES_${PN} += "${INSTALLDIR}"
FILES_${PN}-dbg += "${INSTALLDIR}/.debug"

do_compile() {
	make
}

do_install() {
	oe_runmake install DESTDIR=${D}/ 

        install -d ${D}${sysconfdir}
        install -m 0755 ${WORKDIR}/launcher.sh ${D}${INSTALLDIR}/
}



