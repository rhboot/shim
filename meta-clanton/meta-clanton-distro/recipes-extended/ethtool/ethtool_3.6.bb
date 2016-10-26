SUMMARY = "Display or change ethernet card settings"
DESCRIPTION = "A small utility for examining and tuning the settings of your ethernet-based network interfaces."
HOMEPAGE = "http://www.kernel.org/pub/software/network/ethtool/"
SECTION = "console/network"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=94d55d512a9ba36caa9b7df079bae19f \
                    file://ethtool.c;firstline=4;endline=17;md5=594311a6703a653a992f367bd654f7c1"

SRC_URI = "${KERNELORG_MIRROR}/software/network/ethtool/ethtool-${PV}.tar.gz" 
SRT_URI += "file://clanton.patch"

SRC_URI[md5sum] = "1f4fb25590d26cb622bf3bf03413cfac"
SRC_URI[sha256sum] = "a0305779bc35893cf4683a9cbc47bbd1d726a142ce2b34384ae3d87fa8e44891"


inherit autotools

#S = "${WORKDIR}/git"

