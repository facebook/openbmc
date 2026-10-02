FILESEXTRAPATHS:prepend := "${THISDIR}/files/lib:"
LOCAL_URI += "file://eeprom-devices.json \
              file://wedge-eeprom-config.py \
              file://wedge-eeprom-config.service \
              "

inherit systemd

do_install:prepend() {
    # The base recipe installs ${UNPACKDIR}/eeprom.json to /etc/weutil.
    # Ship the full device list as the default; wedge-eeprom-config.py
    # rewrites it at boot with just the fitted devices.
    cp ${UNPACKDIR}/eeprom-devices.json ${UNPACKDIR}/eeprom.json
}

do_install:append() {
    install -d ${D}/usr/local/bin
    install -d ${D}${systemd_system_unitdir}

    install -m 0755 ${UNPACKDIR}/wedge-eeprom-config.py ${D}/usr/local/bin
    install -m 0644 ${UNPACKDIR}/wedge-eeprom-config.service ${D}${systemd_system_unitdir}

    # wedge-eeprom-config.py generates eeprom.json from the full device list.
    install -m 0644 ${UNPACKDIR}/eeprom-devices.json ${D}/${sysconfdir}/weutil/eeprom-devices.json
}

RDEPENDS:${PN} += "python3-core python3-json"

FILES:${PN} += "/usr/local/bin"
FILES:${PN} += "${systemd_system_unitdir}/wedge-eeprom-config.service"
FILES:${PN} += "${sysconfdir}/weutil/eeprom-devices.json"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} += "wedge-eeprom-config.service"
