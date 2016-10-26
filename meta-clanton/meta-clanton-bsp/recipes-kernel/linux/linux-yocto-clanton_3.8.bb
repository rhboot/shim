# linux-yocto-custom.bb:
#
#   An example kernel recipe that uses the linux-yocto and oe-core
#   kernel classes to apply a subset of yocto kernel management to git
#   managed kernel repositories.
#
#   To use linux-yocto-custom in your layer, create a
#   linux-yocto-custom.bbappend file containing at least the following
#   lines:
#
#     FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
#     COMPATIBLE_MACHINE_yourmachine = "yourmachine"
#
#   You must also provide a Linux kernel configuration. The most direct
#   method is to copy your .config to files/defconfig in your layer,
#   in the same directory as the bbappend.
#
#   To use the yocto kernel tooling to generate a BSP configuration
#   using modular configuration fragments, see the yocto-bsp and
#   yocto-kernel tools documentation.
#
# Warning:
#
#   Building this example without providing a defconfig or BSP
#   configuration will result in build or boot errors. This is not a
#   bug.
#
#
# Notes:
#
#   patches: patches can be merged into to the source git tree itself,
#            added via the SRC_URI, or controlled via a BSP
#            configuration.
#   
#   example configuration addition:
#            SRC_URI += "file://smp.cfg"
#   example patch addition (for kernel v3.4 only):
#            SRC_URI += "file://0001-linux-version-tweak.patch
#   example feature addition (for kernel v3.4 only):
#            SRC_URI += "file://feature.scc"
#

inherit kernel
require recipes-kernel/linux/linux-yocto.inc

# Override SRC_URI in a bbappend file to point at a different source
# tree if you do not want to build from Linus' tree.

SRC_URI = "git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git;branch=linux-3.8.y"

SRC_URI += "file://quark.cfg"
SRC_URI += "file://clanton-standard.scc"

LINUX_VERSION ?= "3.8"
LINUX_VERSION_EXTENSION ?= "-clanton"

# Override SRCREV to point to a different commit in a bbappend file to
# build a different release of the Linux kernel.
SRCREV = "531ec28f9f26f78797124b9efcf2138b89794a1e"

PR = "r0"
PV = "${LINUX_VERSION}"

# Override COMPATIBLE_MACHINE to include your machine in a bbappend
# file. Leaving it empty here ensures an early explicit build failure.
COMPATIBLE_MACHINE = "clanton"

RDEPENDS_kernel-base=""

SRC_URI += "file://0001-tty-don-t-deadlock-while-flushing-workqueue-quark.patch"
SRC_URI += "file://0002-driver-core-constify-data-for-class_find_devic-quark.patch"
SRC_URI += "file://0003-TTY-mark-tty_get_device-call-with-the-proper-c-quark.patch"
SRC_URI += "file://0004-pwm-Add-sysfs-interface-quark.patch"
SRC_URI += "file://0005-drivers-pwm-sysfs.c-add-export.h-RTC-50404-quark.patch"
SRC_URI += "file://0006-core-Quark-patch-quark.patch"
SRC_URI += "file://0007-Quark-Platform-Code-quark.patch"
SRC_URI += "file://0008-Quark-UART-quark.patch"
SRC_URI += "file://0009-EFI-capsule-update-quark.patch"
SRC_URI += "file://0010-Quark-SDIO-host-controller-quark.patch"
SRC_URI += "file://0011-Quark-USB-host-quark.patch"
SRC_URI += "file://0012-USB-gadget-serial-quark.patch"
SRC_URI += "file://0013-Quark-stmmac-Ethernet-quark.patch"
SRC_URI += "file://0014-Quark-GPIO-2-2-quark.patch"
SRC_URI += "file://0015-Quark-GPIO-1-2-quark.patch"
SRC_URI += "file://0016-Quark-GIP-Cypress-I-O-expander-quark.patch"
SRC_URI += "file://0017-Quark-I2C-quark.patch"
SRC_URI += "file://0018-Quark-sensors-quark.patch"
SRC_URI += "file://0019-Quark-SC-SPI-quark.patch"
SRC_URI += "file://0020-Quark-IIO-quark.patch"
SRC_URI += "file://0021-Quark-SPI-flash-quark.patch"

SRC_URI += "file://uart-1.0.patch"
SRC_URI += "file://GAL-118-USBDeviceResetOnSUSRES-2.patch"
