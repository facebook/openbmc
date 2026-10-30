FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://ventura2-adc-upper-bound \
    file://ventura2-adc-upper-bound.service \
    file://ventura2-evt-valve-dac-guard \
    file://ventura2-evt-valve-dac-guard.service \
    "

# ventura2-evt-valve-dac-guard
RDEPENDS:${PN} += "i2c-tools"

SYSTEMD_SERVICE:${PN}:append = " \
    ventura2-adc-upper-bound.service \
    ventura2-evt-valve-dac-guard.service \
    "

do_install:append() {
    install -d ${D}${libexecdir}/${PN}

    install -m 0755 ${UNPACKDIR}/ventura2-adc-upper-bound ${D}${libexecdir}/${PN}/
    install -m 0755 ${UNPACKDIR}/ventura2-evt-valve-dac-guard ${D}${libexecdir}/${PN}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/ventura2-adc-upper-bound.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${UNPACKDIR}/ventura2-evt-valve-dac-guard.service ${D}${systemd_system_unitdir}/
}
