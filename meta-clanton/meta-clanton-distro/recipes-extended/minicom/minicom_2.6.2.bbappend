FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

# patch to make -S parameter able to take 256 characters long path
# instead of original 33
# http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=531813
SRC_URI += "file://long_path_script.patch;patchdir=${WORKDIR}/${P}"
DEPENDS += "virtual/libiconv"
