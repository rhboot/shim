FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
PRINC = "1"

do_install_append_clanton-tiny() {
	install -d ${D}${sysconfdir}
	echo "kernel.hotplug = /sbin/mdev" >> ${D}${sysconfdir}/sysctl.conf
}
