SERIAL_CONSOLES = "9600;ttyS0 57600;ttyS2"

do_install_append() {
    # Undo the masking we had on the wedge100 layer - from which we
    # inherit. OMG this is awful.
    rm -f "${D}${systemd_system_unitdir}/serial-getty@ttyS2.service"
}
