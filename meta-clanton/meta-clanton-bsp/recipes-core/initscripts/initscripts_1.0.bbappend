FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://galileod.sh"

do_install_append() {
	install -d ${D}${sysconfdir}
	install -m 0755 ${WORKDIR}/galileod.sh ${D}${sysconfdir}/init.d
}

inherit update-rc.d

INITSCRIPT_NAME = "galileod.sh"
INITSCRIPT_PARAMS = "start 80 5 ."

