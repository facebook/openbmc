
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://minipack.conf \
            file://0001-disable-bus-substitution.patch \
            file://adm1278-PIM16O.conf \
            file://adm1278-PIM16Q.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../minipack.conf ${D}${sysconfdir}/sensors.d/minipack.conf
    install -d ${D}${sysconfdir}/sensors.d/custom
    install -m 644 ../adm1278-PIM16O.conf ${D}${sysconfdir}/sensors.d/custom/adm1278-PIM16O.conf
    install -m 644 ../adm1278-PIM16Q.conf ${D}${sysconfdir}/sensors.d/custom/adm1278-PIM16Q.conf
}
