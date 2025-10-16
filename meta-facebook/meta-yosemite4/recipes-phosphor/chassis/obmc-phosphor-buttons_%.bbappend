FILESEXTRAPATHS:prepend  := "${THISDIR}/obmc-phosphor-buttons:"


SRC_URI += " \
    file://gpio_defs.json \
"
do_install:append() {
        install -d ${D}${sysconfdir}/default/obmc/gpio/
        install -m 0644 ${UNPACKDIR}/gpio_defs.json ${D}/${sysconfdir}/default/obmc/gpio/
}
