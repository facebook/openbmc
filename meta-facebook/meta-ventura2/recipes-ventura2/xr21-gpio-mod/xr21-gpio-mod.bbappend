FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://xr21-gpio-reset.service \
    file://override.conf \
"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/xr21-gpio-reset.service ${D}${systemd_system_unitdir}
    
    install -d ${D}${systemd_system_unitdir}/xr21-gpio-mod.service.d
    install -m 0644 ${UNPACKDIR}/override.conf ${D}${systemd_system_unitdir}/xr21-gpio-mod.service.d/
}
