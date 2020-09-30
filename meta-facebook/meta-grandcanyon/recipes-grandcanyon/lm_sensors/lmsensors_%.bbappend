
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fbgc.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../fbgc.conf ${D}${sysconfdir}/sensors.d/fbgc.conf
}
