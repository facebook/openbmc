FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
        file://50-disable-watchdog.conf \
"

do_install:append() {
    # Disable systemd built-in watchdog，avoid conflicting with the watchdog.service use /dev/watchdog0
    install -d ${D}${sysconfdir}/systemd/system.conf.d/
    install -m 0644 -D ${UNPACKDIR}/50-disable-watchdog.conf ${D}${sysconfdir}/systemd/system.conf.d/50-disable-watchdog.conf
}