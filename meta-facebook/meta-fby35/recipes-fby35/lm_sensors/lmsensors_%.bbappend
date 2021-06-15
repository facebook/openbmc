
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fby35.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../fby35.conf ${D}${sysconfdir}/sensors.d/fby35.conf
}
