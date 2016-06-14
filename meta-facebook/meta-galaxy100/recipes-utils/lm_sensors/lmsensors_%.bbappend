
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://galaxy100_lc.conf \
			file://galaxy100_fab.conf \
           "

do_install_board_config() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../galaxy100_lc.conf ${D}${sysconfdir}/sensors.d/galaxy100_lc.conf
    install -m 644 ../galaxy100_fab.conf ${D}${sysconfdir}/sensors.d/galaxy100_fab.conf
}
