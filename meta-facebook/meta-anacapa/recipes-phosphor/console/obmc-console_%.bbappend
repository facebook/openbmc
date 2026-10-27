FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://80-obmc-console-ttyACM.rules \
    file://server.ttyACM0.conf \
    file://server.ttyACM1.conf \
    file://server.ttyACM2.conf \
    file://server.ttyACM3.conf \
"

do_install:append() {
    install -d ${D}${nonarch_base_libdir}/udev/rules.d
    install -d ${D}${sysconfdir}/obmc-console

    # Install udev rules for AINIC/Mortaro USB ACM auto-start
    install -m 0644 ${UNPACKDIR}/80-obmc-console-ttyACM.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/

    # Install server configs for all 4 AINIC ttyACM devices
    install -m 0644 ${UNPACKDIR}/server.ttyACM0.conf ${D}${sysconfdir}/obmc-console/
    install -m 0644 ${UNPACKDIR}/server.ttyACM1.conf ${D}${sysconfdir}/obmc-console/
    install -m 0644 ${UNPACKDIR}/server.ttyACM2.conf ${D}${sysconfdir}/obmc-console/
    install -m 0644 ${UNPACKDIR}/server.ttyACM3.conf ${D}${sysconfdir}/obmc-console/
}
