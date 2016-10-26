FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

#Quark errata - strip all lock prefixes from target binaries
SRC_URI += "file://gas2.23.1_WR.patch"
SRC_URI += "file://gas2.23.1_flip_lock_flag_logic.patch"
