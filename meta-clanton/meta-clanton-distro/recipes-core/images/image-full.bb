DESCRIPTION = "A fully functional image to be placed on SD card"

IMAGE_INSTALL = "packagegroup-core-boot ${ROOTFS_PKGMANAGE_BOOTSTRAP} ${CORE_IMAGE_EXTRA_INSTALL}"

IMAGE_LINGUAS = " "

LICENSE = "GPLv2"

IMAGE_FSTYPES = "ext3 live"

inherit core-image

NOISO = "1"
NOHDD = "1"
IMAGE_ROOTFS_SIZE = "307200"

EXTRA_IMAGECMD_append_ext2 = " -N 2000"

IMAGE_FEATURES += "package-management"

IMAGE_INSTALL += "kernel-modules"
IMAGE_INSTALL += "ethtool pciutils"
IMAGE_INSTALL += "strace"
IMAGE_INSTALL += "linuxptp"
IMAGE_INSTALL += "libstdc++"
IMAGE_INSTALL += "dmidecode"

IMAGE_INSTALL += "opencv nodejs"
IMAGE_INSTALL += "python python-modules python-numpy python-opencv"
IMAGE_INSTALL += "alsa-lib alsa-utils alsa-tools"
IMAGE_INSTALL += "wireless-tools wpa-supplicant bluez4"
IMAGE_INSTALL += "ppp openssh"

IMAGE_INSTALL += "linux-firmware-iwlwifi-6000g2a-6"
IMAGE_INSTALL += "linux-firmware-iwlwifi-6000g2b-6"
IMAGE_INSTALL += "linux-firmware-iwlwifi-135-6"

IMAGE_INSTALL += "e2fsprogs-mke2fs e2fsprogs-e2fsck dosfstools util-linux-mkfs"

IMAGE_INSTALL += "connman"
remove_init_ifupdown () {
	#as long as we install connman in this image
	#we have to remove /etc/rc*/*networking
	rm /etc/rc*/networking -f
}
ROOTFS_POSTPROCESS_COMMAND += "remove_init_ifupdown ; "

GRUB_CONF = "grub.conf"
GRUB_PATH = "boot/grub/"
SRC_URI = "file://${GRUB_CONF}"

python () {
        # Ensure we run these usually noexec tasks
        d.delVarFlag("do_fetch", "noexec")
        d.delVarFlag("do_unpack", "noexec")
}

copy_grub_conf() {
  #append to grub kernel command line rootimage=image-name.ext3
  sed s:@ROOT_IMAGE@:$(basename ${ROOTFS}):g ${WORKDIR}/${GRUB_CONF} > ${WORKDIR}/${GRUB_CONF}.sed
  install -d ${DEPLOY_DIR_IMAGE}/${GRUB_PATH}
  install -m 0755 ${WORKDIR}/${GRUB_CONF}.sed ${DEPLOY_DIR_IMAGE}/${GRUB_PATH}/${GRUB_CONF}
}

python do_grub() {
    bb.build.exec_func('copy_grub_conf', d)
}

addtask grub after do_fetch before do_build
do_grub[nostamp] = "1"
