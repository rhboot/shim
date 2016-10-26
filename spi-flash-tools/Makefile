#!/usr/bin/make -f

# Note this is a GNUmakefile

# $(info MAKE_VERSION = ${MAKE_VERSION} )
ifeq (,${.FEATURES})
$(error this Makefile requires GNU make 3.81 or above)
endif

#############################################################
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
#############################################################
#
# Marc Herbert / Krzysztof Sywula
#
#############################################################
## USAGE:
##
## file-add_me_fvheader_and_sign-->FVWRAP-->\
##                                           --->SIGN
## file-only_sign_me----------------------->/
#############################################################

#############################################################
## GLOBAL DEFINITIONS
#############################################################

# WARNING: this MUST match the name in generator/src/SpiFlashImage.h
# It should also match the default argument of platform-data-patch.py
FLASH_NO_PDAT := Flash-missingPDAT.bin

FLASH_CAPSULE := ${FLASH_NO_PDAT:%.bin=%.cap}

RECOVERY      := FVMAIN.fv


# For more details about these flags see the UEFI PI specification and
# the UEFI firmware writer's guide
# - This flag changes EFI behaviour when the OS requests EFI to do the
#   reset
CAPSULE_FLAGS_PERSIST_ACROSS_RESET  := 0x10000
# - The "No return" flag
CAPSULE_FLAGS_INITIATE_RESET        := 0x40000
# - Resets and updates
CAPSULE_FLAGS_PERSIST_THEN_INITIATE := 0x50000

CAPSULE_FLAGS ?= ${CAPSULE_FLAGS_PERSIST_ACROSS_RESET}


# WARNING: this should match the default output name of platform-data-patch.py
FLASH_WITH_PDAT := Flash+PlatformData.bin

# Besides the flash images, these are all the final outputs useful to users
OTHER_GENERATOR_PRODUCTS := image_info.txt CapsuleComponents.ini mfh.bin

.DEFAULT_GOAL := all

# Convenience aliases
raw:      ${FLASH_NO_PDAT}
capsule:  ${FLASH_CAPSULE}
recovery: ${RECOVERY}

all:      recovery

MYSELF := $(lastword ${MAKEFILE_LIST})
MYABSELF := $(abspath ${MYSELF})
MYDIR  := $(dir ${MYSELF})


       ######      Recursive, "sysimages" targets    #######

# Search for layout.conf files in subdirectories
_subdirs := $(wildcard sysimage*/layout.conf)
IMAGE_SUBDIRS := $(dir ${_subdirs})


ifeq (,${MAKECMDGOALS})
NO_TARGET_ON_CLI := non_empty_placeholder
endif

# From the list of targets on the command line, extract the
# list of "current directory", non-recursive targets
CURDIR_TARGETS := $(filter-out sysimage.% sysimages%         \
                               ramfs%     initramfs%   zip   \
                               asset-signing-tool/%          \
                 ,  ${MAKECMDGOALS})


ifneq (,${CURDIR_TARGETS}${NO_TARGET_ON_CLI})
#     At least one sub-directory target was given on the command line
#  OR no target at all                  was given on the command line 
#
#       => we need a layout.mk in the current directory

include layout.mk

endif

ifneq (${CURDIR_TARGETS},${MAKECMDGOALS})
#    => Some subdirectory target(s) was passed on the command line

$(info |       ---------------------- )
$(info | List of sub-directories with a layout.conf file: ${IMAGE_SUBDIRS})
$(info |       ---------------------- )

endif

CFLAGS ?= -Wall -Wextra -g

# TOOL PATHS ------------------------------------------------
# Can be overriden on the command line, see BaseTools/README.txt
BASETOOLS           ?= ${MYDIR}BaseTools/x86_64/C/bin

ifneq (,${CSSTOOLS})
$(warning warning: CSSTOOLS variable now ignored: asset \
	signing tool has been embedded into SPI flash tools )
endif
SIGNTOOL           := ${MYDIR}asset-signing-tool

