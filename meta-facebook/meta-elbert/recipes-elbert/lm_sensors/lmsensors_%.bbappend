
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://elbert.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../elbert.conf ${D}${sysconfdir}/sensors.d/elbert.conf
}
