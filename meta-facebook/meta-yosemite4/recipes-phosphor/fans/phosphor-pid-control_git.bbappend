FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://limitcore.conf \
"

EXTRA_OEMESON:append = " \
    -Doffline-failsafe-pwm=true \
    -Dhandle-missing-object-paths=true \
"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}/phosphor-pid-control.service.d/
    install -m 0644 ${UNPACKDIR}/limitcore.conf ${D}${systemd_system_unitdir}/phosphor-pid-control.service.d/
}
