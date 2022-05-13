FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://dhclient \
    file://wait_mac_addr_ready \
    "

RDEPENDS:${PN}:append = " bash"

do_install:append() {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/dhclient ${D}${sbindir}
}

do_install:append:mf-ncsi() {
    install -d ${D}${libexecdir}/dhclient/pre.d
    install -m 0755 ${WORKDIR}/wait_mac_addr_ready \
        ${D}${libexecdir}/dhclient/pre.d/
}
