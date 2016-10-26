DESCRIPTION = "A small image just capable of allowing a device to boot."

IMAGE_INSTALL = "packagegroup-core-boot ${ROOTFS_PKGMANAGE_BOOTSTRAP} ${CORE_IMAGE_EXTRA_INSTALL}"

IMAGE_LINGUAS = " "

LICENSE = "GPLv2"

inherit core-image

IMAGE_FSTYPES = "${INITRAMFS_TYPES}"
IMAGE_ROOTFS_SIZE = "2048"
EXTRA_IMAGECMD_append_ext2 = " -N 2000"

IMAGE_FEATURES += "package-management"
#ROOTFS_POSTPROCESS_COMMAND += "remove_packaging_data_files ; "

IMAGE_INSTALL += "kernel-modules"
IMAGE_INSTALL += "ethtool"
IMAGE_INSTALL += "strace"
IMAGE_INSTALL += "ppp"
IMAGE_INSTALL += "linuxptp"
IMAGE_INSTALL += "libstdc++"
IMAGE_INSTALL += "dmidecode"
