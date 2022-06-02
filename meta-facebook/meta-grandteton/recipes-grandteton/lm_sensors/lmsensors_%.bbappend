
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://gt.conf \
           "

do_install:append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../gt.conf ${D}${sysconfdir}/sensors.d/gt.conf
}
