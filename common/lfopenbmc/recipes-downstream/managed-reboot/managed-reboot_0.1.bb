LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit allarch systemd

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

RDEPENDS:${PN} += "bash"

SRC_URI += " \
    file://managed-reboot.service \
    file://managed-reboot \
    "

SYSTEMD_SERVICE:${PN}:append = " \
    managed-reboot.service \
    "

do_install() {
    install -d ${D}/${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/managed-reboot.service ${D}/${systemd_system_unitdir}/managed-reboot.service
    install -d ${D}${libexecdir}
    install -m 0755 ${UNPACKDIR}/managed-reboot ${D}${libexecdir}/managed-reboot
}
