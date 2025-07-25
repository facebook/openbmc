FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

EXTRA_OEMESON:append = " \
    -Ddisable-host-power-monitoring=true \
    -Ddisable-in2-alarm-event=true \
    -Dfan_sensor_retry_attempts=5 \
"

SRC_URI += " \
    file://yosemite4-dbus-timeout.conf \
"

do_install:append() {
    for s in ${D}${systemd_system_unitdir}/*.service; do
        [ -e "$s" ] || continue
        install -d "$s.d"
        install -m 0644 ${UNPACKDIR}/yosemite4-dbus-timeout.conf "$s.d/"
    done
}

FILES:${PN} += " \
    ${systemd_system_unitdir}/*.service \
    ${systemd_system_unitdir}/*.service.d \
    ${systemd_system_unitdir}/*.service.d/*.conf \
"
