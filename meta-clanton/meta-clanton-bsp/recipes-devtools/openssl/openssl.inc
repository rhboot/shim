SUMMARY = "Secure Socket Layer"
DESCRIPTION = "Secure Socket Layer (SSL) binary and related cryptographic tools."
HOMEPAGE = "http://www.openssl.org/"
BUGTRACKER = "http://www.openssl.org/news/vulnerabilities.html"
SECTION = "libs/network"

# Big Jump for OpenSSL 1.0 support with meta-oe
INC_PR = "r15"

# "openssl | SSLeay" dual license
LICENSE = "openssl"
LIC_FILES_CHKSUM = "file://LICENSE;md5=f9a8f968107345e0b75aa8c2ecaa7ec8"

DEPENDS = "perl-native-runtime"

SRC_URI = "http://www.openssl.org/source/openssl-${PV}.tar.gz \
          "
S = "${WORKDIR}/openssl-${PV}"

AR_append = " r"
# Avoid binaries being marked as requiring an executable stack since it 
# doesn't(which causes and this causes issues with SELinux
CFLAG = "${@base_conditional('SITEINFO_ENDIANNESS', 'le', '-DL_ENDIAN', '-DB_ENDIAN', d)} \
	-DTERMIO ${CFLAGS} -Wall -Wa,--noexecstack"

# -02 does not work on mipsel: ssh hangs when it tries to read /dev/urandom
CFLAG_mtx-1 := "${@'${CFLAG}'.replace('-O2', '')}"
CFLAG_mtx-2 := "${@'${CFLAG}'.replace('-O2', '')}"

export DIRS = "crypto ssl apps"
export EX_LIBS = "-lgcc -ldl"
export AS = "${CC} -c"

inherit pkgconfig siteinfo

PACKAGES =+ "libcrypto libssl ${PN}-misc"
FILES_libcrypto = "${base_libdir}/libcrypto${SOLIBS}"
FILES_libssl = "${libdir}/libssl.so.*"
FILES_${PN} =+ " ${libdir}/ssl/*"
FILES_${PN}-misc = "${libdir}/ssl/misc ${libdir}/ssl/openssl.cnf"
FILES_${PN}-dev += "${base_libdir}/libcrypto${SOLIBSDEV}"

do_configure_prepend_darwin () {
	sed -i -e '/version-script=openssl\.ld/d' Configure
}

do_configure () {
	cd util
	perl perlpath.pl ${STAGING_BINDIR_NATIVE}
	cd ..
	ln -sf apps/openssl.pod crypto/crypto.pod ssl/ssl.pod doc/

	os=${HOST_OS}
	if [ "x$os" = "xlinux-uclibc" ]; then
		os=linux
	elif [ "x$os" = "xlinux-uclibceabi" ]; then
		os=linux
	elif [ "x$os" = "xlinux-gnueabi" ]; then
		os=linux
	fi
	target="$os-${HOST_ARCH}"
	case $target in
	linux-arm)
		target=linux-elf-arm
		;;
	linux-armeb)
		target=linux-elf-armeb
		;;
	linux-sh3)
		target=debian-sh3
		;;
	linux-sh4)
		target=debian-sh4
		;;
	linux-i486)
		target=debian-i386-i486
		;;
	linux-i586 | linux-viac3)
		target=debian-i386-i586
		;;
	linux-i686)
		target=debian-i386-i686/cmov
		;;
	linux-gnux32-x86_64)
		target=linux-x32
		;;
	linux-gnu64-x86_64)
		target=linux-x86_64
		;;
	linux-mips)
		target=debian-mips
		;;
	linux-mipsel)
		target=debian-mipsel
		;;
        linux-*-mips64)
               target=linux-mips
                ;;
	linux-powerpc)
		target=linux-ppc
		;;
	linux-gnuspe-powerpc)
		target=linux-ppc
		;;
	linux-powerpc64)
		target=linux-ppc64
		;;
	linux-supersparc)
		target=linux-sparcv8
		;;
	linux-sparc)
		target=linux-sparcv8
		;;
	darwin-i386)
		target=darwin-i386-cc
		;;
	esac
	# inject machine-specific flags
	sed -i -e "s|^\(\"$target\",\s*\"[^:]\+\):\([^:]\+\)|\1:${CFLAG}|g" Configure
        useprefix=${prefix}
        if [ "x$useprefix" = "x" ]; then
                useprefix=/
        fi        
	perl ./Configure ${EXTRA_OECONF} shared --prefix=$useprefix --openssldir=${libdir}/ssl --libdir=`basename ${libdir}` $target
}

do_compile () {
	oe_runmake
}

do_install () {
	oe_runmake INSTALL_PREFIX="${D}" MANDIR="${mandir}" install

	oe_libinstall -so libcrypto ${D}${libdir}
	oe_libinstall -so libssl ${D}${libdir}

	# Moving libcrypto to /lib
	if [ ! ${D}${libdir} -ef ${D}${base_libdir} ]; then
		mkdir -p ${D}/${base_libdir}/
		mv ${D}${libdir}/libcrypto* ${D}${base_libdir}/
		sed -i s#libdir=\$\{exec_prefix\}\/lib#libdir=${base_libdir}# ${D}/${libdir}/pkgconfig/libcrypto.pc
	fi

	install -d ${D}${includedir}
	cp --dereference -R include/openssl ${D}${includedir}
	sed -i -e '1s,.*,#!${bindir}/env perl,' ${D}${libdir}/ssl/misc/CA.pl

}

BBCLASSEXTEND = "native nativesdk"

