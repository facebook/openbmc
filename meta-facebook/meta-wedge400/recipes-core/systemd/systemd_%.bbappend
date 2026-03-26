FILESEXTRAPATHS:prepend := "${THISDIR}/policy.conf:${THISDIR}/files:"

SRC_URI += " \
    file://journald-maxlevel-kmsg.conf \
    file://journald-loglevel.conf \
"

do_install:append() {
    # Suppress debug/info messages from being forwarded to kernel console
    install -m 644 -D ${UNPACKDIR}/journald-maxlevel-kmsg.conf \
        ${D}${systemd_unitdir}/journald.conf.d/maxlevel-kmsg.conf

    # Suppress journald's own debug messages (e.g., WATCHDOG notifications)
    install -m 644 -D ${UNPACKDIR}/journald-loglevel.conf \
        ${D}${systemd_system_unitdir}/systemd-journald.service.d/loglevel.conf
}