SIGN_HEADER_SIZE   := 1024

KEYFILE             ?= ./config/key.pem
CAPSULE_KEYFILE     ?= ${KEYFILE}
CAPSULE_SVNINDEX    ?= 0xF
CAPSULE_SVN         ?= 0x0
GENERATORTOOL       ?= ${MYDIR}generator

# TOOLS -----------------------------------------------------
#SPIFLASHGENERATOR   := ${GENERATORTOOL}/SpiFlashGenerator
SPIFLASHGENERATOR   := ${MYDIR}/generator/generator.py

GENFW               := ${BASETOOLS}/GenFw
GENSEC              := ${BASETOOLS}/GenSec
GENFFS              := ${BASETOOLS}/GenFfs
GENFV               := ${BASETOOLS}/GenFv


CSSINPUT            := $(SIGNTOOL)/exports/lastbuilt/bin/cssinput
CODESIGNEMULATOR    := $(SIGNTOOL)/exports/lastbuilt/bin/codesignemulator

SIGNEXE             := $(SIGNTOOL)/sign

# CONFIGURATION FILE ----------------------------------------
LAYOUTFILE          ?= ./layout.conf

INSTALL_DIR         ?= install
INSTALLS            ?= installs

# EDK -------------------------------------------------------

# For CapsuleCreate
FV_TOOLS                   := ${BOOT_INDEX_ZERO_DIR}FV/Tools

# For recovery image
FV_MODULES                 := ${BOOT_INDEX_ZERO_DIR}FV/FlashModules
$(info  --------------------- )
$(info  | FV_MODULES directory (for recovery stage2) = ${FV_MODULES})
$(info  --------------------- )


# ------ Signing uncompressed recovery stage 2 volume for recovery ----- 

# This is folding the uncompressed recovery stage 2 special case
# back into the general signing rules. This one requires special
# treatment because it is not in the layout.conf (the "COMPACT",
# compressed stage2 is instead)

# ** All this relies on recovery and compact living in the
# same directory! **

# FIXME: generator.py should search for type=mfh.host_fw_stage2_signed
# and define the first one for us in the layout.mk
_COMPACT_BASENAME         := EDKII_BOOT_STAGE2_COMPACT.Fv
_RECOVERY_BASENAME        := EDKII_BOOT_STAGE2_RECOVERY.Fv

ifeq (,$(findstring ${_COMPACT_BASENAME},${SYMLINKS}))

# $(info Found relative ${_COMPACT_BASENAME} path: prefixing ${_RECOVERY_BASENAME} with ${FV_MODULES})

    RECOVERY_STAGE2        := ${FV_MODULES}/${_RECOVERY_BASENAME}
     COMPACT_STAGE2        := ${FV_MODULES}/${_COMPACT_BASENAME}

else

# $(info Found absolute path for ${_COMPACT_BASENAME}; manually addding the symlink)
# $(info logic for ${_RECOVERY_BASENAME} since it s not in the layout.conf)

    RECOVERY_STAGE2        := ${_RECOVERY_BASENAME}
     COMPACT_STAGE2        := ${_COMPACT_BASENAME}

SYMLINKS                   += ${_RECOVERY_BASENAME}
${RECOVERY_STAGE2}: ${FV_MODULES}/${_RECOVERY_BASENAME}

endif

COMPACT_STAGE2_SVNINDEX := $(${COMPACT_STAGE2}.SVNINDEX)
COMPACT_STAGE2_KEYFILE  := $(${COMPACT_STAGE2}.KEYFILE)

# If the user did not give an svn_index or keyfile for the COMPACT_STAGE2
# (for instance if it is pre-signed or not signed)
# we replace the empty strings with (more expressive) bogus values
# We do not error straight away so people can still build
# raw images and capsules

