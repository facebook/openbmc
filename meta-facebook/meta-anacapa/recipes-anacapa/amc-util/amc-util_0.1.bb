SUMMARY = "AMC Utility Tool"
DESCRIPTION = "AMC Utility Tool"
LICENSE = "CLOSED"

S = "${UNPACKDIR}"
SRC_URI += " \
    file://amc-util \
    file://amc-util-datetime-sync.service \
    file://amc-util-datetime-sync.timer \
    "

RDEPENDS:${PN} += " bash"

inherit systemd

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE:${PN} = "amc-util-datetime-sync.timer"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${UNPACKDIR}/amc-util ${D}${bindir}/amc-util

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/amc-util-datetime-sync.service ${D}${systemd_system_unitdir}/amc-util-datetime-sync.service
    install -m 0644 ${UNPACKDIR}/amc-util-datetime-sync.timer ${D}${systemd_system_unitdir}/amc-util-datetime-sync.timer
}

FILES:${PN} += "${systemd_system_unitdir}"
