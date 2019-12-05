
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fbyv3.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../fbyv3.conf ${D}${sysconfdir}/sensors.d/fbyv3.conf
}