ifeq (,${COMPACT_STAGE2_SVNINDEX})
${RECOVERY_STAGE2}.SVNINDEX := ${COMPACT_STAGE2}.SVNINDEX_is_not_defined_in_layout.mk
else
${RECOVERY_STAGE2}.SVNINDEX := ${COMPACT_STAGE2_SVNINDEX}
endif

ifeq (,${COMPACT_STAGE2_KEYFILE})
${RECOVERY_STAGE2}.KEYFILE := ${COMPACT_STAGE2}.KEYFILE_is_not_defined_in_layout.mk
else
${RECOVERY_STAGE2}.KEYFILE := ${COMPACT_STAGE2_KEYFILE}
endif


#############################################################
# SOURCES - TO BE WRAPPED WITH FV HEADER --------------------
# PATTERN: --------------------------------------------------
# SRCS_FVWRAP        := firstfile.efi secondfile
# firstfile.efi.GUID := B43BD3E1-64D1-4744-9394-D0E1C4DE8C87
# secondfile.GUID    := C13BF1A9-34B1-1024-7122-E0E3CFDA8089
# SRCS_FVWRAP variable comes from layout.mk file
#############################################################

OBJS_FVWRAP     := ${SRCS_FVWRAP:=.fv}
OBJS_FW         := ${SRCS_FVWRAP:=.fw}
OBJS_PE32       := ${SRCS_FVWRAP:=.fw.pe32}

#############################################################
# SOURCES - TO BE SIGNED ------------------------------------
# PATTERN: --------------------------------------------------
# SRCS_SIGN     := firstfile.efi.fv secondfile
# SRCS_SIGN variable comes from layout.mk file
#############################################################

OBJS_SIGN       := ${SRCS_SIGN:=.signed}
OBJS_SVN        := ${SRCS_SIGN:=.SVN}
OBJS_CSBH       := ${SRCS_SIGN:=.csbh}
OBJS_ALIGN      := ${SRCS_SIGN:=.aligned}
OBJS_CSSINPUT   := ${SRCS_SIGN:=.cssinput}

#############################################################
## TARGETS (DON'T EDIT!)
#############################################################

.DELETE_ON_ERROR:


sysimages: ${IMAGE_SUBDIRS}

.PHONY: sysimages sysimagesclean ${IMAGE_SUBDIRS} sysimages-install

${IMAGE_SUBDIRS}: ${SIGNEXE}
	$(MAKE) -C $@ -f ${MYABSELF}

# This is just for CLI convenience: do NOT start using this variable anywhere else
_IMAGE_SUBDIRS_SLASH := ${IMAGE_SUBDIRS:%=%/}
.PHONY: ${_IMAGE_SUBDIRS_SLASH}
${_IMAGE_SUBDIRS_SLASH}: %/: %

sysimages-clean:
	${RM} -r ${INSTALLS}
	for i in ${IMAGE_SUBDIRS}; do $(MAKE) -C "$$i" -f  ${MYABSELF} clean || true; done

# This target is "best effort" *by design*: it recursively builds
# everything it can ignoring all BUILD errors in subdirectories; then
# it installs all it can. Note: this target DOES fail on INSTALL
# errors, see "wait" code below.
# 
# To abort on the very first build error do not use this target
# *alone*, you MUST call instead:
#
#      spi-flash-tools/Makefile [ -j ] sysimages && spi-flash-tools/Makefile sysimages-install
#
# A for loop is required so one failing sysimage build does not stop
# the others, it's the only way. However a for loop harms parallel
# execution; backgrounding fixes that.
sysimages-install:
	mkdir -p ${INSTALLS}
	children=""; for i in ${IMAGE_SUBDIRS}; do \
	{ $(MAKE) -C "$$i" -f  ${MYABSELF} all || true; \
	$(MAKE) -C "$$i" -f  ${MYABSELF} install INSTALL_DIR=../${INSTALLS}/"$$i"; } & \
	children="$${children} $$!"; done; \
	for chPID in $${children}; do wait $$chPID || exit $$?; done


.PHONY: all fvwrap sign zip clean generatorclean raw capsule recovery

