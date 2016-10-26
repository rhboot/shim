do_install_append() {
        install -m 0644 LICENCE.iwlwifi_firmware ${D}${FWPATH}
        install -m 0644 iwlwifi-6000g2a-6.ucode ${D}${FWPATH}
        install -m 0644 iwlwifi-135-6.ucode ${D}${FWPATH}
}

PACKAGES =+ "${PN}-iwlwifi-6000g2a-6 \
             ${PN}-iwlwifi-135-6"


RDEPENDS_${PN}-iwlwifi-6000g2a-6 = "${PN}-iwlwifi-licence"
RDEPENDS_${PN}-iwlwifi-135-6 = "${PN}-iwlwifi-licence"

FILES_${PN}-iwlwifi-licence =   "${FWPATH}/LICENCE.iwlwifi_firmware"
FILES_${PN}-rtl-license =       "${FWPATH}/LICENCE.rtlwifi_firmware.txt"
FILES_${PN}-iwlwifi-6000g2a-6 = "${FWPATH}/iwlwifi-6000g2a-6.ucode"
FILES_${PN}-iwlwifi-135-6 =     "${FWPATH}/iwlwifi-135-6.ucode"
