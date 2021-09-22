
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://yamp.conf \
           "

do_install:append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../yamp.conf ${D}${sysconfdir}/sensors.d/yamp.conf
}