# -----------------------------------------------------------
${SYMLINKS}:
	ln -s '$^' '$@'

# -----------------------------------------------------------
fvwrap: $(OBJS_FVWRAP)

$(OBJS_FVWRAP): %.fv : %
	$(RM) image.ffs
	$(GENFW) $* -o $*.fw -e UEFI_APPLICATION || { cat ${MYDIR}BaseTools/README.txt; exit 1; }
	$(GENSEC) -s EFI_SECTION_PE32 $*.fw -o $*.fw.pe32
	$(GENFFS) -o image.ffs -t EFI_FV_FILETYPE_APPLICATION -g $($*.GUID) -i $*.fw.pe32
	$(GENFV)  -i inf/$(notdir $*.inf) -o $*.fv

# -----------------------------------------------------------
sign: $(OBJS_SIGN)

${OBJS_SVN} ${RECOVERY_STAGE2}.SVN:
	# Hardcoding all SVNs to zero. TODO collect these from the built assets.
	printf '0x0' > $@


# ----  New sign tool  -----

${SIGNEXE}: ${SIGNEXE}.o
	${CC} ${CFLAGS} $^ -lcrypto -o $@

ifneq (true,${USE_OLD_CSS})

$(OBJS_SIGN) ${RECOVERY_STAGE2}.signed:  %.signed : % %.SVN  ${SIGNEXE} ${KEY_FILES}
	${SIGNEXE} -i $< -o $@ -k '$($*.KEYFILE)' -b 0x400 -s "$$(cat $*.SVN)" -x '$($*.SVNINDEX)'

endif


# ------- Old CSS sign tool kept for test and comparison -------------


$(OBJS_CSSINPUT): %.cssinput : %     ${CSSINPUT}
	$(CSSINPUT) -input $< -bodyoffset 0x400 -svn 0 -svnindex $($*.SVNINDEX) -output $@ >/dev/null


OBJS_OLD_CSS_SIGN := ${SRCS_SIGN:=.css-signed}

ifeq (true,${USE_OLD_CSS})
      ## Old CSS tool is here the default and only tool

$(OBJS_SIGN) : %.signed : %.cssinput ${KEYFILE} ${CODESIGNEMULATOR}
else
      ## Old CSS tool is not the default here but can still be invoked
      ## for comparison

$(OBJS_OLD_CSS_SIGN): %.css-signed : %.cssinput  ${KEYFILE} ${CODESIGNEMULATOR}
endif
	${RM} $@
	>/dev/null $(CODESIGNEMULATOR) -input $< -output $@ -keyfile "${KEYFILE}"
# because codesignemulator does not report (all) errors in its exit status
	test -e $@
	dd if=$@ of=$*.csbh bs=${SIGN_HEADER_SIZE} count=1


ifneq (true,${USE_OLD_CSS})
# Build old and compare with new
.PHONY: old-css-sign
old-css-sign: sign ${OBJS_OLD_CSS_SIGN}
	for i in $^; do test $$i = 'sign' || diff -s $$i $${i%.css-signed}.signed; done
endif


${CSSINPUT} ${CODESIGNEMULATOR}:
	${MAKE} -C ${SIGNTOOL}


PROCESSED_IMAGE_SECTIONS := $(OBJS_SIGN) $(OBJS_FVWRAP) $(SRCS_UNMODIFIED)
 
# -----------------------------------------------------------
${FLASH_NO_PDAT} ${OTHER_GENERATOR_PRODUCTS}: $(SPIFLASHGENERATOR) $(LAYOUTFILE)  ${PROCESSED_IMAGE_SECTIONS}
	$(SPIFLASHGENERATOR)  $(LAYOUTFILE)
	${GENERATORTOOL}/dediprog-checksum.py ${FLASH_NO_PDAT}
	${GENERATORTOOL}/scan-padding.py ${FLASH_NO_PDAT}

