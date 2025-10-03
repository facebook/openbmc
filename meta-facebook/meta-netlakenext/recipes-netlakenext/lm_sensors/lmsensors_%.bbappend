
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://netlakenext.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../netlakenext.conf ${D}${sysconfdir}/sensors.d/netlakenext.conf
}
