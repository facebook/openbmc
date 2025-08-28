FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://limitcore.conf \
    file://0001-dbuspassive-Register-signal-before-property-fetch.patch \
    file://monitor-pldm-sensor \
"

EXTRA_OEMESON:append = " \
    -Doffline-failsafe-pwm=true \
    -Dhandle-missing-object-paths=true \
"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}/phosphor-pid-control.service.d/
    install -m 0644 ${UNPACKDIR}/limitcore.conf ${D}${systemd_system_unitdir}/phosphor-pid-control.service.d/
}