# Note the spurious dependency on building sections which are NOT in
# the capsule. Does not make any difference at the moment since
# CapsuleComponents.ini can only be generated while building the
# complete raw image anyway.
${FLASH_CAPSULE}: ${SIGNEXE} ${CAPSULE_KEYFILE} ${PROCESSED_IMAGE_SECTIONS} CapsuleComponents.ini
	${FV_TOOLS}/CapsuleCreate CapsuleComponents.ini  ./  _$@.noheader ${CAPSULE_FLAGS}
	${SIGNEXE} -i _$@.noheader -o $@ -k '${CAPSULE_KEYFILE}' -b 0x400 -s '${CAPSULE_SVN}' -x '${CAPSULE_SVNINDEX}'
	${RM} _$@.noheader

# The capsule prerequisite is the first one of the two so we get at
# least the capsule built in case of some (more likely) recovery 
# stage2 build issue.
${RECOVERY}: ${FLASH_CAPSULE} ${RECOVERY_STAGE2}.signed
	cat  ${RECOVERY_STAGE2}.signed ${FLASH_CAPSULE} > $@

${FLASH_WITH_PDAT}: ${FLASH_NO_PDAT} ./platform-data.ini
	${MYDIR}/platform-data/platform-data-patch.py --undefined-order
	${GENERATORTOOL}/dediprog-checksum.py $@

# -----------------------------------------------------------
layout.mk: $(SPIFLASHGENERATOR) $(LAYOUTFILE)
	$(SPIFLASHGENERATOR) $(LAYOUTFILE) -M
	# When building on NFS with high precision timestamps you
	# might see a harmless "clock skew" warning. If that bothers
	# you then enable the next line:
	# usleep 100000 || sleep 1

# Alias 'install' to '$INSTALL_DIR' when they are different
ifneq (install,${INSTALL_DIR})
install: ${INSTALL_DIR}
endif
.PHONY: ${INSTALL_DIR}

# If this become too big we could dump this list to a temporary
# "files2install.rsync" file and then use:
#    rsync --files-from=files2install.rsync .
FILES2INSTALL := ${FLASH_NO_PDAT} ${LAYOUTFILE} ${OTHER_GENERATOR_PRODUCTS}
CAPSULES      := ${FLASH_CAPSULE} ${RECOVERY}

# We only depend on the raw flash image to preserve support for non-capsule builds
${INSTALL_DIR}: ${FLASH_NO_PDAT}
	${RM} -r "${INSTALL_DIR}/"
	rsync -ar --delete  ${FILES2INSTALL} "${INSTALL_DIR}/"
	- cp -a ${CAPSULES} "${INSTALL_DIR}/"  # don't fail on missing capsules


# -----------------------------------------------------------
zip:
	${MAKE} -C ${GENERATORTOOL} $@

# -----------------------------------------------------------
clean: ramfs-clean
	$(RM) ${OBJS_FVWRAP}.txt ${OBJS_FVWRAP}.map \
	${OBJS_FVWRAP} ${OBJS_FW} ${OBJS_PE32} image.ffs \
	${OBJS_SIGN} ${OBJS_CSSINPUT} $(OBJS_CSBH) $(SYMLINKS) \
	${OBJS_OLD_CSS_SIGN} \
	layout.mk ${FLASH_NO_PDAT} ${FLASH_WITH_PDAT} ${OTHER_GENERATOR_PRODUCTS} ${CAPSULES}
	${RM} ${OBJS_SVN} ${RECOVERY_STAGE2}.SVN
	${RM} -r ${INSTALL_DIR}
	${RM} ${RECOVERY_STAGE2}.signed

generatorclean: clean
	${RM} ${SIGNEXE} ${SIGNEXE}.o
	${MAKE} -C ${GENERATORTOOL} clean
	${MAKE} -C ${SIGNTOOL} clean


#############################################################
#   Optional feature to insert/overwrite kernel modules
#   (or more) into an existing cpio initramfs
#############################################################
-include ${MYDIR}/ramfs.mk

