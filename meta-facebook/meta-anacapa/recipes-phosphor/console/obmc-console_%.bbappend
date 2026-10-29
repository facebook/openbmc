FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://80-obmc-console-ttyACM.rules \
    file://server.ttyACM-nic1.conf \
    file://server.ttyACM-nic3.conf \
    file://server.ttyACM-nic5.conf \
    file://server.ttyACM-nic7.conf \
"

do_install:append() {
    install -d ${D}${nonarch_base_libdir}/udev/rules.d
    install -d ${D}${sysconfdir}/obmc-console

    # Install udev rules for AINIC/Mortaro USB ACM auto-start
    install -m 0644 ${UNPACKDIR}/80-obmc-console-ttyACM.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/

    # Install server configs for the 4 AINIC consoles, keyed by stable
    # /dev/ttyACM-nicx symlink names created by the udev rules above.
    install -m 0644 ${UNPACKDIR}/server.ttyACM-nic1.conf ${D}${sysconfdir}/obmc-console/
    install -m 0644 ${UNPACKDIR}/server.ttyACM-nic3.conf ${D}${sysconfdir}/obmc-console/
    install -m 0644 ${UNPACKDIR}/server.ttyACM-nic5.conf ${D}${sysconfdir}/obmc-console/
    install -m 0644 ${UNPACKDIR}/server.ttyACM-nic7.conf ${D}${sysconfdir}/obmc-console/
}