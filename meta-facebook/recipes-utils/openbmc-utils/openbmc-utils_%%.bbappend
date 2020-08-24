inherit systemd

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fetch-backports.service \
            file://setup_i2c.service \
            file://power-on.service \
            file://setup_board.service \
            file://disable_watchdog.service \
            file://mnt-data.mount \
            file://early.service \
            file://format_mntdata.service \
            file://enable_watchdog_ext_signal.service \
"

FILES_${PN} += "${systemd_system_unitdir} /usr/local/bin"

do_install_append() {
    install -d ${D}${systemd_system_unitdir}
    install -d ${D}/usr/local/bin
    install -m 0644 fetch-backports.service ${D}${systemd_system_unitdir}
    install -m 0644 early.service ${D}${systemd_system_unitdir}
    install -m 0644 setup_i2c.service ${D}${systemd_system_unitdir}
    install -m 0644 mnt-data.mount ${D}${systemd_system_unitdir}
    install -m 0644 format_mntdata.service ${D}${systemd_system_unitdir}
    install -m 0644 setup_board.service ${D}${systemd_system_unitdir}
    install -m 0644 power-on.service ${D}${systemd_system_unitdir}
    install -m 0644 disable_watchdog.service ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/enable_watchdog_ext_signal.service ${D}${systemd_system_unitdir}
}


SYSTEMD_SERVICE_${PN} = "power-on.service \
                      setup_board.service \
                      disable_watchdog.service \
                      enable_watchdog_ext_signal.service \
                      mnt-data.mount \
                      early.service \
                      setup_i2c.service \
                      format_mntdata.service \
                      fetch-backports.service \
"
