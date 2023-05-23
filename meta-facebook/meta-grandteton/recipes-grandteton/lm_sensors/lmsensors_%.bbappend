#

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://common.conf \
            file://cover.conf \
            file://evt.conf \
            file://evt2.conf \
            file://adm1272-1.conf \
            file://adm1272-2.conf \
           "

FILES:${PN}-libsensors += "${sysconfdir}/sensors_cfg"


do_install:append() {
    install -d ${D}${sysconfdir}/sensors_cfg
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../common.conf ${D}${sysconfdir}/sensors.d/common.conf
    install -m 644 ../cover.conf ${D}${sysconfdir}/sensors.d/cover.conf
    install -m 644 ../evt.conf   ${D}${sysconfdir}/sensors_cfg/evt.conf
    install -m 644 ../evt2.conf ${D}${sysconfdir}/sensors_cfg/evt2.conf
    install -m 644 ../adm1272-1.conf ${D}${sysconfdir}/sensors_cfg/adm1272-1.conf
    install -m 644 ../adm1272-2.conf ${D}${sysconfdir}/sensors_cfg/adm1272-2.conf
}
