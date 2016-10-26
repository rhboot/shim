################################################################################
# Copyright (c) 2013 Intel Corporation.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Intel Corporation nor the names of its
# contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
################################################################################

# Marc.Herbert@intel.com

################################################################################
# Optional feature to insert/overwrite kernel modules
# into an existing cpio initramfs
################################################################################


# Override these on the command line or create a symbolic link in the
# current directoryw, e.g.:
#    ln -s /p/clanton/swbuilds/Yocto/latest/core-image-spi-clanton.cpio.gz initramfs_orig.cpio.gz
RAMFS_ORIG         ?= ./initramfs_orig.cpio.gz
INSTALL_MOD_PATH   ?= .

# Work directory
RAMFS_WORK_DIR     := initramfs.dir
RAMFS_NEW          := initramfs_new.cpio

# Alias for all compression types
ramfs: ${RAMFS_NEW}.gz ${RAMFS_NEW}.lzma

# RAMFS_NEW must be rebuilt everytime because its dependency has no
# usable timestamp
.PHONY: ramfs-clean ramfs ${RAMFS_NEW}

ramfs-clean:
	${RM} ${RAMFS_NEW} ${RAMFS_NEW}.gz ${RAMFS_NEW}.lzma
	${RM} -r ${RAMFS_WORK_DIR}

# Extract RAMFS_ORIG if missing
${RAMFS_WORK_DIR}:
	mkdir "$@"
	# Exclude dev nodes since they require root
	{ zcat "${RAMFS_ORIG}" || rmdir "$@"; } | ( cd "$@" && cpio -idf 'dev/*' )

# Update RAMFS directory and re-archive
${RAMFS_NEW}: ${RAMFS_WORK_DIR}
	# We do not have room for several kernel versions and even
	# more importantly we do not want to mix drivers from
	# different places: scrub any existing drivers with --delete
	rsync -a --delete ${INSTALL_MOD_PATH}/lib/modules/ "$^"/lib/modules/
	# Modify the inittab to make the getty work with the emulation platform
	sed -i s/^S:2345/\#S:2345/ "$^"/etc/inittab
	echo "qrk1:2345:respawn:/sbin/getty 9600 ttyQRK1" >> "$^"/etc/inittab
	( cd $^ && find . | cpio -o -H newc --owner root:root ) > $@

# Re-compress
%.gz: %
	gzip -9 $^ -c > $@

%.lzma: %
	lzma -9 $^ -c > $@
