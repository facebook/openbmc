FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://ncsi-bounce.timer \
    "
SYSTEMD_SERVICE:${PN}:append = " ncsi-bounce.timer"

do_install:append() {
    install -d ${D}${libexecdir}/${PN}
    install -m 0644 ${UNPACKDIR}/ncsi-bounce.timer ${D}${systemd_system_unitdir}
}
