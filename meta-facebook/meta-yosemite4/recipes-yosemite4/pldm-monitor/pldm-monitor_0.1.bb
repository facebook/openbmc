LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit allarch systemd

RDEPENDS:${PN} += "bash"

S = "${UNPACKDIR}"
SRC_URI += " \
    file://pldm-monitor.timer \
    file://pldm-monitor.service \
    file://pldm-monitor.sh \
    "

SYSTEMD_SERVICE:${PN}:append = " \
    pldm-monitor.service \
    pldm-monitor.timer \
    "

do_install() {
    install -d ${D}/${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/pldm-monitor.service ${D}/${systemd_system_unitdir}/pldm-monitor.service
    install -m 0644 ${UNPACKDIR}/pldm-monitor.timer ${D}/${systemd_system_unitdir}/pldm-monitor.timer
    install -d ${D}${libexecdir}
    install -m 0755 ${UNPACKDIR}/pldm-monitor.sh ${D}${libexecdir}/pldm-monitor
}

FILES:${PN} += "${systemd_system_unitdir}"
