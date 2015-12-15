
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://wedge100.conf \
           "

do_install_board_config() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../wedge100.conf ${D}${sysconfdir}/sensors.d/wedge100.conf
}
