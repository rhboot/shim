FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://modules.conf"

IMAGE_INSTALL = "initramfs-live-boot busybox base-passwd udev"

IMAGE_INSTALL += "kernel-module-usb-storage"
IMAGE_INSTALL += "kernel-module-ehci-hcd kernel-module-ehci-pci kernel-module-ohci-hcd"
IMAGE_INSTALL += "kernel-module-stmmac"
IMAGE_INSTALL += "kernel-module-sdhci kernel-module-sdhci-pci kernel-module-mmc-block"

