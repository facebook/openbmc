
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://cmm.conf \
            file://0001-disable-bus-substitution.patch \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../cmm.conf ${D}${sysconfdir}/sensors.d/cmm.conf
}
