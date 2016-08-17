
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fbttn.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../fbttn.conf ${D}${sysconfdir}/sensors.d/fbttn.conf
}
