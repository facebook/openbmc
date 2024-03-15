LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit obmc-phosphor-systemd

RDEPENDS:${PN} += " bash"

SRC_URI += " \
    file://system-state-init \
    file://system-state-init@.service \
    "

do_install() {
    install -d ${D}${libexecdir}
    install -m 0755 ${WORKDIR}/system-state-init ${D}${libexecdir}
}

TGT = "${SYSTEMD_DEFAULT_TARGET}"
HARMA_SYS_ST_INIT_INSTFMT="../system-state-init@.service:multi-user.target.wants/system-state-init@{0}.service"

SYSTEMD_SERVICE:${PN} += "system-state-init@.service"
SYSTEMD_LINK:${PN} += "${@compose_list(d, 'HARMA_SYS_ST_INIT_INSTFMT', 'OBMC_CHASSIS_INSTANCES')}"
