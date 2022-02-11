
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://wedge.conf \
           "

do_install_board_config() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../wedge.conf ${D}${sysconfdir}/sensors.d/wedge.conf
}

do_install:append() {
    do_install_board_config
}
