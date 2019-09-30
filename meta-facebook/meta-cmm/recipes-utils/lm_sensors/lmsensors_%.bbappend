
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://cmm.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../cmm.conf ${D}${sysconfdir}/sensors.d/cmm.conf
}
