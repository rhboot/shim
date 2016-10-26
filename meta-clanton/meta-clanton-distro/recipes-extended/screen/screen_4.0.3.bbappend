FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

#ugly patch to get rid of #include <sys/stropts.h>
SRC_URI += "file://screen_uclibc.patch;patchdir=${WORKDIR}/${P}"

