
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://mavericks.conf \
           "

do_install_board_config() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../mavericks.conf ${D}${sysconfdir}/sensors.d/mavericks.conf
}
