SERIAL_CONSOLES = "9600;ttyS0"

do_install:append() {
    # Mask the default serial-getty@ttyS0. We boot with 9600 bauds and
    # that's the serial speed we want here but we may get another
    # getty at 57600, and the console goes bananas over conflicting
    # speeds.
    ln -s /dev/null "${D}${systemd_system_unitdir}/serial-getty@ttyS0.service"
}
