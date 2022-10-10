FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://dhclient \
    file://wait_mac_addr_ready \
    "

RDEPENDS:${PN}:append = " bash"

do_install_fake_dhclient() {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/dhclient ${D}${sbindir}
}

do_install:append() {
    # install fake dhclient
    do_install_fake_dhclient

    # set DUID type to DUID-LL
    sed -i 's/^duid.*/duid ll/g' ${D}${sysconfdir}/dhcpcd.conf

    # set SLAAC address generate from hardware address
    sed -i 's/^#slaac hwaddr/slaac hwaddr/g' ${D}${sysconfdir}/dhcpcd.conf
    sed -i 's/^slaac private/#slaac private/g' ${D}${sysconfdir}/dhcpcd.conf

    # Request a DHCPv6 Normal Address
    echo "" >> ${D}${sysconfdir}/dhcpcd.conf
    echo "# Request a DHCPv6 Normal Address" >> ${D}${sysconfdir}/dhcpcd.conf
    echo "ia_na" >> ${D}${sysconfdir}/dhcpcd.conf
    echo "" >> ${D}${sysconfdir}/dhcpcd.conf
    echo "# No IPv4LL address" >> ${D}${sysconfdir}/dhcpcd.conf
    echo "noipv4ll" >> ${D}${sysconfdir}/dhcpcd.conf
}

do_install:append:mf-ncsi() {
    install -d ${D}${libexecdir}/dhclient/pre.d
    install -m 0755 ${WORKDIR}/wait_mac_addr_ready \
        ${D}${libexecdir}/dhclient/pre.d/
}
