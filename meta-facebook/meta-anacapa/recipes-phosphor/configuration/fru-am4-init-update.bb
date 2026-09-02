SUMMARY = "FRU AM4 init update helper"
DESCRIPTION = "Install FRU AM4 update and manual restore helpers"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit systemd

FILESEXTRAPATHS:prepend := "${THISDIR}/fru-am4-init-update/files:"

SRC_URI = " \
    file://fru-am4-init-update \
    file://fru-am4-restore-from-backup \
    file://fru-am4-init-update.service \
"

RDEPENDS:${PN} += "bash"

SYSTEMD_SERVICE:${PN} = "fru-am4-init-update.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

FILES:${PN} += " \
    ${libexecdir}/phosphor-state-manager/fru-am4-init-update \
    ${sbindir}/fru-am4-restore-from-backup \
    ${systemd_system_unitdir}/fru-am4-init-update.service \
"

do_install() {
    install -d ${D}${libexecdir}/phosphor-state-manager
    install -m 0755 ${UNPACKDIR}/fru-am4-init-update ${D}${libexecdir}/phosphor-state-manager/

    install -d ${D}${sbindir}
    install -m 0755 ${UNPACKDIR}/fru-am4-restore-from-backup ${D}${sbindir}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/fru-am4-init-update.service ${D}${systemd_system_unitdir}/
}
