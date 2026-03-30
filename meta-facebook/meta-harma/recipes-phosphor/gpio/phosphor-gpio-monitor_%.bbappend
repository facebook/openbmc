FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://mmc-recovery.conf"

inherit systemd

do_install:append() {
    install -d ${D}${systemd_system_unitdir}/mmc-recovery.service.d
    install -m 0644 ${UNPACKDIR}/mmc-recovery.conf \
        ${D}${systemd_system_unitdir}/mmc-recovery.service.d/mmc-recovery.conf
}

FILES:${PN}-monitor += "${systemd_system_unitdir}/mmc-recovery.service.d/mmc-recovery.conf"
