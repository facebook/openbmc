#

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://gt.conf \
            file://gt_evt.conf \
            file://gt_evt2.conf \
           "

FILES:${PN}-libsensors += "${sysconfdir}/sensors_cfg"


do_install:append() {
    install -d ${D}${sysconfdir}/sensors_cfg
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../gt.conf ${D}${sysconfdir}/sensors.d/gt.conf
    install -m 644 ../gt_evt.conf ${D}${sysconfdir}/sensors_cfg/gt_evt.conf
    install -m 644 ../gt_evt2.conf ${D}${sysconfdir}/sensors_cfg/gt_evt2.conf
}
