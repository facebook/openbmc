FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://wantedby.conf \
"

FILES:${PN} += "${systemd_system_unitdir}/*"

do_install:append () {
    override_dir=${D}${systemd_system_unitdir}/mctpd.service.d
    install -d ${D}${systemd_system_unitdir}/mctpd.service.d

    install -m 0644 ${UNPACKDIR}/wantedby.conf ${override_dir}/wantedby.conf
}


