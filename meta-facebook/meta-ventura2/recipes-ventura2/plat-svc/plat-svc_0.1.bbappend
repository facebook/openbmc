FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://ventura2-adc-upper-bound \
    file://ventura2-adc-upper-bound.service \
    "

SYSTEMD_SERVICE:${PN}:append = " \
    ventura2-adc-upper-bound.service \
    "

do_install:append() {
    install -d ${D}${libexecdir}/${PN}
    
    install -m 0755 ${UNPACKDIR}/ventura2-adc-upper-bound ${D}${libexecdir}/${PN}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/ventura2-adc-upper-bound.service ${D}${systemd_system_unitdir}/
}
