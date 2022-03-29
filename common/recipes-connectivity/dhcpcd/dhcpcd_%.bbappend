FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://dhclient"
RDEPENDS:${PN}:append = " bash"

do_install:append() {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/dhclient ${D}${sbindir}
}
