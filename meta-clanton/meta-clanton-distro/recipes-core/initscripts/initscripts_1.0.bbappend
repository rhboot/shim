FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}-${PV}:"

SRC_URI += "file://mdev.sh \
            file://mqueue.sh \
            file://sysfs_devtmpfs.sh \
            file://mdev.conf \
            file://automount.sh"

do_install_append() {
	install -d ${D}${sysconfdir}
	install -d ${D}${sysconfdir}/rcS.d
	install -m 0755 ${WORKDIR}/mdev.sh ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/mqueue.sh ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/sysfs_devtmpfs.sh ${D}${sysconfdir}/init.d
	ln -sf ../init.d/mdev.sh ${D}${sysconfdir}/rcS.d/S12mdev.sh
	ln -sf ../init.d/mqueue.sh ${D}${sysconfdir}/rcS.d/S12mqueue.sh
	ln -sf ../init.d/sysfs_devtmpfs.sh	${D}${sysconfdir}/rcS.d/S02sysfs_devtmpfs.sh
	install -m 0755 ${WORKDIR}/mdev.conf ${D}${sysconfdir}
	install -d ${D}/usr/bin
	install -m 0755 ${WORKDIR}/automount.sh ${D}/usr/bin/
}

