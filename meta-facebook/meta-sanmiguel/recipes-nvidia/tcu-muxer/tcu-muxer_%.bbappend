FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://tcu-muxer-helper \
    file://tcu-muxer-helper-cpu0.service \
    file://tcu-muxer-helper-cpu1.service \
    file://tcu-muxer-ttyUSB0.service \
    file://tcu-muxer-ttyUSB4.service \
    "

RDEPENDS:${PN}:append = "bash"

inherit systemd

SYSTEMD_SERVICE:${PN} = " \
        tcu-muxer-helper-cpu0.service \
        tcu-muxer-helper-cpu1.service \
        tcu-muxer-ttyUSB0.service \
        tcu-muxer-ttyUSB4.service \
        "

FILES:${PN} += " \
    ${bindir}/tcu-muxer-helper \
    ${sysconfdir}/tcu-muxer \
    ${systemd_system_unitdir}/tcu-muxer-helper-cpu0.service \
    ${systemd_system_unitdir}/tcu-muxer-helper-cpu1.service \
    ${systemd_system_unitdir}/tcu-muxer-ttyUSB0.service \
    ${systemd_system_unitdir}/tcu-muxer-ttyUSB4.service \
"

do_install:append() {
    install -d ${D}/${bindir}
    install -m 0755 ${UNPACKDIR}/tcu-muxer-helper ${D}${bindir}/

    install -d ${D}${sysconfdir}/tcu-muxer

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/tcu-muxer-helper-cpu0.service ${D}${systemd_system_unitdir}/tcu-muxer-helper-cpu0.service
    install -m 0644 ${UNPACKDIR}/tcu-muxer-helper-cpu1.service ${D}${systemd_system_unitdir}/tcu-muxer-helper-cpu1.service
    install -m 0644 ${UNPACKDIR}/tcu-muxer-ttyUSB0.service ${D}${systemd_system_unitdir}/tcu-muxer-ttyUSB0.service
    install -m 0644 ${UNPACKDIR}/tcu-muxer-ttyUSB4.service ${D}${systemd_system_unitdir}/tcu-muxer-ttyUSB4.service
}



