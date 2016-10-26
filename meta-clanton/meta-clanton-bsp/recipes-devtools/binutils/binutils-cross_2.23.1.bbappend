FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

DEPENDS += "dejagnu-native"

#Quark errata - strip all lock prefixes from target binaries
SRC_URI += "file://gas2.23.1_WR.patch"
SRC_URI += "file://gas2.23.1_flip_lock_flag_logic.patch"
SRC_URI += "file://lock-test.patch"

do_execute_tests[nostamp] = "1"
do_execute_tests() {
        cd ${B}/gas
        echo ${B}/gas
        make check-DEJAGNU
}
addtask execute_tests before do_install after do_compile

