
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://yamp.conf \
            file://0001-disable-bus-substitution.patch \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../yamp.conf ${D}${sysconfdir}/sensors.d/yamp.conf
}
