SUMMARY = "Configure eth0 NIC under systemd"
DESCRIPTION = "Configure eth0 NIC for DHCP and SLAAC under systemd"
SECTION = "base"

LIC_FILES_CHKSUM = "file://10-eth0.network;beginline=3;endline=16;md5=0b1ee7d6f844d472fa306b2fee2167e0"

inherit systemd

LICENSE = "GPL-2.0-or-later"

LOCAL_URI = " \
    file://10-eth0.network \
    file://30-usb0.network \
    file://20-eth0.4088.network \
    file://20-eth0.4088.netdev \
    file://eth0_mac_fixup.service \
    file://configure-eth0.service \
    "

do_install() {
    install -d ${D}${sysconfdir}/systemd/network
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 10-eth0.network ${D}${sysconfdir}/systemd/network
    install -m 0644 20-eth0.4088.network ${D}${sysconfdir}/systemd/network
    install -m 0644 20-eth0.4088.netdev ${D}${sysconfdir}/systemd/network
    install -m 0644 30-usb0.network ${D}${sysconfdir}/systemd/network
    install -m 0644 eth0_mac_fixup.service ${D}${systemd_system_unitdir}
    install -m 0644 configure-eth0.service ${D}${systemd_system_unitdir}
}

FILES:${PN} = "${sysconfdir} "
SYSTEMD_SERVICE:${PN} = "eth0_mac_fixup.service configure-eth0.service"
CONFFILES:${PN} = "${sysconfdir}/systemd/network/30-usb0.network \
                 ${sysconfdir}/systemd/network/10-eth0.network \
                 ${sysconfdir}/systemd/network/20-eth0.4088.network \
                 ${sysconfdir}/systemd/network/20-eth0.4088.netdev \
                 "
