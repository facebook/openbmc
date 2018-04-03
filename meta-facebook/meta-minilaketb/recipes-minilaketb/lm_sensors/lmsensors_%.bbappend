
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://minilaketb.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../minilaketb.conf ${D}${sysconfdir}/sensors.d/minilaketb.conf
}
