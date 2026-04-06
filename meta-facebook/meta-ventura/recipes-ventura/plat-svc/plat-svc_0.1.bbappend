FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += " \
    file://ventura-adc-upper-bound \
    file://ventura-adc-upper-bound.service \
    "

SYSTEMD_SERVICE:${PN}:append = " \
    ventura-adc-upper-bound.service \
    "

do_install:append() {
    install -m 0755 ${UNPACKDIR}/ventura-adc-upper-bound ${VENTURA_LIBEXECDIR}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/ventura-adc-upper-bound.service ${D}${systemd_system_unitdir}
}
