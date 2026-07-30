FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://0001-meta-add-Ventura2-network-initialization-support.patch \
"

DEPENDS:append = " \
    libmnl \
    libftdi \
"

EXTRA_OEMESON:append = " -Dplatform-name=meta-ventura2"

RDEPENDS:${PN}:append = " \
    kernel-module-mdio-netlink \
"
