
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://fbtp.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../fbtp.conf ${D}${sysconfdir}/sensors.d/fbtp.conf
}
