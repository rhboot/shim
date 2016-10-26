FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://uclibc-enable.patch"
DEPENDS += "virtual/libiconv"
