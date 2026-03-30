LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit obmc-phosphor-utils
inherit systemd

RDEPENDS:${PN} += "bash"

SRC_URI += " \
    file://system-state-init \
    file://system-state-init@.service \
    "

do_install() {
    install -d ${D}${libexecdir}
    install -m 0755 ${UNPACKDIR}/system-state-init ${D}${libexecdir}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/system-state-init@.service \
        ${D}${systemd_system_unitdir}/system-state-init@.service
}

SYSTEMD_SERVICE_FMT = "system-state-init@{0}.service"

SYSTEMD_SERVICE:${PN} += "${@compose_list(d, 'SYSTEMD_SERVICE_FMT', 'OBMC_CHASSIS_INSTANCES')}"
FILES:${PN} += "${systemd_system_unitdir}/system-state-init@.service"
