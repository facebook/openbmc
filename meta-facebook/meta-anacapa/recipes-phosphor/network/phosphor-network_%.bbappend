FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://ncsi-bounce.timer \
    file://00-amc-usb0.network \
    "

SYSTEMD_SERVICE:${PN}:append = " ncsi-bounce.timer"

FILES:${PN}:append = " \
    ${sysconfdir}/systemd/network/00-amc-usb0.network \
    "

do_install:append() {
    install -d ${D}${libexecdir}/${PN}
    install -m 0644 ${UNPACKDIR}/ncsi-bounce.timer ${D}${systemd_system_unitdir}

    install -d ${D}${sysconfdir}/systemd/network/
    install -m 0644 ${UNPACKDIR}/00-amc-usb0.network ${D}${sysconfdir}/systemd/network/00-amc-usb0.network
}
