PACKAGECONFIG:remove:openbmc-fb = "serial-getty-generator"

do_install:append:openbmc-fb() {
    rm -f ${D}${systemd_system_unitdir}/serial-getty*
}
