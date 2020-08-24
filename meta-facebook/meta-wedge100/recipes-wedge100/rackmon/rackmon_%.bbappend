FILESEXTRAPATHS_prepend := ":${THISDIR}/rackmon:"

SRC_URI += "file://rackmond.service file://configure-rackmond.service"

inherit systemd

do_install_append() {
    install -d ${D}${systemd_system_unitdir}

    install -m 0644 rackmond.service ${D}${systemd_system_unitdir}
    install -m 0644 configure-rackmond.service ${D}${systemd_system_unitdir}

    rm -rf ${D}${sysconfdir}/{init.d,sv,rc*.d}
}

SYSTEMD_SERVICE_${PN} = "rackmond.service configure-rackmond.service"

FILES_${PN} += "${systemd_system_unitdir}"
