FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    chown root:tss ${D}/etc/tcsd.conf
    chmod 0640 ${D}/etc/tcsd.conf
}
