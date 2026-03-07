LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit allarch systemd obmc-phosphor-systemd

S = "${UNPACKDIR}"
RDEPENDS:${PN} += "bash"

SRC_URI += " \
    file://fan-manual-mode.timer \
    file://fan-manual-mode.service \
    file://fan-manual-mode.sh \
    "

SYSTEMD_SERVICE:${PN}:append = " \
    fan-manual-mode.service \
    fan-manual-mode.timer \
    "

do_install() {
    install -d ${D}/${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/fan-manual-mode.service ${D}/${systemd_system_unitdir}/fan-manual-mode.service
    install -m 0644 ${UNPACKDIR}/fan-manual-mode.timer ${D}/${systemd_system_unitdir}/fan-manual-mode.timer
    install -d ${D}${libexecdir}
    install -m 0755 ${UNPACKDIR}/fan-manual-mode.sh ${D}${libexecdir}/fan-manual-mode
}
