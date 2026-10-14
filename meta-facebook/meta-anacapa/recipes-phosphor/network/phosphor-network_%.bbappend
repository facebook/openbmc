FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://ncsi-bounce.timer \
    file://91-bmc-mctpusb-network.rules \
    file://92-mctpusb-hotplug.rules \
    "

SYSTEMD_SERVICE:${PN}:append = " ncsi-bounce.timer"

FILES:${PN}:append = " \
    ${sysconfdir}/udev/rules.d/91-bmc-mctpusb-network.rules \
    ${sysconfdir}/udev/rules.d/92-mctpusb-hotplug.rules \
    "

do_install:append() {
    install -d ${D}${libexecdir}/${PN}
    install -m 0644 ${UNPACKDIR}/ncsi-bounce.timer ${D}${systemd_system_unitdir}

    install -d ${D}${sysconfdir}/udev/rules.d
    install -m 0644 ${UNPACKDIR}/91-bmc-mctpusb-network.rules ${D}${sysconfdir}/udev/rules.d/91-bmc-mctpusb-network.rules
    install -m 0644 ${UNPACKDIR}/92-mctpusb-hotplug.rules ${D}${sysconfdir}/udev/rules.d/92-mctpusb-hotplug.rules
}
