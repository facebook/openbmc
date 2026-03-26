FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://tcu-muxer-helper \
    "

RDEPENDS:${PN}:append = "bash"

inherit systemd
inherit obmc-phosphor-systemd

SYSTEMD_SERVICE:${PN} = " \
        tcu-muxer-helper-cpu0.service \
        tcu-muxer-helper-cpu1.service \
        tcu-muxer-ttyUSB0.service \
        tcu-muxer-ttyUSB4.service \
        "

FILES:${PN} += " \
    ${bindir}/tcu-muxer-helper \
    ${sysconfdir}/tcu-muxer \
"

do_install:append() {
    install -d ${D}/${bindir}
    install -m 0755 ${UNPACKDIR}/tcu-muxer-helper ${D}${bindir}/

    install -d ${D}${sysconfdir}/tcu-muxer
}



