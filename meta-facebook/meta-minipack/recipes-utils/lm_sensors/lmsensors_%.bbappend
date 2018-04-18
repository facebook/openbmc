
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://minipack.conf \
            file://0001-disable-bus-substitution.patch \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../minipack.conf ${D}${sysconfdir}/sensors.d/minipack.conf
}
