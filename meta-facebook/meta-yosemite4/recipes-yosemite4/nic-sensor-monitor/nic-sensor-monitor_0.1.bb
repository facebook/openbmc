LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit allarch systemd

RDEPENDS:${PN} += "bash expect"

S = "${UNPACKDIR}"
SRC_URI += " \
    file://nic-sensor-monitor.timer \
    file://nic-sensor-monitor.service \
    file://nic-sensor-monitor.sh \
    file://slice_diagnostics.expect \
    "

SYSTEMD_SERVICE:${PN}:append = " \
    nic-sensor-monitor.service \
    nic-sensor-monitor.timer \
    "

do_install() {
    install -d ${D}/${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/nic-sensor-monitor.service ${D}/${systemd_system_unitdir}/nic-sensor-monitor.service
    install -m 0644 ${UNPACKDIR}/nic-sensor-monitor.timer ${D}/${systemd_system_unitdir}/nic-sensor-monitor.timer
    install -d ${D}${libexecdir}
    install -m 0755 ${UNPACKDIR}/nic-sensor-monitor.sh ${D}${libexecdir}/nic-sensor-monitor
    install -m 0755 ${UNPACKDIR}/slice_diagnostics.expect ${D}${libexecdir}/nic-slice-diagnostics
}

FILES:${PN} += "${systemd_system_unitdir}"
