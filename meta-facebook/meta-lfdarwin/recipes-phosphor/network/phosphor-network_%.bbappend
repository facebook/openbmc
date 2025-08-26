FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append ="file://eth0_mac_fixup.service \
                 file://configure-eth0.service \
                 "

inherit systemd

do_install:append() {
    install -d ${D}${systemd_system_unitdir}

    install -m 0644 ${UNPACKDIR}/eth0_mac_fixup.service ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/configure-eth0.service ${D}${systemd_system_unitdir}
}

FILES:${PN}:append = " \
    ${systemd_system_unitdir}/eth0_mac_fixup.service \
    ${systemd_system_unitdir}/configure-eth0.service \
"

SYSTEMD_SERVICE:${PN}:append:fb-fboss = "eth0_mac_fixup.service configure-eth0.service"
