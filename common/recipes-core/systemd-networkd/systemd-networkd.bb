SUMMARY = "Configure eth0 NIC under systemd"
DESCRIPTION = "Configure eth0 NIC for DHCP and SLAAC under systemd"
SECTION = "base"

LIC_FILES_CHKSUM = "file://eth0.network;beginline=3;endline=16;md5=0b1ee7d6f844d472fa306b2fee2167e0"

inherit systemd

LICENSE = "GPLv2"

SRC_URI = "file://eth0.network \
        file://usb0.network \
        file://eth0.4088.network \
        file://eth0.4088.netdev \
        file://eth0_mac_fixup.service \
        file://configure-eth0.service \
"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sysconfdir}/systemd/network
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 eth0.network ${D}${sysconfdir}/systemd/network
    install -m 0644 eth0.4088.network ${D}${sysconfdir}/systemd/network
    install -m 0644 eth0.4088.netdev ${D}${sysconfdir}/systemd/network
    install -m 0644 usb0.network ${D}${sysconfdir}/systemd/network
    install -m 0644 eth0_mac_fixup.service ${D}${systemd_system_unitdir}
    install -m 0644 configure-eth0.service ${D}${systemd_system_unitdir}
}

FILES_${PN} = "${sysconfdir} "
SYSTEMD_SERVICE_${PN} = "eth0_mac_fixup.service configure-eth0.service"
CONFFILES_${PN} = "${sysconfdir}/systemd/network/usb0.network \
                 ${sysconfdir}/systemd/network/eth0.network \
                 ${sysconfdir}/systemd/network/eth0.4088.network \
                 ${sysconfdir}/systemd/network/eth0.4088.netdev \
                 "
