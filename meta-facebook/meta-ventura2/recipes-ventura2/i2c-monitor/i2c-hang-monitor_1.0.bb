SUMMARY = "AST2600 I2C hang monitor and MCTP/PLDM recovery"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit systemd

SRC_URI = " \
    file://i2c-hang-monitor \
    file://i2c-hang-monitor.service \
    file://i2c-hang-monitor.conf \
"

SYSTEMD_SERVICE:${PN} = "i2c-hang-monitor.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install() {
    install -d ${D}${libexecdir}
    install -m 0755 ${UNPACKDIR}/i2c-hang-monitor ${D}${libexecdir}/i2c-hang-monitor

    install -d ${D}${sysconfdir}/default
    install -m 0644 ${UNPACKDIR}/i2c-hang-monitor.conf ${D}${sysconfdir}/default/i2c-hang-monitor

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/i2c-hang-monitor.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${systemd_system_unitdir}/* ${libexecdir}/* ${sysconfdir}/default/*"
