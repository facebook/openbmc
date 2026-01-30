FILESEXTRAPATHS:prepend := "${THISDIR}/files/lib:"
LOCAL_URI += "file://eeprom.json \
              file://rackmon-eeprom.json \
              file://wedge-eeprom-config.sh \
              file://wedge-eeprom-config.service \
              "

inherit systemd

do_install:append() {
    install -d ${D}/usr/local/bin
    install -d ${D}${systemd_system_unitdir}

    install -m 0755 ${UNPACKDIR}/wedge-eeprom-config.sh ${D}/usr/local/bin
    install -m 0644 ${UNPACKDIR}/wedge-eeprom-config.service ${D}${systemd_system_unitdir}

    install -m 0644 ${UNPACKDIR}/rackmon-eeprom.json ${D}/${sysconfdir}/weutil/rackmon-eeprom.json
}

RDEPENDS:${PN} += "bash"

FILES:${PN} += "/usr/local/bin"
FILES:${PN} += "${systemd_system_unitdir}/wedge-eeprom-config.service"
FILES:${PN} += "${sysconfdir}/weutil/rackmon-eeprom.json"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} += "wedge-eeprom-config.service"
