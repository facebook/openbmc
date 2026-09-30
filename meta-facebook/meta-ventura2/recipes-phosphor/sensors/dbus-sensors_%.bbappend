FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

PACKAGECONFIG:append = " valvemonitor"

RDEPENDS:${PN} += "bash"

SRC_URI += " \
    file://xyz.openbmc_project.valve.open@.service \
    file://xyz.openbmc_project.valve.close@.service \
    file://valve-event-handler \
    file://0001-leakdetector-Raise-leakdetector-file-limit.patch \
"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/xyz.openbmc_project.valve.open@.service \
        ${D}${systemd_system_unitdir}/
    install -m 0644 ${UNPACKDIR}/xyz.openbmc_project.valve.close@.service \
        ${D}${systemd_system_unitdir}/

    LIBEXECDIR_PN="${D}${libexecdir}/${PN}"
    install -d ${LIBEXECDIR_PN}
    install -m 0755 ${UNPACKDIR}/valve-event-handler ${LIBEXECDIR_PN}
}

FILES:${PN} += "${systemd_system_unitdir}/*"
