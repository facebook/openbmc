
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://netlakemtp.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../netlakemtp.conf ${D}${sysconfdir}/sensors.d/netlakemtp.conf
}
