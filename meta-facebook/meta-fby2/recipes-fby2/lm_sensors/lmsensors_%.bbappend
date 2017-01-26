
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fby2.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../fby2.conf ${D}${sysconfdir}/sensors.d/fby2.conf
}
