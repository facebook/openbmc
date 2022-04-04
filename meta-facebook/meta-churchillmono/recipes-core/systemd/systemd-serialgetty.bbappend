SERIAL_CONSOLES = "57600;ttyS1 9600;ttyS0"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI = "file://serial-getty@.service"

do_install:append() {
    # Mask the default serial-getty@ttyS0.
    ln -s /dev/null "${D}${systemd_system_unitdir}/serial-getty@ttyS0.service"
}

