
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://elbert.conf \
            file://pim16q.conf \
            file://pim8ddm.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../elbert.conf ${D}${sysconfdir}/sensors.d/elbert.conf
    install -m 644 ../pim16q.conf ${D}${sysconfdir}/sensors.d/.pim16q.conf
    install -m 644 ../pim8ddm.conf ${D}${sysconfdir}/sensors.d/.pim8ddm.conf
}
